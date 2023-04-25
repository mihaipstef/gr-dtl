/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_fec_frame_bvb_impl.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_fec_frame_bvb");

ofdm_adaptive_fec_frame_bvb::sptr
ofdm_adaptive_fec_frame_bvb::make(const vector<fec_enc::sptr> encoders,
                                 int frame_capacity,
                                 int max_bps,
                                 const string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_fec_frame_bvb_impl>(
        encoders, frame_capacity, max_bps, len_key);
}

/*
 * The private constructor
 */
ofdm_adaptive_fec_frame_bvb_impl::ofdm_adaptive_fec_frame_bvb_impl(
    const vector<fec_enc::sptr> encoders,
    int frame_capacity,
    int max_bps,
    const string& len_key)
    : gr::block("ofdm_adaptive_fec_frame_bvb",
                gr::io_signature::make(
                    1 /* min inputs */, 1 /* max inputs */, sizeof(unsigned char)),
                gr::io_signature::make(
                    1 /* min outputs */, 1 /*max outputs */, frame_capacity * sizeof(unsigned char))),
      d_encoders(encoders),
      d_frame_capacity(frame_capacity),
      d_tb_len(0),
      d_tb_count(0),
      d_cw_count(0),
      d_feedback_fec_idx(1),
      d_feedback_cnst(constellation_type_t::BPSK),
      d_current_enc(nullptr),
      d_current_cnst(constellation_type_t::BPSK),
      d_current_bps(1),
      d_current_frame_len(0),
      d_current_frame_offset(0),
      d_len_key(pmt::intern(len_key)),
      d_tag_offset(0),
      d_action(Action::PROCESS_INPUT),
      d_used_frames_count(0)

{
    // Find longest code
    auto it = max_element(d_encoders.begin() + 1,
                          d_encoders.end(),
                          [](const decltype(d_encoders)::value_type& l,
                             const decltype(d_encoders)::value_type& r) {
                              return l->get_n() < r->get_n();
                          });
    if (it == d_encoders.end()) {
        throw(std::runtime_error("No encoder found!"));
    }

    d_tb_enc = make_shared<tb_encoder>(d_frame_capacity * max_bps *
                                           compute_tb_len((*it)->get_n(), max_bps),
                                       (*it)->get_n());

    set_min_noutput_items(d_frame_capacity);

}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_fec_frame_bvb_impl::~ofdm_adaptive_fec_frame_bvb_impl() {}

void ofdm_adaptive_fec_frame_bvb_impl::process_feedback(pmt::pmt_t feedback)
{
    if (pmt::is_dict(feedback)) {
        if (pmt::dict_has_key(feedback, feedback_constellation_key())) {
            constellation_type_t constellation =
                static_cast<constellation_type_t>(pmt::to_long(pmt::dict_ref(
                    feedback,
                    feedback_constellation_key(),
                    pmt::from_long(static_cast<int>(constellation_type_t::BPSK)))));
            int bps = get_bits_per_symbol(constellation);
            // Update constellation only if valid data received
            if (bps) {
                d_feedback_cnst = constellation;
            }
        }
        if (pmt::dict_has_key(feedback, fec_feedback_key())) {
            d_feedback_fec_idx = pmt::to_long(
                pmt::dict_ref(feedback, fec_feedback_key(), pmt::from_long(0)));
        }
    }
    DTL_LOG_DEBUG("process_feedback: d_constellation={}, d_fec_scheme={}",
                  static_cast<int>(d_feedback_cnst),
                  d_feedback_fec_idx);
}

int ofdm_adaptive_fec_frame_bvb_impl::compute_tb_len(int cw_len, int bps)
{
    int frame_capacity = d_frame_capacity * bps;
    int tb_len = 1;
    if (frame_capacity >= cw_len) {
        tb_len = 1 + frame_capacity / cw_len;
    }
    return tb_len;
}


void ofdm_adaptive_fec_frame_bvb_impl::add_frame_tags(int frame_payload)
{

    DTL_LOG_DEBUG("add tags: tag_offset={}", d_tag_offset);
    add_item_tag(0, d_tag_offset, d_len_key, pmt::from_long(d_frame_capacity));
    add_item_tag(0,
                 d_tag_offset,
                 get_constellation_tag_key(),
                 pmt::from_long(static_cast<int>(d_current_cnst)));
    add_item_tag(0, d_tag_offset, payload_length_key(), pmt::from_long(frame_payload));
    add_item_tag(0, d_tag_offset, fec_key(), pmt::from_long(d_current_fec_idx));
    add_item_tag(0,
                 d_tag_offset,
                 fec_tb_key(),
                 pmt::from_long(static_cast<int>(d_tb_count & 0xffff)));
    add_item_tag(0,
                 d_tag_offset,
                 fec_tb_payload_key(),
                 pmt::from_long(static_cast<int>(d_tb_enc->buf_payload() & 0xffff)));
    add_item_tag(0,
                 d_tag_offset,
                 fec_tb_index_key(),
                 pmt::from_long(d_used_frames_count && 0xf));
    add_item_tag(0,
                 d_tag_offset,
                 fec_tb_key(),
                 pmt::from_long(d_tb_count && 0xff));
    d_tag_offset += 1;
}


void ofdm_adaptive_fec_frame_bvb_impl::padded_frame_out(int frame_payload)
{

    add_frame_tags(tb_offset_to_bytes());
}

int n_bits_to_bytes(int nbits)
{
    int nbytes = nbits / 8;
    if (nbits % 8) {
        ++nbytes;
    }
    return nbytes;
}

int n_bytes_to_syms(int nbytes, int bps)
{
    int nsyms = nbytes * 8 / bps;
    if (nbytes * 8 % bps) {
        ++nsyms;
    }
    return nsyms;
}

int n_bits_to_syms(int nbits, int bps)
{
    int nsyms = nbits / bps;
    if (nbits % bps) {
        ++nsyms;
    }
    return nsyms;
}


int ofdm_adaptive_fec_frame_bvb_impl::tb_offset_to_bytes()
{
    int offset_in_bytes = d_current_frame_offset * d_current_bps / 8;
    if (d_current_frame_offset * d_current_bps % 8) {
        ++offset_in_bytes;
    }
    return offset_in_bytes;
}

int ofdm_adaptive_fec_frame_bvb_impl::current_frame_available_bytes()
{
    int available_bytes = (d_frame_capacity - d_current_frame_offset) * d_current_bps / 8;
    return available_bytes;
}


int ofdm_adaptive_fec_frame_bvb_impl::general_work(int noutput_items,
                                                  gr_vector_int& ninput_items,
                                                  gr_vector_const_void_star& input_items,
                                                  gr_vector_void_star& output_items)
{
    auto in = static_cast<const unsigned char*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    int write_index = d_current_frame_offset;
    int read_index = 0;
    int consumed_input = 0;
    int produced_frames = 0;
    int output_available = noutput_items * d_frame_capacity;


    DTL_LOG_DEBUG("general_work start: d_frame_capacity={}, noutput={}, ninput={}, action={}",
                  d_frame_capacity,
                  output_available,
                  ninput_items[0],
                  (int)d_action);

    // If no input but enough space in output buffer, generate an empty frame
    if (ninput_items[0] == 0 && noutput_items > 0) {
        int frame_payload = tb_offset_to_bytes();
        if (output_available >= 1) {
            padded_frame_out(frame_payload);
            return 1;
        }
    }

    bool wait_next_work = false;


    while (read_index < ninput_items[0] && !wait_next_work) {

        assert(d_action == Action::PROCESS_INPUT && d_tb_enc->ready());

        switch (d_action) {

            case Action::PROCESS_INPUT:
            {
                // Update constellation and FEC
                d_current_fec_idx = d_feedback_fec_idx;
                d_current_enc = d_encoders[d_current_fec_idx];
                d_current_cnst = d_feedback_cnst;
                d_current_bps = get_bits_per_symbol(d_current_cnst);
                d_tb_len = compute_tb_len(d_current_enc->get_n(), d_current_bps);

                // Frame carries an integer number of bytes
                d_current_frame_len = d_frame_capacity * d_current_bps / 8;
                //d_current_frame_len *= 8;

                int tb_required_nin = d_tb_len * d_current_enc->get_k();

                int available_in = ninput_items[0] - read_index;

                int to_read = min(available_in, tb_required_nin);

                // encode
                d_tb_enc->encode(&in[read_index], to_read, d_current_enc, d_tb_len);

                read_index += to_read;

                d_action = Action::OUTPUT_BUFFER;
                d_used_frames_count = 0;
                ++d_tb_count;
                DTL_LOG_DEBUG("Input processed: tb_len={}, tb_payload={}, read_index={}",
                            d_tb_enc->size(),
                            d_tb_enc->buf_payload(),
                            read_index);

            }
            break;

            // Empty TB buffer in output buffer
            case Action::OUTPUT_BUFFER:
            {

                if (noutput_items == 0) {
                    wait_next_work = true;
                } else {

                    // Output the TB frame by frame
                    while (!d_tb_enc->ready() && produced_frames < noutput_items) {

                        int bytes_needed = n_bits_to_bytes(d_tb_enc->remaining_buf_size());
                        int out_frame_bytes =
                            min(bytes_needed, current_frame_available_bytes());

                        DTL_LOG_DEBUG("output_buffer: bytes={}, syms={}, offset={}", out_frame_bytes, n_bytes_to_syms(out_frame_bytes, d_current_bps), d_current_frame_offset);

                        d_tb_enc->buf_out(&out[write_index], min(out_frame_bytes * 8, d_tb_enc->remaining_buf_size()), d_current_bps);

                        if (out_frame_bytes == current_frame_available_bytes()) {
                            // add tags
                            add_frame_tags(d_frame_capacity * d_current_bps / 8);
                            d_current_frame_offset = 0;
                            ++produced_frames;
                        } else {
                            d_current_frame_offset += n_bytes_to_syms(out_frame_bytes, d_current_bps);
                        }
                        ++d_used_frames_count;
                        write_index += n_bytes_to_syms(out_frame_bytes, d_current_bps);
                    }

                    // If TB buffer empty check if we can start another TB in current frame
                    if (d_tb_enc->ready()) {

                        if (d_used_frames_count == 1) {
                            d_action = Action::FINALIZE_FRAME;
                        } else {
                            d_action = Action::PROCESS_INPUT;
                        }

                        consumed_input += d_tb_enc->buf_payload();
                    } else {
                        // Did not output the entire buffer, wait for next scheduler allocation
                        wait_next_work = true;
                    }
                }
                DTL_LOG_DEBUG("output buffer: next={}, consumed={}, offset={}", (int)d_action, consumed_input, d_current_frame_offset);
            }
            break;
            case Action::FINALIZE_FRAME:
            {
                // pad rest  of the frame (frame_len - frame_offset)
                int frame_payload = d_current_frame_offset * d_current_bps / 8;
                if (d_current_frame_offset * d_current_bps % 8) {
                    ++frame_payload;
                }
                padded_frame_out(frame_payload);
                d_current_frame_offset = 0;
                d_action = Action::PROCESS_INPUT;
                DTL_LOG_DEBUG("finalize frame: payload={}", frame_payload);
            }
            break;
        } // action switch
    } // input loop

    DTL_LOG_DEBUG("general_work finish: d_frame_capacity={}, noutput={}, ninput={}, "
                  "read_index={}, write_index={}, tb_len={}, frame_offset={}",
                  d_frame_capacity,
                  output_available,
                  ninput_items[0],
                  read_index,
                  write_index,
                  d_tb_len,
                  d_current_frame_offset);

    consume_each(consumed_input);
    return produced_frames;
}

} /* namespace dtl */
} /* namespace gr */
