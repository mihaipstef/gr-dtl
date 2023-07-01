/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_TB_ENCODER_H
#define INCLUDED_DTL_TB_ENCODER_H

#include <gnuradio/dtl/fec.h>


namespace gr {
namespace dtl {

class tb_encoder
{
private:
    std::vector<std::vector<unsigned char>> d_cw_buffers;
    std::vector<std::vector<unsigned char>> d_tb_buffers;
    int d_payload;
    std::size_t d_buf_idx;
public:

    typedef std::shared_ptr<tb_encoder> sptr;

    tb_encoder(int max_tb_len, int max_cw_len);

    int encode(const unsigned char* in, int len, fec_enc::sptr enc, int current_tb_len);

    int buf_out(unsigned char* out, int len, int bps);

    bool ready() const;

    int remaining_buf_size();

    int size();

    int buf_payload();

};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_TB_ENCODER_H*/
