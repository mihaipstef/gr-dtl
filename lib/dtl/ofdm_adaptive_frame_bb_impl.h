/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H

#include <chrono>
#include "crc_util.h"
#include "frame_file_store.h"
#include <gnuradio/dtl/ofdm_adaptive_frame_bb.h>
#include <random>
#include <gnuradio/testbed/repack.h>

namespace gr {
namespace dtl {

/*!
 * \brief Prepare the frame by adding the payload CRC, unpacking and tagging the
 * stream according to the feedback received. \ingroup dtl
 *
 */
class ofdm_adaptive_frame_bb_impl : public ofdm_adaptive_frame_bb
{
public:
    ofdm_adaptive_frame_bb_impl(const std::string& len_tag_key,
                                const std::vector<constellation_type_t>& constellations,
                                size_t frame_len,
                                double frame_rate,
                                size_t n_payload_carriers,
                                std::string frames_fname,
                                int max_empty_frames);

    void process_feedback(pmt::pmt_t feedback);

    void process_feedback_header(pmt::pmt_t header_data);

    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items) override;
    bool start() override;
    void set_constellation(constellation_type_t constellation) override;

protected:
    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

private:
    size_t frame_length_bits(size_t frame_len,
                             size_t n_payload_carriers,
                             size_t bits_per_symbol);
    size_t frame_length();

    void rand_pad(unsigned char* buf, size_t len, std::uniform_int_distribution<>& dist);

    int frame_out(const unsigned char* in,
                   int nbytes_in,
                   unsigned char* out,
                   int nsyms_out,
                   repack& repacker);

    constellation_type_t d_constellation;
    unsigned char d_fec_scheme;
    uint64_t d_tag_offset;
    int d_frame_len;
    pmt::pmt_t d_packet_len_tag;
    int d_payload_carriers;
    size_t d_bytes;
    unsigned char d_bps;
    bool d_waiting_full_frame;
    bool d_waiting_for_input;
    int d_max_empty_frames;
    crc_util d_crc;
    std::vector<unsigned char> d_frame_buffer;
    unsigned long d_frame_count;
    frame_file_store d_frame_store;
    int d_frame_in_bytes;
    int d_consecutive_empty_frames;
    std::chrono::time_point<std::chrono::steady_clock> d_start_time;
    std::chrono::duration<double> d_frame_duration;
    constellation_type_t d_feedback_cnst;
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FRAME_BB_IMPL_H */