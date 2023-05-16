/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_TB_DECODER_H
#define INCLUDED_DTL_TB_DECODER_H

#include <gnuradio/dtl/fec.h>


namespace gr {
namespace dtl {

class tb_decoder
{
private:
    std::vector<std::vector<unsigned char>> d_cw_buffers;
    std::vector<std::vector<unsigned char>> d_tb_buffers;
    int d_payload;
    int d_buf_idx;
    int d_tb_number;
public:

    typedef std::shared_ptr<tb_decoder> sptr;

    void frame_in(const char* in, int len);

    tb_decoder(int max_tb_len, int max_cw_len);


};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_TB_DECODER_H*/
