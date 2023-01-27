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
    #include <iomanip>
    #include <memory>
    #include <tuple>

    namespace gr {
    namespace dtl {

        void register_logger(const std::shared_ptr<gr::logger>& logger);

        struct dtl_logger_wrapper {
            std::shared_ptr<gr::logger> gr_logger;
            dtl_logger_wrapper(const std::string& name) {
                gr_logger = std::make_shared<gr::logger>(name);
                register_logger(
                    gr_logger
                );
            }
        };

        #define INIT_DTL_LOGGER(name) \
            static dtl_logger_wrapper _logger(name);

        // HACK: This is not standard.
        #define VA_ARGS(...) ,##__VA_ARGS__

        #define DTL_LOG_INFO(msg, ...) _logger.gr_logger->info(msg VA_ARGS(__VA_ARGS__));
        #define DTL_LOG_DEBUG(msg, ...)  _logger.gr_logger->debug(msg VA_ARGS(__VA_ARGS__));

        #define DTL_LOG_TAGS(title, tags) \
            _logger.gr_logger->debug(title); \
            for (auto& t: tags) { \
                if(pmt::is_integer(t.value)) { \
                    _logger.gr_logger->debug("k:{}, v:{}, offset:{}", pmt::symbol_to_string(t.key), pmt::to_long(t.value), t.offset); \
                } \
                else { \
                    _logger.gr_logger->debug("k:{}, offset:{}", pmt::symbol_to_string(t.key), t.offset); \
                } \
            }

        #define DTL_LOG_BYTES(msg, buffer, length) \
            { \
                std::stringstream ss; \
                for(int i=0; i<length; ++i) { \
                    ss << "," << std::setfill('0') << std::setw(2) << std::hex << (int)buffer[i]; \
                } \
                _logger.gr_logger->debug("{}", ss.str()); \
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
