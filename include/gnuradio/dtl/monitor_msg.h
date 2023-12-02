/* -*- c++ -*- */
/*
 * Copyright 2023 DTL.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DTL_MONITOR_MSG_H
#define INCLUDED_DTL_MONITOR_MSG_H

#include <pmt/pmt.h>
#include <iostream>

namespace gr {
namespace dtl {

template <typename T> struct is_string: std::false_type {};
template <> struct is_string<std::string>: std::true_type {};
template <> struct is_string<const char *>: std::true_type {};

template <typename T, bool I, bool F, bool S, bool P>
struct _to_pmt {
    static pmt::pmt_t value(T v) {
        throw std::runtime_error("Not implemented");
    }
};

template <typename T>
struct _to_pmt<T, true, false, false, false>{
    static pmt::pmt_t value(T v) {
        return pmt::from_long(v);
    }
};

template <typename T>
struct _to_pmt<T, false, true, false, false>{
    static pmt::pmt_t value(T v) {
        return pmt::from_float(v);
    }
};

template <typename T>
struct _to_pmt<T, false, false, true, false>{
    static pmt::pmt_t value(T v) {
        return pmt::string_to_symbol(v);
    }
};

template <typename T>
struct _to_pmt<T, false, false, false, true>{
    static pmt::pmt_t value(T v) {
        return v;
    }
};

template <class T>
pmt::pmt_t to_pmt(T v) {
    return _to_pmt<
        T,
        std::is_integral<T>::value,
        std::is_floating_point<T>::value,
        is_string<T>::value,
        std::is_same<T, pmt::pmt_t>::value
        >::value(v);
}

uint64_t system_ts();

template <class... P>
pmt::pmt_t monitor_msg(P ...pairs) {
    uint64_t ts = system_ts();
    pmt::pmt_t msg = pmt::dict_add(
        pmt::make_dict(),
        to_pmt("time"),
        pmt::from_uint64(ts)
    );
    auto add_kv = [&msg](const auto& kv) {
        msg = pmt::dict_add(msg, to_pmt(kv.first), to_pmt(kv.second));
    };
    (add_kv(pairs), ...);
    return msg;
}


// template <class... P>
// pmt::pmt_t monitor_msg_b(P ...pairs) {
//     pmt::pmt_t msg = monitor_msg(pairs...);
//     std::stringbuf sb;
//     pmt::serialize(msg, sb);
//     std::string s = sb.str();
//     return pmt::init_u8vector(s.size(), (uint8_t*)s.c_str());
// }


} // namespace dtl
} // namespace gr

#endif