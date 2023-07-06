/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Action diagram

@startuml
title "FEC frame construction"
(*) --> if "No input" then
        ->[true] "Generate a single
                  empty frame" as empty_frame
    else
        --> if "action == INPUT" then
            ->[true] "Compute a single transport block (TB)
                      and store it in internal buffer" as encode
        else
            if "action == OUTPUT" then
                ->[true] "Output the internal buffer frame by frame"
            else
                if "action == FINALIZE" then
                    ->[true] "Pad and finalize the frame"
                else
                endif
            endif
        endif
endif
@enduml
*/

#include "ofdm_adaptive_fec_frame_bvb_impl.h"

#include "fec_utils.h"
#include "logger.h"
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>
#include "repack.h"

namespace gr {
namespace dtl {

using namespace std;

INIT_DTL_LOGGER("ofdm_adaptive_fec_frame_bvb");

ofdm_adaptive_fec_frame_bvb::sptr
ofdm_adaptive_fec_frame_bvb::make(const vector<fec_enc::sptr>& encoders,
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
    const vector<fec_enc::sptr>& encoders,
    int frame_capacity,
    int max_bps,
    const string& len_key)
    : gr::block("ofdm_adaptive_fec_frame_bvb",
                gr::io_signature::make(
                    1 /* min inputs */, 1 /* max inputs */, sizeof(unsigned char)),
                gr::io_signature::make(1 /* min outputs */,
                                       1 /*max outputs */,
                                       frame_capacity * sizeof(unsigned char))),
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
      d_used_frames_count(0),
      d_frame_used_capacity(0),
      d_consecutive_empty_frames(0),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF)
{
    // Find longest code
    if (d_encoders.size() <= 1) {
        throw(std::runtime_error("No encoder found!"));
    }
    auto it_max_n = max_element(d_encoders.begin() + 1,
                          d_encoders.end(),
                          [](const decltype(d_encoders)::value_type& l,
                             const decltype(d_encoders)::value_type& r) {
                              return l->get_n() < r->get_n();
                          });
    auto it_max_k = max_element(d_encoders.begin() + 1,
                          d_encoders.end(),
                          [](const decltype(d_encoders)::value_type& l,
                             const decltype(d_encoders)::value_type& r) {
                              return l->get_k() < r->get_k();
                          });

    int ncws = compute_tb_len((*it_max_n)->get_n(), d_frame_capacity * max_bps);
    d_tb_enc = make_shared<tb_encoder>((*it_max_n)->get_n() * ncws, (*it_max_n)->get_n());

    d_tb_payload.resize((*it_max_k)->get_k() * ncws);
    d_crc_buffer.resize((*it_max_k)->get_k() * ncws / 8 + 1);

    set_min_noutput_items(1);
    this->message_port_register_in(pmt::mp("feedback"));
    this->set_msg_handler(pmt::mp("feedback"),
                          [this](pmt::pmt_t msg) { this->process_feedback(msg); });
    message_port_register_out(pmt::mp("monitor"));
    DTL_LOG_DEBUG("frame_capacity={}, max_bps={}", frame_capacity, max_bps);
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
                 fec_tb_payload_key(),
                 pmt::from_long(static_cast<int>(d_tb_enc->buf_payload() & 0xffff)));
    // add_item_tag(
    //     0, d_tag_offset, fec_tb_index_key(), pmt::from_long(d_used_frames_count &&
    //     0xf));
    add_item_tag(0,
                 d_tag_offset,
                 fec_offset_key(),
                 pmt::from_long(d_current_frame_offset & 0xfff));

    add_item_tag(0, d_tag_offset, fec_tb_key(), pmt::from_long(d_tb_count & 0xff));
    d_tag_offset += 1;
    DTL_LOG_DEBUG("frame_out: tb_no={}", d_tb_count);
}


void ofdm_adaptive_fec_frame_bvb_impl::padded_frame_out(int frame_payload)
{

    add_frame_tags(frame_payload);
}

int ofdm_adaptive_fec_frame_bvb_impl::align_bytes_to_syms(int nbytes)
{
    int nsyms = nbytes * 8 / d_current_bps;
    if (nbytes * 8 % d_current_bps) {
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
    int available_bytes = (d_frame_capacity - d_frame_used_capacity) * d_current_bps / 8;
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
    repack to_bytes(1, 8);
    repack to_bits(8, 1);

    DTL_LOG_DEBUG("work_start: d_frame_capacity={}, noutput={}, ninput={}, action={}",
                  d_frame_capacity,
                  output_available,
                  ninput_items[0],
                  (int)d_action);

    // If no input but enough space in output buffer, generate an empty frame
    if (ninput_items[0] == 0 && noutput_items > 0) {
        int frame_payload = tb_offset_to_bytes();
        if (output_available >= 1) {
            if (++d_consecutive_empty_frames == 3) {
                DTL_LOG_DEBUG("work_done: consecutive_empty_frames={}",
                              d_consecutive_empty_frames);
                return WORK_DONE;
            } else {
                DTL_LOG_DEBUG("empty_frame: payload={}", frame_payload);
                padded_frame_out(frame_payload);
                return 1;
            }
        } else {
            return 0;
        }
    }

    bool wait_next_work = false;

    // While there is data to read or processed data to output
    while ((read_index < ninput_items[0] || d_action != Action::PROCESS_INPUT) &&
           !wait_next_work) {

        assert(d_action == Action::PROCESS_INPUT && d_tb_enc->ready());

        switch (d_action) {

        case Action::PROCESS_INPUT: {

            // Update constellation and FEC
            if (d_feedback_cnst != d_current_cnst && d_current_frame_offset > 0) {
                d_action = Action::FINALIZE_FRAME;
                continue;
            }

            d_current_fec_idx = d_feedback_fec_idx;
            d_current_enc = d_encoders[d_current_fec_idx];
            d_current_cnst = d_feedback_cnst;
            d_current_bps = get_bits_per_symbol(d_current_cnst);

            // Frame carries an integer number of bytes
            d_current_frame_len = d_frame_capacity * d_current_bps / 8;
            d_tb_len = compute_tb_len(d_current_enc->get_n(), d_current_frame_len * 8);
            d_frame_padding_syms =
                (d_frame_capacity * d_current_bps - d_current_frame_len * 8) /
                d_current_bps;

            // d_current_frame_len *= 8;

            int tb_payload_max = d_tb_len * d_current_enc->get_k() - d_crc.get_crc_len() * 8;

            if (tb_payload_max <= 0) {
                throw runtime_error("Misconfiguration: TB should be able to carry  at least 1 user data bit");
            }

            int available_in = ninput_items[0] - read_index;

            int to_read = min(available_in, tb_payload_max);

            // Copy user data in payload and CRC buffers
            memcpy(&d_tb_payload[0], &in[read_index], to_read);
            int crc_buf_len = to_bytes.repack_lsb_first(&in[read_index], to_read, &d_crc_buffer[0]);
            // Compute CRC and move the value in payload buffer
            d_crc.append_crc(&d_crc_buffer[0], crc_buf_len);
            to_bits.repack_lsb_first(&d_crc_buffer[crc_buf_len], d_crc.get_crc_len(), &d_tb_payload[to_read]);

            // encode
            d_tb_enc->encode(&d_tb_payload[0], to_read + d_crc.get_crc_len() * 8, d_current_enc, d_tb_len);

            read_index += to_read;

            d_action = Action::OUTPUT_BUFFER;
            consumed_input += to_read;

            d_used_frames_count = 0;
            ++d_tb_count;
            d_consecutive_empty_frames = 0;

            DTL_LOG_DEBUG("input_processed: tb_len={}, tb_payload={}, read_index={}, "
                          "padding_syms={}, tb_no={}",
                          d_tb_enc->size(),
                          d_tb_enc->buf_payload(),
                          read_index,
                          d_frame_padding_syms,
                          d_tb_count);

        } break;

        // Empty TB buffer in output buffer
        case Action::OUTPUT_BUFFER: {
            // If there isn't any frame available in output buffer...
            if (noutput_items == 0) {
                //...skip to next scheduled work.
                wait_next_work = true;
            } else {

                // Output the TB frame by frame
                while (!d_tb_enc->ready() && produced_frames < noutput_items) {

                    int bytes_left_in_tb =
                        align_bits_to_bytes(d_tb_enc->remaining_buf_size());
                    int out_frame_bytes =
                        min(bytes_left_in_tb, current_frame_available_bytes());

                    DTL_LOG_DEBUG("output_buffer: bytes={}, syms={}, offset={}",
                                  out_frame_bytes,
                                  align_bytes_to_syms(out_frame_bytes),
                                  d_current_frame_offset);

                    d_tb_enc->buf_out(
                        &out[write_index],
                        min(out_frame_bytes * 8, d_tb_enc->remaining_buf_size()),
                        d_current_bps);

                    // If we fill out the current frame ...
                    if (out_frame_bytes == current_frame_available_bytes()) {
                        // add tags
                        add_frame_tags(d_frame_used_capacity * d_current_bps / 8 +
                                       out_frame_bytes);
                        d_current_frame_offset = 0;
                        ++produced_frames;
                        write_index += (d_frame_capacity - d_frame_used_capacity);
                        d_frame_used_capacity = 0;
                    } else {
                        d_current_frame_offset += out_frame_bytes;
                        d_frame_used_capacity = align_bytes_to_syms(out_frame_bytes);
                        write_index += d_frame_used_capacity;
                    }
                    ++d_used_frames_count;
                }

                // If TB buffer empty check if we can start another TB in current frame
                if (d_tb_enc->ready()) {

                    if (d_used_frames_count == 1) {
                        d_action = Action::FINALIZE_FRAME;
                    } else {
                        // If no data left in the input buffer...
                        if (read_index == ninput_items[0]) {
                            // ...go ahead and finalize the frame.
                            d_action = Action::FINALIZE_FRAME;
                            // Otherwise...
                        } else {
                            // ...continue with input processing.
                            d_action = Action::PROCESS_INPUT;
                        }
                    }
                } else {
                    // Did not output the entire buffer, wait for next scheduler
                    // allocation
                    wait_next_work = true;
                }
            }
            DTL_LOG_DEBUG("output_buffer: next={}, consumed={}, offset={}",
                          (int)d_action,
                          consumed_input,
                          d_current_frame_offset);
        } break;
        case Action::FINALIZE_FRAME: {
            DTL_LOG_DEBUG("finalize_frame: payload={}", d_current_frame_offset);
            padded_frame_out(d_current_frame_offset);
            d_frame_used_capacity = 0;
            d_current_frame_offset = 0;
            d_action = Action::PROCESS_INPUT;
            ++produced_frames;
        } break;
        } // action switch
    }     // input loop

    DTL_LOG_DEBUG("work_finish: d_frame_capacity={}, noutput={}, ninput={}, "
                  "read_index={}, write_index={}, tb_len={}, frame_offset={}, "
                  "produced={}, consumed={}",
                  d_frame_capacity,
                  output_available,
                  ninput_items[0],
                  read_index,
                  write_index,
                  d_tb_len,
                  d_current_frame_offset,
                  produced_frames,
                  consumed_input);

    consume_each(consumed_input);
    return produced_frames;
}

} /* namespace dtl */
} /* namespace gr */
