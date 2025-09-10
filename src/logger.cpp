#include "logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> logger::Logger::logger_;

void logger::Logger::init() {
  const auto console_sink =
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "polystation.log", true);

  console_sink->set_level(spdlog::level::debug);
  console_sink->set_pattern("%^[%l] %v%$");

  file_sink->set_level(spdlog::level::trace);
  file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

  const spdlog::sinks_init_list sink_list = {console_sink, file_sink};
  logger_ = std::make_shared<spdlog::logger>("polystation", sink_list.begin(),
                                             sink_list.end());
  logger_->set_level(spdlog::level::trace);
  logger_->flush_on(spdlog::level::warn);

  spdlog::register_logger(logger_);
  spdlog::set_default_logger(logger_);

  LOG_INFO_CORE("PolyStation logging initialized");
}

void logger::Logger::shutdown() {
  if (logger_) {
    LOG_INFO_CORE("Shutting down logging");
    logger_->flush();
    spdlog::shutdown();
    logger_.reset();
  }
}