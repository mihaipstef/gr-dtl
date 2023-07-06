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
#define DTL_LOG_ERROR(msg, ...) _logger.logger->error(msg VA_ARGS(__VA_ARGS__));

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

// inline void _append_buf_to_stream(std::stringstream& ss, unsigned char* buf, int len)
// {
//     for (int i = 0; i < len; ++i) {
//         ss << "," << std::setfill('0') << std::setw(2) << std::hex << (int)buf[i];
//     }
// }

template<typename T>
inline void format(std::stringstream& ss)
{
    throw std::runtime_error("not implemented");
}


template<>
inline void format<float>(std::stringstream& ss)
{
    ss << std::setw(11) << std::setprecision(6);// << std::setfill('');
}

template<>
inline void format<const float>(std::stringstream& ss)
{
    ss << std::setw(11) << std::setprecision(6);// << std::setfill('');
}

template<>
inline void format<unsigned char>(std::stringstream& ss)
{
    ss << std::setw(2) << std::hex;
}

template<>
inline void format<const unsigned char>(std::stringstream& ss)
{
    ss << std::setw(2) << std::hex;
}

// template<>
// inline void format<const uint8_t>(std::stringstream& ss)
// {
//     ss << std::setfill('0') << std::setw(2) << std::hex;
// }

// template<>
// inline void format<uint8_t>(std::stringstream& ss)
// {
//     ss << std::setfill('0') << std::setw(2) << std::hex;
// }


template<typename T>
inline void _append_vec_to_stream(std::stringstream& ss, std::vector<T>& vec)
{
    ss << "size=" << vec.size();
    format<T>(ss);
    for (auto& v: vec) {
        if (sizeof(T) == 1) {
            ss << "," << (int)v;
        } else {
            ss << "," << v;
        }
    }
}

template<typename T>
inline void _append_buf_to_stream(std::stringstream& ss, T* buf, int len)
{
    format<T>(ss);
    for (int i=0; i<len; ++i) {
        if (sizeof(T) == 1) {
            ss << "," << (int)buf[i];
        } else {
            ss << "," << buf[i];
        }
    }
}

#define DTL_LOG_BYTES(msg, buffer, length)              \
    {                                                   \
        std::stringstream ss;                           \
        _append_buf_to_stream(ss, buffer, length);      \
        _logger.logger->debug("{}: {}", msg, ss.str()); \
    }

#define DTL_LOG_BUFFER(msg, buffer, length)              \
    {                                                   \
        std::stringstream ss;                           \
        _append_buf_to_stream(ss, buffer, length);      \
        _logger.logger->debug("{}: {}", msg, ss.str()); \
    }

#define DTL_LOG_VEC(msg, vec)                           \
    {                                                   \
        std::stringstream ss;                           \
        _append_vec_to_stream(ss, vec);                 \
        _logger.logger->debug("{}: {}", msg, ss.str()); \
    }


} // namespace dtl
} // namespace gr

#else
#define INIT_DTL_LOGGER(name)
#define DTL_LOG_INFO(msg, ...)
#define DTL_LOG_DEBUG(msg, ...)
#define DTL_LOG_ERROR(msg, ...)
#define DTL_LOG_TAGS(title, tags)
#define DTL_LOG_BYTES(msg, buffer, length)
#define DTL_LOG_VEC(msg, vec)
#define DTL_LOG_BUFFER(msg, buffer, length)
#endif

#endif /* INCLUDED_DTL_LOGGER_H */
