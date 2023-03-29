/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logger.h"
#include "ofdm_adaptive_constellation_metric_vcvf_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace dtl {

INIT_DTL_LOGGER("ofdm_adaptive_constellation_metric_vcvf")

using namespace std;

ofdm_adaptive_constellation_metric_vcvf::sptr
ofdm_adaptive_constellation_metric_vcvf::make(
    unsigned int fft_len,
    const vector<unsigned>& subcarriers,
    const std::vector<constellation_type_t>& constellations,
    const std::string& len_key)
{
    return gnuradio::make_block_sptr<ofdm_adaptive_constellation_metric_vcvf_impl>(
        fft_len, subcarriers, constellations, len_key);
}


/*
 * The private constructor
 */
ofdm_adaptive_constellation_metric_vcvf_impl::
    ofdm_adaptive_constellation_metric_vcvf_impl(
        unsigned int fft_len,
        const vector<unsigned>& subcarriers,
        const std::vector<constellation_type_t>& constellations,
        const std::string& len_key)
    : gr::tagged_stream_block(
          "ofdm_adaptive_constellation_metric_vcvf",
          gr::io_signature::make2(
              1, 2, sizeof(gr_complex) * fft_len, sizeof(gr_complex) * fft_len),
          gr::io_signature::make(1, 1, sizeof(float) * fft_len),
          len_key),
      d_fft_len(fft_len),
      d_subcarriers(fft_len, false)
{
    // Assert max subcarrier value
    auto m = *max_element(subcarriers.begin(), subcarriers.end());
    if (m > fft_len) {
        throw(runtime_error(
            "Argument mismatch: max_subcarrier does not match subcarriers"));
    }

    // Compute minimum distance for all constellations
    for (auto& c : constellations) {
        auto cnst = get_constellation(c);
        float min_dist = numeric_limits<float>::max();
        for (unsigned int i = 0; i < cnst->arity(); ++i) {
            gr_complex sample;
            vector<float> metric(cnst->arity(), 0);
            cnst->map_to_points(i, &sample);
            cnst->calc_euclidean_metric(&sample, &metric[0]);
            metric[i] = numeric_limits<float>::max();
            auto m = *min_element(metric.begin(), metric.end());
            if (m < min_dist) {
                min_dist = m;
            }
        }
        d_min_dist[c] = min_dist;
    }

    // Build subcarrier mask
    for (auto& index : subcarriers) {
        if (index < 0) {
            d_subcarriers[index + fft_len] = true;
        } else {
            d_subcarriers[index] = true;
        }
    }
}

/*
 * Our virtual destructor.
 */
ofdm_adaptive_constellation_metric_vcvf_impl::
    ~ofdm_adaptive_constellation_metric_vcvf_impl()
{
}

float ofdm_adaptive_constellation_metric_vcvf_impl::get_cnst_min_distance(
    constellation_type_t cnst)
{
    auto it = d_min_dist.find(cnst);
    if (it == d_min_dist.end()) {
        return 0.0;
    }
    return it->second;
}


int ofdm_adaptive_constellation_metric_vcvf_impl::work(
    int noutput_items,
    gr_vector_int& ninput_items,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    auto in_est = static_cast<const gr_complex*>(input_items[0]);
    auto in_soft = static_cast<const gr_complex*>(input_items[1]);

    auto out = static_cast<float*>(output_items[0]);


    if (ninput_items[0] != ninput_items[1]) {
        throw(runtime_error("Input mismatch"));
    }

    std::vector<tag_t> tags;
    get_tags_in_window(tags, 0, 0, 1);
    auto cnst_tag_it = find_constellation_tag(tags);
    if (cnst_tag_it == tags.end()) {
        throw std::invalid_argument("Missing constellation tag.");
    }
    constellation_type_t cnst_type = get_constellation_type(*cnst_tag_it);
    if (d_min_dist.find(cnst_type) == d_min_dist.end()) {
        throw std::invalid_argument("Unknown constellation");
    }
    float min_dist = d_min_dist[cnst_type];

    DTL_LOG_DEBUG("work: n_syms={}, cnst={}, min_dist={}",
                  ninput_items[0],
                  static_cast<int>(cnst_type),
                  min_dist);
    for (int i = 0; i < ninput_items[0]; ++i) {
        fill(&out[i * d_fft_len], &out[(i + 1) * d_fft_len], 0.0);
        for (unsigned int k = 0; k < d_fft_len; ++k) {
            if (d_subcarriers[k]) {
                out[k] += norm(in_est[i * d_fft_len + k] - in_soft[i * d_fft_len + k]);
            }
        }
    }

    for (unsigned int k = 0; k < d_fft_len; ++k) {
        out[k] /= ninput_items[0];
        out[k] /= min_dist;
    }

    return 1;
}

} /* namespace dtl */
} /* namespace gr */
