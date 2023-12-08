/* -*- c++ -*- */
/*
 * Copyright 2022 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_TAG_UTILITIES_H
#define INCLUDED_DTL_TAG_UTILITIES_H

#include <map>

namespace gr {
namespace dtl {


template <typename... A>
struct _if_my_tag;

template <typename H, typename... T>
struct _if_my_tag<H, T...> {
    static void append(std::map<pmt::pmt_t, tag_t>& result, const tag_t& tag, const H& head, const T&... keys) {
        if (head == tag.key) {
            result.insert(make_pair(head, tag));
        }
        _if_my_tag<T...>::append(result, tag, keys...);
    }
};

template <>
struct _if_my_tag<> {
    static void append(std::map<pmt::pmt_t, tag_t>& result, const tag_t& tag) {}
};


template <typename... T>
std::map<pmt::pmt_t, tag_t>
get_tags(const std::vector<tag_t>& tags, const T&... keys) {
    std::map<pmt::pmt_t, tag_t> result;
    for (auto& t: tags) {
        _if_my_tag<T...>::append(result, t, keys...);
        if (result.size() == sizeof...(T)) {
            break;
        }
    }
    return result;
}

}
}

#endif