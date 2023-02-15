/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_CRC_UTL_H
#define INCLUDED_DTL_CRC_UTL_H

#include <gnuradio/digital/crc.h>

namespace gr {
namespace dtl {

class crc_util
{

private:
    gr::digital::crc d_crc;
    std::size_t d_crc_len;
    unsigned long d_count;
    unsigned long d_failed;


public:
    crc_util(std::size_t len,
             unsigned long poly,
             unsigned long initial_value,
             unsigned long final_xor);

    bool verify_crc(unsigned char* buffer, std::size_t len);

    unsigned long append_crc(unsigned char* buffer, std::size_t len);

    void reset_monitoring();
    std::size_t get_failed() const;
    std::size_t get_success() const;
    std::size_t get_crc_len() const;
};

} // namespace dtl
} // namespace gr

#endif // INCLUDED_DTL_CRC_UTL_H