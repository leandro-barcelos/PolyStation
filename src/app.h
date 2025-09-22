#ifndef POLYSTATION_APP_H
#define POLYSTATION_APP_H
#include <SDL.h>
#include <SDL_vulkan.h>

#include <cstdio>
#include <vector>

#include "cpu.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#define APP_USE_UNLIMITED_FRAME_RATE

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
constexpr bool kEnableSimplifiedUI = true;
#else
constexpr bool kEnableValidationLayers = true;
constexpr bool kEnableSimplifiedUI = false;
#endif

VkResult CreateDebugMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger);

void DestroyDebugMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debug_messenger,
                              const VkAllocationCallbacks* p_allocator);

void CheckVkResult(VkResult err);

namespace app {
class Application {
 public:
  explicit Application(const std::string& bios_path) : cpu_(bios_path) {}

  void Run();

 private:
  cpu::CPU cpu_;
  bool running_ = false;

  bool done_ = false;
  SDL_Window* window_ = nullptr;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  uint32_t queue_family_ = static_cast<uint32_t>(-1);
  VkQueue queue_ = VK_NULL_HANDLE;
  VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  ImGui_ImplVulkanH_Window main_window_data_;
  uint32_t min_image_count_ = 2;
  bool swap_chain_rebuild_ = false;

  // Error window
  bool show_error_popup_ = false;
  std::array<char, 1024> error_message_{};

  bool step_to_pc_ = false;
  uint32_t target_pc_ = bios::kBiosBase;

  void InitSDL();
  void InitVulkan();
  void InitImGui() const;
  void MainLoop();
  void Cleanup();

  void CreateInstance();
  void SetupDebugMessenger();
  void CreateSurface();
  void PickPhysicalDevice();
  void CreateLogicalDevice();
  void CreateDescriptorPool();
  void SetupVulkanWindow();

  void HandleEvents();
  void RenderFrame();
  void PresentFrame();
  void DrawCPUStateWindow() const;
  void DrawControlWindow();
  void DrawCpuDisassembler() const;
  void DrawErrorPopup();
  static void DrawMainViewWindow();
  static void SetupDockingLayout();
  static void DrawTableCell(const char* reg_name, uint32_t reg_value);
  void DrawSimplifiedUI();

  [[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;
  static bool IsExtensionAvailable(
      const std::vector<VkExtensionProperties>& available_extensions,
      const char* extension);
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                void* p_user_data);
  static void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& create_info);
};
}  // namespace app

#endif  // POLYSTATION_APP_H