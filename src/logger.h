#ifndef POLYSTATION_LOGGER_H
#define POLYSTATION_LOGGER_H

#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace logger {

class Logger {
 public:
  static void init();
  static void shutdown();
  static std::shared_ptr<spdlog::logger> get() { return logger_; }

 private:
  static std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros for your emulator components
#define LOG_INFO_BUS(...) \
  SPDLOG_LOGGER_INFO(logger::Logger::get(), "[BUS] " __VA_ARGS__)

#define LOG_INFO_CPU(...) \
  SPDLOG_LOGGER_INFO(logger::Logger::get(), "[CPU] " __VA_ARGS__)

#define LOG_INFO_CORE(...) \
  SPDLOG_LOGGER_INFO(logger::Logger::get(), "[CORE] " __VA_ARGS__)
#define LOG_ERROR_CORE(...) \
  SPDLOG_LOGGER_ERROR(logger::Logger::get(), "[CORE] " __VA_ARGS__)
#define LOG_FATAL_CORE(...) \
  SPDLOG_LOGGER_CRITICAL(logger::Logger::get(), "[CORE] " __VA_ARGS__)

#define LOG_TRACE_VULKAN(...) \
  SPDLOG_LOGGER_TRACE(logger::Logger::get(), "[Vulkan] " __VA_ARGS__)
#define LOG_INFO_VULKAN(...) \
  SPDLOG_LOGGER_INFO(logger::Logger::get(), "[Vulkan] " __VA_ARGS__)
#define LOG_WARN_VULKAN(...) \
  SPDLOG_LOGGER_WARN(logger::Logger::get(), "[Vulkan] " __VA_ARGS__)
#define LOG_ERROR_VULKAN(...) \
  SPDLOG_LOGGER_ERROR(logger::Logger::get(), "[Vulkan] " __VA_ARGS__)

}  // namespace logger

#endif  // POLYSTATION_LOGGER_H
