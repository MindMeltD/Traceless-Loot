#pragma once

#ifdef ENABLE_LOGGING
    #define LOG_TRACE(...) logger::trace(__VA_ARGS__)
    #define LOG_DEBUG(...) logger::debug(__VA_ARGS__)
    #define LOG_INFO(...) logger::info(__VA_ARGS__)
    #define LOG_WARN(...) logger::warn(__VA_ARGS__)
#else
    #define LOG_TRACE(...) ((void)0)
    #define LOG_DEBUG(...) ((void)0)
    #define LOG_INFO(...) ((void)0)
    #define LOG_WARN(...) ((void)0)
#endif  // ENABLE_LOGGING

#define LOG_ERROR(...) logger::error(__VA_ARGS__)
#define LOG_CRITICAL(...) logger::critical(__VA_ARGS__)



// This is a snippet you can put at the top of all of your SKSE plugins!

#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}

// Then just call SetupLog() in your SKSE plugin initialization
//
// ^---- don't forget to do this or your logs won't work :)