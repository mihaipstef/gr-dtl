/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "fec_utils.h"
#include <gnuradio/testbed/logger.h>
#include <gnuradio/testbed/monitor_msg.h>
#include <gnuradio/dtl/ofdm_adaptive_utils.h>
#include <gnuradio/io_signature.h>
#include "ofdm_adaptive_fec_decoder_impl.h"

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_fec_decoder");

static const pmt::pmt_t MONITOR_PORT = pmt::mp("monitor");

using namespace std;

typedef tuple<unsigned char, unsigned char, int, unsigned char, unsigned char>
    fec_header_tuple_t;

ofdm_adaptive_fec_decoder::sptr
ofdm_adaptive_fec_decoder::make(const vector<fec_dec::sptr>& decoders,
                                int frame_capacity,
                                int max_bps,
                                const string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_fec_decoder_impl>(
        decoders, frame_capacity, max_bps, len_key);
}

ofdm_adaptive_fec_decoder_impl::ofdm_adaptive_fec_decoder_impl(
    const vector<fec_dec::sptr>& decoders,
    int frame_capacity,
    int max_bps,
    const string& len_key)
    : gr::block(
          "ofdm_adaptive_fec_decoder",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(float)),
          gr::io_signature::make(
              1 /* min outputs */, 1 /*max outputs */, sizeof(unsigned char))),
      d_len_key(pmt::string_to_symbol(len_key)),
      d_decoders(decoders),
      d_frame_capacity(frame_capacity),
      d_processed_input(0),
      d_crc(4, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF),
      d_to_bytes(1, 8)
{
    auto it_max_n = max_element(d_decoders.begin() + 1,
                                d_decoders.end(),
                                [](const decltype(d_decoders)::value_type& l,
                                   const decltype(d_decoders)::value_type& r) {
                                    return l->get_n() < r->get_n();
                                });
    auto it_max_k = max_element(d_decoders.begin() + 1,
                                d_decoders.end(),
                                [](const decltype(d_decoders)::value_type& l,
                                   const decltype(d_decoders)::value_type& r) {
                                    return l->get_k() < r->get_k();
                                });
    if (it_max_n == d_decoders.end() || it_max_k == d_decoders.end()) {
        throw(std::runtime_error("Misconfiguration: decoder not found!"));
    }
    int frame_len = d_frame_capacity * max_bps;
    int ncws = compute_tb_len((*it_max_n)->get_n(), frame_len);
    d_tb_dec = make_shared<tb_decoder>((*it_max_n)->get_n() * ncws);
    d_crc_buffer.resize((*it_max_k)->get_k() * ncws / 8 + 1);
    message_port_register_out(pmt::mp("monitor"));
    set_tag_propagation_policy(block::tag_propagation_policy_t::TPP_DONT);
}

ofdm_adaptive_fec_decoder_impl::~ofdm_adaptive_fec_decoder_impl() {}


int ofdm_adaptive_fec_decoder_impl::general_work(int noutput_items,
                                                 gr_vector_int& ninput_items,
                                                 gr_vector_const_void_star& input_items,
                                                 gr_vector_void_star& output_items)
{
    auto in = static_cast<const float*>(input_items[0]);
    auto out = static_cast<unsigned char*>(output_items[0]);

    int read_index = 0;
    int write_index = 0;

    DTL_LOG_DEBUG("work: ninput={}, noutput={}", ninput_items[0], noutput_items);

    while (read_index < ninput_items[0]) {
        vector<tag_t> tags;
        get_tags_in_range(
            tags, 0, nitems_read(0) + read_index, nitems_read(0) + read_index + 1);

        int test = 0;
        int bps = 0;
        int len = 0;
        int frame_len = 0;

        for (auto& tag : tags) {

            if (tag.key == get_constellation_tag_key()) {
                bps = get_bits_per_symbol(
                    static_cast<constellation_type_t>(pmt::to_long(tag.value)));
                test |= 1;
            } else if (tag.key == d_len_key) {
                len = pmt::to_long(tag.value);
                test |= 2;
                // remove_item_tag(0, tag);
            }
            if (test == 3) {
                break;
            }
        }

        DTL_LOG_DEBUG("nitems_read={}, read_index={}, frame_len={}", nitems_read(0), read_index, len);

        if (test != 3) {
            DTL_LOG_ERROR("Tags missing: check_bitmap={}, lookup_offset={}",
                          test,
                          nitems_read(0) + read_index);
            throw runtime_error("Tags missing");
        }

        frame_len = len;

        if (read_index + frame_len > ninput_items[0]) {
            break;
        }

        fec_info_t::sptr fec_info = make_fec_info(tags, {}, d_decoders);

        if (!fec_info) {
            throw runtime_error("FEC tags missing");
        }

        // If empty frame received...
        if (fec_info->d_frame_payload == 0) {
            // ... only update consumed samples.
            read_index += frame_len;
        // Otherwise...
        } else {
            // ... proceed with the frame.
            fec_info->d_tb_offset *= 8;
            int code_n = fec_info->get_n();
            int ncws = compute_tb_len(code_n, 8 * align_bits_to_bytes(d_frame_capacity * bps));


            // Make sure we consume input only if we'll be able to produce the output
            // if ((d_tb_dec->receive_buffer_empty() &&
            //     noutput_items < fec_info->d_tb_payload_len) ||
            //     (!d_tb_dec->receive_buffer_empty() &&
            //     noutput_items < d_tb_dec->get_current_tb_payload())) {
            //     break;
            // }

            // When transport block is decoded copy user data to the output buffer
            auto on_data_ready = [this, &write_index, &out, &bps, &code_n, &ncws](
                                    const std::vector<unsigned char>& data_buffer,
                                    fec_info_t::sptr tb_fec_info, int avg_it) {
                int user_data_len = data_buffer.size() - d_crc.get_crc_len() * 8;
                int crc_buf_len = d_to_bytes.repack_lsb_first(
                    &data_buffer[0], user_data_len, &d_crc_buffer[0]);
                d_to_bytes.repack_lsb_first(&data_buffer[user_data_len],
                                            d_crc.get_crc_len() * 8,
                                            &d_crc_buffer[crc_buf_len]);
                bool crc_ok =
                    d_crc.verify_crc(&d_crc_buffer[0], crc_buf_len + d_crc.get_crc_len());

                if (crc_ok) {
                    struct iphdr *iph = (struct iphdr *)&data_buffer[0];
                    memcpy(&out[write_index], &data_buffer[0], user_data_len);

                    // int data_bytes = d_to_bytes.repack_lsb_first(&data_buffer[0], user_data_len, &out[write_index]);
                    // assert(data_bytes == user_data_len/8);
                    add_item_tag(0, nitems_written(0)+write_index, d_len_key, pmt::from_long(user_data_len));
                    write_index += user_data_len;
                }
                double tber = 100 * d_crc.get_failed() / static_cast<double>(d_crc.get_failed() + d_crc.get_success());

                pmt::pmt_t msg = monitor_msg_builder.build_any(
                    make_pair("tb_no", tb_fec_info->d_tb_number),
                    make_pair("tb_payload", tb_fec_info->d_tb_payload_len),
                    make_pair("tb_code_k", tb_fec_info->get_k()),
                    make_pair("tb_code_n", tb_fec_info->get_n()),
                    make_pair("tb_codewords", ncws),
                    make_pair("frame_payload", tb_fec_info->d_frame_payload),
                    make_pair("bps", bps),
                    make_pair("crc_ok_count", d_crc.get_success()),
                    make_pair("crc_fail_count", d_crc.get_failed()),
                    make_pair("tber", tber),
                    make_pair("avg_it", avg_it));
                message_port_pub(MONITOR_PORT, msg);

                DTL_LOG_DEBUG("tb_payload_ready: crc_ok={}, tb_no={}, tb_payload={}, bps={}, user_data_len={}, avg_it={}, crc_fail_count={}",
                            crc_ok,
                            tb_fec_info->d_tb_number,
                            tb_fec_info->d_tb_payload_len,
                            bps,
                            user_data_len, avg_it, d_crc.get_failed());
            };

            d_tb_dec->process_frame(&in[read_index],
                                    8 * align_bits_to_bytes(d_frame_capacity * bps),
                                    bps,
                                    fec_info,
                                    on_data_ready);
            read_index += frame_len;
        }
    }
    DTL_LOG_DEBUG("work: consumed={}, produced={}", read_index, write_index);
    consume_each(read_index);
    return write_index;
}

} /* namespace dtl */
} /* namespace gr */
