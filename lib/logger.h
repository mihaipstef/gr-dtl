/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_LOGGER_H
#define INCLUDED_DTL_LOGGER_H

#include <gnuradio/logger.h>

namespace gr {
namespace dtl {

#if true

    #define INIT_DTL_LOGGER(name) static gr::logger _logger(name);

    #define DTL_LOG_INFO(msg, ...) _logger.info(msg, __VA_ARGS__);
    #define DTL_LOG_DEBUG(msg, ...) _logger.debug(msg, __VA_ARGS__);

    #define DTL_LOG_TAGS(title, tags) \
        _logger.debug(title); \
        for (auto& t: tags) { \
            if(pmt::is_integer(t.value)) { \
                _logger.debug("k:{}, v:{}, offset:{}", pmt::symbol_to_string(t.key), pmt::to_long(t.value), t.offset); \
            } \
            else { \
                _logger.debug("k:{}, offset:{}", pmt::symbol_to_string(t.key), t.offset); \
            } \
        }

    #define DTL_LOG_BYTES(msg, buffer, length) \
        { \
            std::stringstream ss; \
            for(int i=0; i<length; ++i) { \
                ss << "," << std::hex << buffer[i]; \
            } \
            _logger.debug(ss.str()); \
        }

#else
    #define INIT_DTL_LOGGER(name)
    #define DTL_LOG_INFO(msg, ...)
    #define DTL_LOG_DEBUG(msg, ...)
    #define DTL_LOG_TAGS(title, tags)
    #define DTL_LOG_BYTES(msg, buffer, length)
#endif

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_LOGGER_H */
