/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */

#include "core/reactor.hh"
#include "packet.hh"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace net {

constexpr size_t packet::internal_data_size;
constexpr size_t packet::default_nr_frags;

void packet::linearize(size_t at_frag, size_t desired_size) {
    _impl->unuse_internal_data();
    size_t nr_frags = 0;
    size_t accum_size = 0;
    while (accum_size < desired_size) {
        accum_size += _impl->_frags[at_frag + nr_frags].size;
        ++nr_frags;
    }
    std::unique_ptr<char[]> new_frag{new char[accum_size]};
    auto p = new_frag.get();
    for (size_t i = 0; i < nr_frags; ++i) {
        auto& f = _impl->_frags[at_frag + i];
        p = std::copy(f.base, f.base + f.size, p);
    }
    // collapse nr_frags into one fragment
    std::copy(_impl->_frags + at_frag + nr_frags, _impl->_frags + _impl->_nr_frags,
            _impl->_frags + at_frag + 1);
    _impl->_nr_frags -= nr_frags - 1;
    _impl->_frags[at_frag] = fragment{new_frag.get(), accum_size};
    _impl->_deleter = make_deleter(std::move(_impl->_deleter), [buf = std::move(new_frag)] {});
}


packet packet::free_on_cpu(unsigned cpu)
{
    // make new deleter that runs old deleter on an origin cpu
    _impl->_deleter = make_deleter(deleter(), [d = std::move(_impl->_deleter), cpu] () mutable {
        smp::submit_to(cpu, [d = std::move(d)] () mutable {
            // deleter needs to be moved from lambda capture to be destroyed here
            // otherwise deleter destructor will be called on a cpu that called smp::submit_to()
            // when work_item is destroyed.
            deleter xxx(std::move(d));
        });
    });

    return packet(impl::copy(_impl.get()));
}

std::ostream& operator<<(std::ostream& os, const packet& p) {
    os << "packet{";
    bool first = true;
    for (auto&& frag : p.fragments()) {
        if (!first) {
            os << ", ";
        }
        first = false;
        if (std::all_of(frag.base, frag.base + frag.size, [] (int c) { return c >= 9 && c <= 0x7f; })) {
            os << '"';
            for (auto p = frag.base; p != frag.base + frag.size; ++p) {
                auto c = *p;
                if (isprint(c)) {
                    os << c;
                } else if (c == '\r') {
                    os << "\\r";
                } else if (c == '\n') {
                    os << "\\n";
                } else if (c == '\t') {
                    os << "\\t";
                } else {
                    uint8_t b = c;
                    os << "\\x" << (b / 16) << (b % 16);
                }
            }
            os << '"';
        } else {
            os << "{";
            bool nfirst = true;
            for (auto p = frag.base; p != frag.base + frag.size; ++p) {
                if (!nfirst) {
                    os << " ";
                }
                nfirst = false;
                uint8_t b = *p;
                os << sprint("%02x", unsigned(b));
            }
            os << "}";
        }
    }
    os << "}";
    return os;
}

}
