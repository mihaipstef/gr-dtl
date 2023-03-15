/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_LOGGER_H
#define INCLUDED_DTL_LOGGER_H


#if DTL_LOGGING_ENABLE

#include <gnuradio/logger.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iomanip>
#include <memory>
#include <tuple>


namespace gr {
namespace dtl {

std::shared_ptr<spdlog::sinks::dist_sink_mt> dtl_logging_backend();

void register_logger(const std::shared_ptr<gr::logger>& logger);

struct dtl_logging_backend_wrapper {
    std::shared_ptr<spdlog::sinks::dist_sink_mt> backend;
    dtl_logging_backend_wrapper()
        : backend(std::make_shared<spdlog::sinks::dist_sink_mt>())
    {
        backend->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
    }
};

struct dtl_logger_wrapper {
    std::shared_ptr<spdlog::logger> logger;
    dtl_logger_wrapper(const std::string& name)
    {
        logger = std::make_shared<spdlog::logger>(name, dtl_logging_backend());
        logger->set_level(logging::singleton().default_level());
        logger->set_pattern("%D %H:%M:%S.%f %n:%v");
        register_logger(logger);
    }
};

#define INIT_DTL_LOGGER(name) static dtl_logger_wrapper _logger(name);

// HACK: This is not standard.
#define VA_ARGS(...) , ##__VA_ARGS__

#define DTL_LOG_INFO(msg, ...) _logger.logger->info(msg VA_ARGS(__VA_ARGS__));
#define DTL_LOG_DEBUG(msg, ...) _logger.logger->debug(msg VA_ARGS(__VA_ARGS__));

#define DTL_LOG_TAGS(title, tags)                                           \
    _logger.logger->debug(title);                                           \
    for (auto& t : tags) {                                                  \
        if (pmt::is_integer(t.value)) {                                     \
            _logger.logger->debug("k:{}, v:{}, offset:{}",                  \
                                  pmt::symbol_to_string(t.key),             \
                                  pmt::to_long(t.value),                    \
                                  t.offset);                                \
        } else {                                                            \
            _logger.logger->debug(                                          \
                "k:{}, offset:{}", pmt::symbol_to_string(t.key), t.offset); \
        }                                                                   \
    }

#define DTL_LOG_BYTES(msg, buffer, length)                             \
    {                                                                  \
        std::stringstream ss;                                          \
        for (int i = 0; i < length; ++i) {                             \
            ss << "," << std::setfill('0') << std::setw(2) << std::hex \
               << (int)buffer[i];                                      \
        }                                                              \
        _logger.logger->debug("{}: {}", msg, ss.str());                \
    }

} // namespace dtl
} // namespace gr

#else
#define INIT_DTL_LOGGER(name)
#define DTL_LOG_INFO(msg, ...)
#define DTL_LOG_DEBUG(msg, ...)
#define DTL_LOG_TAGS(title, tags)
#define DTL_LOG_BYTES(msg, buffer, length)
#endif

#endif /* INCLUDED_DTL_LOGGER_H */
