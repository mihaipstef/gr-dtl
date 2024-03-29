/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "crc_util.h"
#include <limits>
#include <gnuradio/testbed/logger.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("crc_util");

using namespace std;

crc_util::crc_util(size_t len,
                   unsigned long poly,
                   unsigned long initial_value,
                   unsigned long final_xor)
    : d_crc(len * 8, poly, initial_value, final_xor, true, true),
      d_crc_len(len),
      d_count(0),
      d_failed(0)
{
}


unsigned long crc_util::append_crc(unsigned char* buffer, std::size_t len)
{
    unsigned long crc_val = d_crc.compute(buffer, len);
    for (unsigned i = 0; i < d_crc_len; ++i) {
        buffer[len + i] = (crc_val >> (i * 8)) & 0xFF;
    }
    DTL_LOG_BUFFER("crc_append=", &buffer[len], d_crc_len);
    return crc_val;
}

bool crc_util::verify_crc(unsigned char* buffer, std::size_t len)
{
    size_t payload_len = len - d_crc_len;
    unsigned long crc_val = d_crc.compute(buffer, payload_len);
    DTL_LOG_BUFFER("crc_rcvd=", &buffer[payload_len], d_crc_len);
    bool crc_ok = true;
    for (unsigned i = 0; i < d_crc_len; ++i) {
        if (buffer[payload_len + i] != ((crc_val >> (i * 8)) & 0xFF)) {
            crc_ok = false;
            //break;
        }
    }
    ++d_count;
    if (d_count == 0) {
        d_failed = 0;
    } else if (!crc_ok) {
        ++d_failed;
    }
    return crc_ok;
}


void crc_util::reset_monitoring()
{
    d_count = 0;
    d_failed = 0;
}

size_t crc_util::get_failed() const
{
    return d_failed;
}


size_t crc_util::get_success() const
{
    return d_count - d_failed;
}

double crc_util::get_fail_rate() const
{
    return 100.0*d_failed/d_count;
}

size_t crc_util::get_crc_len() const { return d_crc_len; }

} // namespace dtl
} // namespace gr