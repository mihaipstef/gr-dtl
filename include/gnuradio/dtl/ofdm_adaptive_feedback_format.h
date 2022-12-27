/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_FORMAT_H
#define INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_FORMAT_H

#include <gnuradio/digital/crc.h>
#include <gnuradio/digital/header_buffer.h>
#include <gnuradio/digital/header_format_base.h>
#include <gnuradio/dtl/api.h>
#include <pmt/pmt.h>

namespace gr {
namespace dtl {

/*!
 * \brief
 * \ingroup dtl
 *
 */
class DTL_API ofdm_adaptive_feedback_format : public gr::digital::header_format_base
{
public:
    typedef std::shared_ptr<ofdm_adaptive_feedback_format> sptr;
    ofdm_adaptive_feedback_format(const std::string& access_code, int threshold);
    ~ofdm_adaptive_feedback_format() override;

    bool format(int nbytes_in,
                const unsigned char* input,
                pmt::pmt_t& output,
                pmt::pmt_t& info) override;

    bool parse(int nbits_in,
               const unsigned char* input,
               std::vector<pmt::pmt_t>& info,
               int& nbits_processed) override;

    size_t header_nbits() const override;

    unsigned long long access_code() const;

    unsigned int threshold() const;

    static sptr make(const std::string& access_code, int threshold);

protected:
    uint64_t d_access_code;
    size_t d_access_code_len;
    unsigned long long d_data_reg;
    unsigned long long d_mask;
    unsigned int d_threshold; 
    gr::digital::crc d_crc8;

    bool header_ok() override;
    int header_payload() override;

    // WORKAROUND to fix "Undefined symbol" errors
    inline void enter_search() override { /* Nothing */ }
    inline void enter_have_sync() override { /* Nothing */ }
    inline void enter_have_header(int payload_len) override { /* Nothing */}

private:
    bool parse_feedback(int nbits_in,
                        const unsigned char* input,
                        std::vector<pmt::pmt_t>& info);
};

} // namespace dtl
} // namespace gr

#endif /* INCLUDED_DTL_OFDM_ADAPTIVE_FEEDBACK_FORMAT_H */
