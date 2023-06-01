/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_TB_DECODER_H
#define INCLUDED_DTL_TB_DECODER_H

#include "fec_utils.h"
#include <gnuradio/dtl/fec.h>


namespace gr {
namespace dtl {

class tb_decoder
{

private:

    enum buffers_id_t {
        RCV_BUF = 0,
        FULL_BUF,
    };

    std::vector<std::vector<float>> d_tb_buffers;
    std::vector<unsigned char> d_data_buffer;

    int d_payload;
    int d_tb_payload_len;
    int d_tb_number;
    int d_buf_idx;
    int d_tb_len;
    fec_info_t::sptr d_fec_info;
    int d_processed;

    int decode(int tb_len);

public:

    typedef std::shared_ptr<tb_decoder> sptr;

    bool process_frame(const float* in, int frame_len, int frame_payload_len, int bps, fec_info_t::sptr fec_info);

    std::pair<int, int> buf_out(unsigned char* out);
 
    tb_decoder(int max_tb_len);


};

} // namespace dtl
} // namespace gr

#endif /*INCLUDED_DTL_TB_DECODER_H*/
