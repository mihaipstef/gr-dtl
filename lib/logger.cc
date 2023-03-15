/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include <gnuradio/dtl/log.h>
#include <functional>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace gr {
namespace dtl {

#if DTL_LOGGING_ENABLE

std::shared_ptr<spdlog::sinks::dist_sink_mt> dtl_logging_backend()
{
    static std::shared_ptr<spdlog::sinks::dist_sink_mt> backend =
        std::make_shared<spdlog::sinks::dist_sink_mt>();
    backend->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
    return backend;
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

#endif

} // namespace dtl
} // namespace gr