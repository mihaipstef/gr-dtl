/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include <gnuradio/dtl/log.h>
#include <functional>
#include <stdexcept>

namespace gr {
namespace dtl {

#if DTL_LOGGING_ENABLE

dtl_logging_backend_wrapper::dtl_logging_backend_wrapper()
    : backend(std::make_shared<spdlog::sinks::dist_sink_mt>())
{
    backend->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
}


dtl_logger_wrapper::dtl_logger_wrapper(const std::string& name)
{
    logger = std::make_shared<spdlog::logger>(name, dtl_logging_backend());
    logger->set_level(logging::singleton().default_level());
    logger->set_pattern("%D %H:%M:%S.%f %P %t %n:%v");
    register_logger(logger);
}

std::shared_ptr<spdlog::sinks::dist_sink_mt> dtl_logging_backend()
{
    static dtl_logging_backend_wrapper backend;
    return backend.backend;
}

std::vector<std::shared_ptr<gr::logger>>& dtl_loggers()
{
    static std::vector<std::shared_ptr<gr::logger>> _dtl_loggers;
    return _dtl_loggers;
}

void register_logger(const std::shared_ptr<gr::logger>& logger)
{
    dtl_loggers().push_back(logger);
}

void set_dtl_log_level(const std::string& level)
{
    for (auto& logger : dtl_loggers()) {
        logger->set_level(level);
    }
}

#else

void set_dtl_log_level(const std::string& level)
{
    throw std::runtime_error("Not implemented");
}

#endif

} // namespace dtl
} // namespace gr