#include "app.h"

#include <deque>
#include <format>
#include <gsl/gsl>
#include <iostream>
#include <stdexcept>

#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"

#ifdef _WIN32
#include <windows.h>  // SetProcessDPIAware()
#endif

VkResult CreateDebugMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger) {
  const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  if (func != nullptr) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  }

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debug_messenger,
                              const VkAllocationCallbacks* p_allocator) {
  const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debug_messenger, p_allocator);
  }
}

void CheckVkResult(const VkResult err) {
  if (err == 0) {
    return;
  }
  std::cerr << std::format("[vulkan] Error: VkResult = {}",
                           static_cast<int>(err))
            << '\n';
  if (err < 0) {
    abort();
  }
}

void app::Application::Run() {
#ifdef _WIN32
  SetProcessDPIAware();
#endif

  InitSDL();
  InitVulkan();
  SetupVulkanWindow();
  InitImGui();
  MainLoop();
  Cleanup();
}

void app::Application::InitSDL() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    throw std::runtime_error("failed to initialize SDL2!");
  }

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Create window with Vulkan graphics context
  const float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
  constexpr auto kWindowFlags = static_cast<SDL_WindowFlags>(
      SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ = SDL_CreateWindow("PolyStation", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             static_cast<int>(1280 * main_scale),
                             static_cast<int>(720 * main_scale), kWindowFlags);
  if (window_ == nullptr) {
    throw std::runtime_error("failed to create window!");
  }
}

void app::Application::InitVulkan() {
  CreateInstance();
  SetupDebugMessenger();
  CreateSurface();
  PickPhysicalDevice();
  CreateLogicalDevice();
  CreateDescriptorPool();
}

void app::Application::InitImGui() const {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& im_gui_io = ImGui::GetIO();
  (void)im_gui_io;
  im_gui_io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  im_gui_io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  im_gui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  const float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForVulkan(window_);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance_;
  init_info.PhysicalDevice = physical_device_;
  init_info.Device = device_;
  init_info.QueueFamily = queue_family_;
  init_info.Queue = queue_;
  init_info.PipelineCache = pipeline_cache_;
  init_info.DescriptorPool = descriptor_pool_;
  init_info.RenderPass = main_window_data_.RenderPass;
  init_info.Subpass = 0;
  init_info.MinImageCount = min_image_count_;
  init_info.ImageCount = main_window_data_.ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = CheckVkResult;
  ImGui_ImplVulkan_Init(&init_info);
}

void app::Application::MainLoop() {
  while (!done_) {
    HandleEvents();

    if ((SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) != 0U) {
      SDL_Delay(10);
      continue;
    }

    // Resize swap chain?
    int fb_width = 0;
    int fb_height = 0;
    SDL_GetWindowSize(window_, &fb_width, &fb_height);
    if (fb_width > 0 && fb_height > 0 &&
        (swap_chain_rebuild_ || main_window_data_.Width != fb_width ||
         main_window_data_.Height != fb_height)) {
      ImGui_ImplVulkan_SetMinImageCount(min_image_count_);
      ImGui_ImplVulkanH_CreateOrResizeWindow(
          instance_, physical_device_, device_, &main_window_data_,
          queue_family_, nullptr, fb_width, fb_height, min_image_count_);
      main_window_data_.FrameIndex = 0;
      swap_chain_rebuild_ = false;
    }

    // Emulator
    if (running_) {
      try {
        cpu_.Cycle();
      } catch (const std::exception& e) {
        std::snprintf(error_message_.data(), error_message_.size(), "%s",
                      std::format("CPU Exception: {}", e.what()).c_str());
        show_error_popup_ = true;
      }

      if (step_to_pc_ && cpu_.GetPC() == target_pc_) {
        running_ = false;
        step_to_pc_ = false;
      }
    }

    RenderFrame();
    PresentFrame();
  }

  // Wait for device to be idle before cleanup
  const VkResult err = vkDeviceWaitIdle(device_);
  CheckVkResult(err);
}

void app::Application::HandleEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) {
      done_ = true;
    }
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(window_)) {
      done_ = true;
    }
  }
}

void app::Application::RenderFrame() {
  constexpr auto kClearColor = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);

  // Start the Dear ImGui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  SetupDockingLayout();

  DrawCPUStateWindow();
  DrawControlWindow();
  DrawCpuDisassembler();
  DrawMainViewWindow();
  DrawErrorPopup();

  // Rendering
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();
  const bool is_minimized =
      (draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F);
  if (!is_minimized) {
    main_window_data_.ClearValue.color.float32[0] =
        kClearColor.x * kClearColor.w;
    main_window_data_.ClearValue.color.float32[1] =
        kClearColor.y * kClearColor.w;
    main_window_data_.ClearValue.color.float32[2] =
        kClearColor.z * kClearColor.w;
    main_window_data_.ClearValue.color.float32[3] = kClearColor.w;
    // Use the proper frame rendering method
    VkSemaphore image_acquired_semaphore =
        main_window_data_.FrameSemaphores[main_window_data_.SemaphoreIndex]
            .ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore =
        main_window_data_.FrameSemaphores[main_window_data_.SemaphoreIndex]
            .RenderCompleteSemaphore;
    VkResult err =
        vkAcquireNextImageKHR(device_, main_window_data_.Swapchain, UINT64_MAX,
                              image_acquired_semaphore, VK_NULL_HANDLE,
                              &main_window_data_.FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
      swap_chain_rebuild_ = true;
      return;
    }
    if (err != VK_SUCCESS) {
      CheckVkResult(err);
    }

    const ImGui_ImplVulkanH_Frame* h_frame =
        &main_window_data_.Frames[main_window_data_.FrameIndex];
    {
      err = vkWaitForFences(device_, 1, &h_frame->Fence, VK_TRUE, UINT64_MAX);
      CheckVkResult(err);

      err = vkResetFences(device_, 1, &h_frame->Fence);
      CheckVkResult(err);
    }
    {
      err = vkResetCommandPool(device_, h_frame->CommandPool, 0);
      CheckVkResult(err);
      VkCommandBufferBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      err = vkBeginCommandBuffer(h_frame->CommandBuffer, &info);
      CheckVkResult(err);
    }
    {
      VkRenderPassBeginInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      info.renderPass = main_window_data_.RenderPass;
      info.framebuffer = h_frame->Framebuffer;
      info.renderArea.extent.width = main_window_data_.Width;
      info.renderArea.extent.height = main_window_data_.Height;
      info.clearValueCount = 1;
      info.pClearValues = &main_window_data_.ClearValue;
      vkCmdBeginRenderPass(h_frame->CommandBuffer, &info,
                           VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, h_frame->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(h_frame->CommandBuffer);
    {
      constexpr VkPipelineStageFlags kWaitStage =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      VkSubmitInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      info.waitSemaphoreCount = 1;
      info.pWaitSemaphores = &image_acquired_semaphore;
      info.pWaitDstStageMask = &kWaitStage;
      info.commandBufferCount = 1;
      info.pCommandBuffers = &h_frame->CommandBuffer;
      info.signalSemaphoreCount = 1;
      info.pSignalSemaphores = &render_complete_semaphore;

      err = vkEndCommandBuffer(h_frame->CommandBuffer);
      CheckVkResult(err);
      err = vkQueueSubmit(queue_, 1, &info, h_frame->Fence);
      CheckVkResult(err);
    }
  }
}

void app::Application::PresentFrame() {
  if (swap_chain_rebuild_) {
    return;
  }
  VkSemaphore render_complete_semaphore =
      main_window_data_.FrameSemaphores[main_window_data_.SemaphoreIndex]
          .RenderCompleteSemaphore;
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &main_window_data_.Swapchain;
  info.pImageIndices = &main_window_data_.FrameIndex;
  VkResult const err = vkQueuePresentKHR(queue_, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    swap_chain_rebuild_ = true;
    return;
  }
  if (err != VK_SUCCESS) {
    CheckVkResult(err);
  }
  main_window_data_.SemaphoreIndex =
      (main_window_data_.SemaphoreIndex + 1) % main_window_data_.SemaphoreCount;
}

void app::Application::DrawCPUStateWindow() const {
  if (ImGui::Begin("PolyStation - CPU State")) {
    // Cycle Count
    ImGui::Text("Step Count: %llu", cpu_.GetStepCount());
    ImGui::Separator();
    // Program Counter - separate line
    ImGui::TextUnformatted("PC (Program Counter)");
    ImGui::SameLine(200);
    ImGui::TextColored(ImVec4(1.0F, 0.8F, 0.2F, 1.0F), "0x%08X",
                       cpu_.GetNextPC());
    ImGui::SameLine(320);
    ImGui::Text("(%u)", cpu_.GetNextPC());
    ImGui::Separator();
    // Register type selection
    static int register_type = 0;  // 0 = CPU registers, 1 = COP0 registers
    const char* register_types[] = {"CPU Registers", "COP0 Registers"};
    ImGui::Text("Register Set:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::Combo("##RegisterType", &register_type, register_types,
                 IM_ARRAYSIZE(register_types));
    ImGui::Separator();
    if (register_type == 0) {
      // CPU Registers
      ImGui::TextUnformatted("CPU Registers");
      ImGui::Separator();
      // Register names for MIPS/PlayStation CPU
      // Create table for registers in 8x4 grid
      if (ImGui::BeginTable("RegisterTable", 12,
                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        // Setup columns - 4 groups of (Name, Hex, Decimal)
        for (int group = 0; group < 4; group++) {
          ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed,
                                  80.0F);
          ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed,
                                  90.0F);
          ImGui::TableSetupColumn("Decimal", ImGuiTableColumnFlags_WidthFixed,
                                  100.0F);
        }
        // Header row
        ImGui::TableHeadersRow();
        // 8 rows of 4 registers each
        for (int row = 0; row < 8; row++) {
          ImGui::TableNextRow();
          for (int col = 0; col < 4; col++) {
            constexpr std::array kRegisterNames{
                "R0 (zero)", "R1 (at)",  "R2 (v0)",  "R3 (v1)",  "R4 (a0)",
                "R5 (a1)",   "R6 (a2)",  "R7 (a3)",  "R8 (t0)",  "R9 (t1)",
                "R10 (t2)",  "R11 (t3)", "R12 (t4)", "R13 (t5)", "R14 (t6)",
                "R15 (t7)",  "R16 (s0)", "R17 (s1)", "R18 (s2)", "R19 (s3)",
                "R20 (s4)",  "R21 (s5)", "R22 (s6)", "R23 (s7)", "R24 (t8)",
                "R25 (t9)",  "R26 (k0)", "R27 (k1)", "R28 (gp)", "R29 (sp)",
                "R30 (fp)",  "R31 (ra)"};
            int const reg_index = (row * 4) + col;
            DrawTableCell(gsl::at(kRegisterNames, reg_index),
                          cpu_.GetRegister(reg_index));
          }
        }

        // HI / LO
        ImGui::TableNextRow();
        DrawTableCell("HI", cpu_.GetHI());
        DrawTableCell("LO", cpu_.GetLO());

        ImGui::EndTable();
      }
    } else {
      // COP0 Registers
      ImGui::TextUnformatted("COP0 Registers");
      ImGui::Separator();
      if (ImGui::BeginTable("COP0RegisterTable", 3,
                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        // Setup columns
        ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed,
                                120.0F);
        ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed,
                                100.0F);
        ImGui::TableSetupColumn("Decimal", ImGuiTableColumnFlags_WidthFixed,
                                100.0F);
        // Header row
        ImGui::TableHeadersRow();
        // Status Register (Register 12)
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("R12 (Status)");
        ImGui::TableNextColumn();
        const uint32_t status_value = cpu_.GetCop0().GetStatusRegister();
        if (status_value != 0) {
          ImGui::TextColored(ImVec4(0.2F, 1.0F, 0.2F, 1.0F), "0x%08X",
                             status_value);
        } else {
          ImGui::Text("0x%08X", status_value);
        }
        ImGui::TableNextColumn();
        if (status_value != 0) {
          ImGui::TextColored(ImVec4(0.2F, 1.0F, 0.2F, 1.0F), "%u",
                             status_value);
        } else {
          ImGui::Text("%u", status_value);
        }
        // // Add more COP0 registers here as they are implemented
        // // For now, we'll show placeholders for common COP0 registers
        // for (int i = 0; i < 16; i++) {
        //   constexpr std::array kCop0RegisterNames = {
        //       "R0 (Index)",    "R1 (Random)",   "R2 (EntryLo0)",
        //       "R3 (EntryLo1)", "R4 (Context)",  "R5 (PageMask)",
        //       "R6 (Wired)",    "R7 (Reserved)", "R8 (BadVAddr)",
        //       "R9 (Count)",    "R10 (EntryHi)", "R11 (Compare)",
        //       "R13 (Cause)",   "R14 (EPC)",     "R15 (PRId)"};
        //
        //   if (i == 12) {
        //     continue;  // Skip status register as it's already shown
        //   }
        //
        //   ImGui::TableNextRow();
        //   ImGui::TableNextColumn();
        //   ImGui::Text("%s", gsl::at(kCop0RegisterNames, i));
        //   ImGui::TableNextColumn();
        //   ImGui::TextColored(ImVec4(0.5F, 0.5F, 0.5F, 1.0F), "0x00000000");
        //   ImGui::TableNextColumn();
        //   ImGui::TextColored(ImVec4(0.5F, 0.5F, 0.5F, 1.0F), "0");
        // }
        ImGui::EndTable();
      }
    }
    ImGui::Separator();
  }
  ImGui::End();
}

void app::Application::DrawControlWindow() {
  if (ImGui::Begin("PolyStation - Controls")) {
    float const available_width = ImGui::GetContentRegionAvail().x;

    if (const auto* control_btn_label = !running_ ? "Run" : "Pause";
        ImGui::Button(control_btn_label, ImVec2(available_width, 0.0))) {
      running_ = !running_;
    }

    ImGui::Text("Status: %s", running_ ? "Running" : "Paused");

    ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

    ImGui::Separator();

    if (ImGui::Button("Step one", ImVec2(available_width, 0.0)) && !running_) {
      try {
        cpu_.Cycle();
      } catch (const std::exception& e) {
        std::snprintf(error_message_.data(), error_message_.size(), "%s",
                      std::format("CPU Exception: {}", e.what()).c_str());
        show_error_popup_ = true;
      }
    }

    ImGui::Separator();

    constexpr uint32_t kProgramCounterStep = 4;
    constexpr uint32_t kProgramCounterFastStep = 400;
    ImGui::InputScalar("Target", ImGuiDataType_U32, &target_pc_,
                       &kProgramCounterStep, &kProgramCounterFastStep, "%08X");

    if (ImGui::Button("Step to PC", ImVec2(available_width, 0.0)) &&
        !running_) {
      running_ = true;
      step_to_pc_ = true;
    }

    ImGui::Separator();

    if (ImGui::Button("Reset", ImVec2(available_width, 0.0)) && !running_) {
      cpu_.Reset();
      target_pc_ = bios::kBiosBase;
    }
  }
  ImGui::End();
}

void app::Application::DrawCpuDisassembler() const {
  constexpr uint32_t kMaxInstructions = 25;

  const uint32_t current_pc = cpu_.GetPC();

  ImGui::Begin("PolyStation - CPU Disassembler");

  uint32_t start_pc = current_pc;
  if (constexpr uint32_t kInstructionsBefore = 5;
      current_pc >= kInstructionsBefore * 4) {
    start_pc = current_pc - (kInstructionsBefore * 4);
  } else {
    start_pc = 0;
  }

  start_pc = (start_pc / 4) * 4;

  for (uint32_t pc = start_pc; pc < start_pc + (kMaxInstructions * 4);
       pc += 4) {
    bool valid_address = false;

    if (pc >= bios::kBiosBase && pc < bios::kBiosBase + bios::kBiosSize) {
      valid_address = true;
    } else if (bus::kRamMemoryRange.InRange(pc)) {
      valid_address = true;
    }
    if (!valid_address) {
      continue;
    }

    uint32_t instruction_data = 0;
    try {
      instruction_data = cpu_.Load32(pc);
    } catch (const std::exception&) {
      continue;
    }

    const cpu::Instruction instruction(instruction_data);

    if (pc == current_pc) {
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
      const float line_height = ImGui::GetTextLineHeight();
      const std::string full_text =
          std::format("0x{:08X}  {}", pc, instruction.ToString());
      const ImVec2 text_size = ImGui::CalcTextSize(full_text.c_str());
      const auto rect_min = ImVec2(cursor_pos.x - 2.0f, cursor_pos.y);
      const auto rect_max = ImVec2(cursor_pos.x + text_size.x + 15.0f,
                                   cursor_pos.y + line_height);
      const ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.2f, 0.4f, 0.8f, 0.4f));
      draw_list->AddRectFilled(rect_min, rect_max, bg_color);
    }

    ImGui::Text("0x%08X", pc);
    ImGui::SameLine();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const float line_height = ImGui::GetTextLineHeight();
    const auto line_start = ImVec2(cursor_pos.x, cursor_pos.y);
    const auto line_end = ImVec2(cursor_pos.x, cursor_pos.y + line_height);
    const ImU32 separator_color = ImGui::GetColorU32(ImGuiCol_Separator);
    draw_list->AddLine(line_start, line_end, separator_color, 1.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::SameLine();

    if (pc >= bios::kBiosBase && pc < bios::kBiosBase + bios::kBiosSize) {
      ImGui::TextColored(ImVec4(0.8F, 0.8F, 0.2F, 1.0F), "[BIOS] ");
    } else if (bus::kRamMemoryRange.InRange(pc)) {
      ImGui::TextColored(ImVec4(0.2F, 0.8F, 0.2F, 1.0F), "[RAM]  ");
    }
    ImGui::SameLine();

    ImGui::Text("%s", instruction.ToString().c_str());
  }
  ImGui::End();
}

void app::Application::DrawErrorPopup() {
  if (show_error_popup_) {
    ImGui::OpenPopup("CPU Error");
    running_ = false;
    show_error_popup_ = false;
  }

  // Center the popup on the screen
  const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5F, 0.5F));
  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Once);

  if (ImGui::BeginPopupModal("CPU Error", nullptr, ImGuiWindowFlags_NoResize)) {
    // Error icon and text
    ImGui::TextColored(ImVec4(1.0F, 0.3F, 0.3F, 1.0F), "⚠");
    ImGui::SameLine();
    ImGui::TextUnformatted("An error occurred in the CPU emulation:");
    ImGui::Separator();
    // Error message in a text box (selectable for copying)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2F, 0.2F, 0.2F, 1.0F));
    ImGui::InputTextMultiline("##error_text", error_message_.data(),
                              error_message_.size(), ImVec2(-1, 80),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    ImGui::Separator();
    // Buttons
    constexpr float kButtonWidth = 100.0F;
    constexpr float kButtonSpacing = 10.0F;
    constexpr float kTotalWidth = (kButtonWidth * 3) + (kButtonSpacing * 2);
    const float start_x = (ImGui::GetWindowWidth() - kTotalWidth) * 0.5F;
    ImGui::SetCursorPosX(start_x);
    ImGui::SameLine(0, kButtonSpacing);
    if (ImGui::Button("Reset CPU", ImVec2(kButtonWidth, 0))) {
      cpu_.Reset();
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0, kButtonSpacing);
    if (ImGui::Button("Exit", ImVec2(kButtonWidth, 0))) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void app::Application::DrawMainViewWindow() {
  if (ImGui::Begin("PolyStation - Display")) {
  }
  ImGui::End();
}

void app::Application::SetupDockingLayout() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
  ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  if (ImGuiIO const& im_gui_io = ImGui::GetIO();
      im_gui_io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID const dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_None);

    static bool first_time = true;
    if (first_time) {
      first_time = false;
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

      ImGuiID dock_top = 0;
      ImGuiID dock_bottom = 0;
      ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.60F, &dock_top,
                                  &dock_bottom);

      ImGuiID dock_top_left = 0;
      ImGuiID dock_top_right = 0;
      ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.15F,
                                  &dock_top_left, &dock_top_right);

      ImGuiID dock_top_center = 0;
      ImGui::DockBuilderSplitNode(dock_top_right, ImGuiDir_Right, 0.23F,
                                  &dock_top_right, &dock_top_center);

      ImGui::DockBuilderDockWindow("PolyStation - Controls", dock_top_left);
      ImGui::DockBuilderDockWindow("PolyStation - CPU State", dock_bottom);
      ImGui::DockBuilderDockWindow("PolyStation - Display", dock_top_center);
      ImGui::DockBuilderDockWindow("PolyStation - CPU Disassembler",
                                   dock_top_right);

      ImGui::DockBuilderFinish(dockspace_id);
    }
  }

  ImGui::End();
}

void app::Application::DrawTableCell(const char* reg_name,
                                     const uint32_t reg_value) {
  // Register name
  ImGui::TableNextColumn();
  ImGui::Text("%s", reg_name);
  // Hex value
  ImGui::TableNextColumn();
  if (reg_value != 0) {
    ImGui::TextColored(ImVec4(0.2F, 1.0F, 0.2F, 1.0F), "0x%08X", reg_value);
  } else {
    ImGui::Text("0x%08X", reg_value);
  }
  // Decimal value
  ImGui::TableNextColumn();
  if (reg_value != 0) {
    ImGui::TextColored(ImVec4(0.2F, 1.0F, 0.2F, 1.0F), "%u", reg_value);
  } else {
    ImGui::Text("%u", reg_value);
  }
}

void app::Application::Cleanup() {
  if (device_ != VK_NULL_HANDLE) {
    // Wait for device to be idle before cleanup
    vkDeviceWaitIdle(device_);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    ImGui_ImplVulkanH_DestroyWindow(instance_, device_, &main_window_data_,
                                    nullptr);

    if (descriptor_pool_ != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
      descriptor_pool_ = VK_NULL_HANDLE;
    }

    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  if constexpr (kEnableValidationLayers) {
    if (debug_messenger_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
      DestroyDebugMessengerEXT(instance_, debug_messenger_, nullptr);
      debug_messenger_ = VK_NULL_HANDLE;
    }
  }

  if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }

  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  SDL_Quit();
}

void app::Application::CreateInstance() {
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "PolyStation";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  // Enumerate available extensions
  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         available_extensions.data());

  // Enable required extensions
  std::vector<const char*> required_extensions = GetRequiredExtensions();
  if (IsExtensionAvailable(
          available_extensions,
          VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
    required_extensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
  if (IsExtensionAvailable(available_extensions,
                           VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    required_extensions.push_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  // Enabling validation layers
  if constexpr (kEnableValidationLayers) {
    constexpr std::array kLayers = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = kLayers.data();
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  // Create Vulkan Instance
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(required_extensions.size());
  create_info.ppEnabledExtensionNames = required_extensions.data();

  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

void app::Application::SetupDebugMessenger() {
  if constexpr (!kEnableValidationLayers) {
    return;
  }

  VkDebugUtilsMessengerCreateInfoEXT create_info;
  PopulateDebugMessengerCreateInfo(create_info);

  if (CreateDebugMessengerEXT(instance_, &create_info, nullptr,
                              &debug_messenger_) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void app::Application::CreateSurface() {
  if (SDL_Vulkan_CreateSurface(window_, instance_, &surface_) == 0) {
    throw std::runtime_error("failed to create window surface!");
  }
}

void app::Application::PickPhysicalDevice() {
  physical_device_ = ImGui_ImplVulkanH_SelectPhysicalDevice(instance_);
  IM_ASSERT(physical_device_ != VK_NULL_HANDLE);
}

void app::Application::CreateLogicalDevice() {
  // Select graphics queue family
  queue_family_ = ImGui_ImplVulkanH_SelectQueueFamilyIndex(physical_device_);
  IM_ASSERT(std::cmp_not_equal(queue_family_, -1));

  ImVector<const char*> device_extensions;
  device_extensions.push_back("VK_KHR_swapchain");

  // Enumerate physical device extension
  uint32_t properties_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr,
                                       &properties_count, nullptr);

  std::vector<VkExtensionProperties> properties(properties_count);
  vkEnumerateDeviceExtensionProperties(physical_device_, nullptr,
                                       &properties_count, properties.data());

#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
  if (IsExtensionAvailable(properties,
                           VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

  constexpr std::array kQueuePriority = {1.0F};
  std::array<VkDeviceQueueCreateInfo, 1> queue_info{};
  queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].queueFamilyIndex = queue_family_;
  queue_info[0].queueCount = 1;
  queue_info[0].pQueuePriorities = kQueuePriority.data();

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
  create_info.pQueueCreateInfos = queue_info.data();
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions.Size);
  create_info.ppEnabledExtensionNames = device_extensions.Data;
  if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
}

void app::Application::CreateDescriptorPool() {
  constexpr std::array<VkDescriptorPoolSize, 1> kPoolSizes{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
  };

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 0;
  for (const auto& [_, descriptorCount] : kPoolSizes) {
    pool_info.maxSets += descriptorCount;
  }
  pool_info.poolSizeCount = static_cast<uint32_t>(kPoolSizes.size());
  pool_info.pPoolSizes = kPoolSizes.data();
  if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void app::Application::SetupVulkanWindow() {
  main_window_data_.Surface = surface_;

  // Check for WSI support
  VkBool32 res = 0;
  vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_,
                                       main_window_data_.Surface, &res);
  if (res != VK_TRUE) {
    throw std::runtime_error("Error no WSI support on physical device");
  }

  // Select Surface Format
  constexpr std::array kRequestSurfaceImageFormat = {
      VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
  constexpr VkColorSpaceKHR kRequestSurfaceColorSpace =
      VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  main_window_data_.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      physical_device_, main_window_data_.Surface,
      kRequestSurfaceImageFormat.data(), kRequestSurfaceImageFormat.size(),
      kRequestSurfaceColorSpace);

  // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
  std::array present_modes = {VK_PRESENT_MODE_MAILBOX_KHR,
                              VK_PRESENT_MODE_IMMEDIATE_KHR,
                              VK_PRESENT_MODE_FIFO_KHR};
#else
  std::array present_modes = {VK_PRESENT_MODE_FIFO_KHR};
#endif
  main_window_data_.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
      physical_device_, main_window_data_.Surface, present_modes.data(),
      present_modes.size());

  // Create SwapChain, RenderPass, Framebuffer, etc.
  IM_ASSERT(min_image_count_ >= 2);
  int width = 0;
  int height = 0;
  SDL_GetWindowSize(window_, &width, &height);
  ImGui_ImplVulkanH_CreateOrResizeWindow(
      instance_, physical_device_, device_, &main_window_data_, queue_family_,
      nullptr, width, height, min_image_count_);
}

std::vector<const char*> app::Application::GetRequiredExtensions() const {
  uint32_t sdl_extensions_count = 0;
  SDL_Vulkan_GetInstanceExtensions(window_, &sdl_extensions_count, nullptr);

  std::vector<const char*> sdl_extensions(sdl_extensions_count);
  SDL_Vulkan_GetInstanceExtensions(window_, &sdl_extensions_count,
                                   sdl_extensions.data());

  return sdl_extensions;
}

bool app::Application::IsExtensionAvailable(
    const std::vector<VkExtensionProperties>& available_extensions,
    const char* extension) {
  for (const auto& [extensionName, _] : available_extensions) {
    if (strcmp(extensionName, extension) == 0) {
      return true;
    }
  }
  return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL app::Application::DebugCallback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    [[maybe_unused]] void* p_user_data) {
  std::cerr << "validation layer: " << p_callback_data->pMessage << '\n';
  return VK_FALSE;
}

void app::Application::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& create_info) {
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = DebugCallback;
}