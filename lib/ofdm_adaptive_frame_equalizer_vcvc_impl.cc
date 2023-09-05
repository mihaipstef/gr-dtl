/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ofdm_adaptive_frame_equalizer_vcvc_impl.h"

#include "logger.h"
#include <gnuradio/expj.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include "monitor_msg.h"


namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_frame_equalizer_vcvc");

using namespace gr::digital;
using namespace std;

static const pmt::pmt_t CARR_OFFSET_KEY = pmt::mp("ofdm_sync_carr_offset");
static const pmt::pmt_t CHAN_TAPS_KEY = pmt::mp("ofdm_sync_chan_taps");
static const pmt::pmt_t FEEDBACK_PORT = pmt::mp("feedback_port");
static const pmt::pmt_t MONITOR_PORT = pmt::mp("monitor");


ofdm_adaptive_frame_equalizer_vcvc::sptr ofdm_adaptive_frame_equalizer_vcvc::make(
    ofdm_adaptive_equalizer_base::sptr equalizer,
    ofdm_adaptive_feedback_decision_base::sptr feedback_decision,
    int cp_len,
    const std::string& len_tag_key,
    const std::string& frame_no_key,
    bool propagate_channel_state,
    bool propagate_feedback_tags)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_frame_equalizer_vcvc_impl>(
        equalizer,
        feedback_decision,
        cp_len,
        len_tag_key,
        frame_no_key,
        propagate_channel_state,
        propagate_feedback_tags);
}

ofdm_adaptive_frame_equalizer_vcvc_impl::ofdm_adaptive_frame_equalizer_vcvc_impl(
    ofdm_adaptive_equalizer_base::sptr equalizer,
    ofdm_adaptive_feedback_decision_base::sptr feedback_decision,
    int cp_len,
    const std::string& len_tag_key,
    const std::string& frame_no_key,
    bool propagate_channel_state,
    bool propagate_feedback_tags)
    : tagged_stream_block(
          "ofdm_adaptive_frame_equalizer_vcvc",
          io_signature::make(1, 1, sizeof(gr_complex) * equalizer->fft_len()),
          io_signature::make2(
              1, 2, sizeof(gr_complex) * equalizer->fft_len(), sizeof(gr_complex)),
          len_tag_key),
      d_fft_len(equalizer->fft_len()),
      d_cp_len(cp_len),
      d_eq(equalizer),
      d_propagate_channel_state(propagate_channel_state),
      d_channel_state(equalizer->fft_len(), gr_complex(1, 0)),
      d_decision_feedback(feedback_decision),
      d_propagate_feedback_tags(propagate_feedback_tags),
      d_frame_no_key(pmt::string_to_symbol(frame_no_key)),
      d_expected_frame_no(0),
      d_lost_frames(0),
      d_frames_count(0)
{


    set_relative_rate(1, 1);
    // Really, we have TPP_ONE_TO_ONE, but the channel state is not propagated
    set_tag_propagation_policy(TPP_DONT);

    message_port_register_out(FEEDBACK_PORT);
    message_port_register_out(MONITOR_PORT);
}

ofdm_adaptive_frame_equalizer_vcvc_impl::~ofdm_adaptive_frame_equalizer_vcvc_impl() {}

void ofdm_adaptive_frame_equalizer_vcvc_impl::parse_length_tags(
    const std::vector<std::vector<tag_t>>& tags, gr_vector_int& n_input_items_reqd)
{

    for (unsigned k = 0; k < tags[0].size(); k++) {
        if (tags[0][k].key == pmt::string_to_symbol(d_length_tag_key_str)) {
            n_input_items_reqd[0] = pmt::to_long(tags[0][k].value);
        }
    }
}

int ofdm_adaptive_frame_equalizer_vcvc_impl::work(int noutput_items,
                                                  gr_vector_int& ninput_items,
                                                  gr_vector_const_void_star& input_items,
                                                  gr_vector_void_star& output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];
    gr_complex* out_soft = (gr_complex*)output_items[1];

    int carrier_offset = 0;

    int n_ofdm_sym = ninput_items[0];
    int payload = 0;
    int current_frame_no = 0;

    std::vector<tag_t> tags;
    get_tags_in_window(tags, 0, 0, 1);
    for (unsigned i = 0, test = 0; i < tags.size() && test != 15; i++) {
        if (tags[i].key == CHAN_TAPS_KEY) {
            d_channel_state = pmt::c32vector_elements(tags[i].value);
            test |= 1;
        } else if (tags[i].key == CARR_OFFSET_KEY) {
            carrier_offset = pmt::to_long(tags[i].value);
            DTL_LOG_DEBUG("carrier_offset={}", carrier_offset);
            test |= 2;
        } else if (tags[i].key == d_frame_no_key) {
            current_frame_no = pmt::to_long(tags[i].value);
            int lost_frames = 0;
            if (current_frame_no != d_expected_frame_no) {
                if (current_frame_no < d_expected_frame_no) {
                    lost_frames = 4096 + current_frame_no - d_expected_frame_no;
                } else {
                    lost_frames = current_frame_no - d_expected_frame_no;
                }
            }
            assert(lost_frames >= 0);
            d_lost_frames += lost_frames;
            d_frames_count += lost_frames + 1;
            d_expected_frame_no = (current_frame_no + 1) % 4096;
            test |= 4;
        } else if (tags[i].key == payload_length_key()) {
            payload = pmt::to_long(tags[i].value);
            test |= 8;
        }
    }

    DTL_LOG_DEBUG("frame_no={}, payload={}, carrier_offset", current_frame_no, payload, carrier_offset);

    auto cnst_tag_it = find_constellation_tag(tags);
    if (cnst_tag_it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }

    // Copy the frame and the channel state vector such that the symbols are shifted to
    // the correct position
    if (carrier_offset < 0) {
        memset((void*)out, 0x00, sizeof(gr_complex) * (-carrier_offset));
        memcpy((void*)&out[-carrier_offset],
               (void*)in,
               sizeof(gr_complex) * (d_fft_len * n_ofdm_sym + carrier_offset));
    } else {
        memset((void*)(out + d_fft_len * n_ofdm_sym - carrier_offset),
               0x00,
               sizeof(gr_complex) * carrier_offset);
        memcpy((void*)out,
               (void*)(in + carrier_offset),
               sizeof(gr_complex) * (d_fft_len * n_ofdm_sym - carrier_offset));
    }


    // Correct the frequency shift on the symbols
    gr_complex phase_correction;
    for (int i = 0; i < n_ofdm_sym; i++) {
        phase_correction =
            gr_expj(-(2.0 * GR_M_PI) * carrier_offset * d_cp_len / d_fft_len * (i + 1));
        for (int k = 0; k < d_fft_len; k++) {
            out[i * d_fft_len + k] *= phase_correction;
        }
    }

    // Do the equalizing
    d_eq->reset();
    try {
        d_eq->equalize(out, out_soft, n_ofdm_sym, d_channel_state, tags);
    } catch (const std::exception& e) {
        d_logger->error(e.what());
    }
    d_eq->get_channel_state(d_channel_state);

    // Update the channel state regarding the frequency offset
    phase_correction =
        gr_expj((2.0 * GR_M_PI) * carrier_offset * d_cp_len / d_fft_len * n_ofdm_sym);
    for (int k = 0; k < d_fft_len; k++) {
        d_channel_state[k] *= phase_correction;
    }

    // Publish decided constellation to decision feedback port.
    ofdm_adaptive_feedback_t feedback = d_decision_feedback->get_feedback(d_eq->get_snr());
    std::vector<unsigned char> feedback_vector{
        static_cast<unsigned char>(feedback.first), // constellation
        feedback.second, // FEC
    };
    pmt::pmt_t feedback_msg = pmt::cons(pmt::PMT_NIL,
        pmt::init_u8vector(feedback_vector.size(), feedback_vector));

    message_port_pub(FEEDBACK_PORT, feedback_msg);

    pmt::pmt_t msg = monitor_msg(
        make_pair(feedback_constellation_key(), static_cast<unsigned char>(feedback.first)),
        make_pair(fec_key(), feedback.second),
        make_pair(estimated_snr_tag_key(), d_eq->get_snr()),
        make_pair(noise_tag_key(), d_eq->get_noise()),
        make_pair("lost_frames_rate", 100*(double)d_lost_frames/d_frames_count));
    message_port_pub(MONITOR_PORT, msg);

    if (payload) {
        // Propagate tags (except for the channel state and the TSB tag)
        for (size_t i = 0; i < tags.size(); i++) {
            if (tags[i].key != CHAN_TAPS_KEY &&
                tags[i].key != pmt::mp(d_length_tag_key_str)) {
                add_item_tag(0, nitems_written(0), tags[i].key, tags[i].value);
            }
        }

        // Housekeeping
        if (d_propagate_channel_state) {
            add_item_tag(0,
                        nitems_written(0),
                        CHAN_TAPS_KEY,
                        pmt::init_c32vector(d_fft_len, d_channel_state));
        }
        add_item_tag(0,
                        nitems_written(0),
                        noise_tag_key(),
                        pmt::from_double(d_eq->get_noise()));
        // Propagate feedback via tags
        if (d_propagate_feedback_tags) {
            add_item_tag(0,
                        nitems_written(0),
                        estimated_snr_tag_key(),
                        pmt::from_double(d_eq->get_snr()));
            add_item_tag(0,
                        nitems_written(0),
                        feedback_constellation_key(),
                        pmt::from_long(static_cast<unsigned char>(feedback.first)));
            add_item_tag(0, nitems_written(0), fec_feedback_key(), pmt::from_long(feedback.second));
        }

        return n_ofdm_sym;
    }
    return 0;
}

} /* namespace dtl */
} /* namespace gr */
