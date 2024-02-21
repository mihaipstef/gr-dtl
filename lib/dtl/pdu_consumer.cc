
#include "pdu_consumer.h"
#include <gnuradio/testbed/logger.h>
#include <utility>


namespace gr {
namespace dtl {


INIT_DTL_LOGGER("pdu_consumer");


pdu_consumer::pdu_consumer() : d_current_pdu_remain(0) {}


int pdu_consumer::advance(int ninput_items,
                          int payload_max,
                          int start_offset,
                          const std::function<std::optional<tag_t>(int)>& get_len_tag)
{
    int to_read = 0; // ninput_items[0] - read_index;
    while (start_offset + to_read < ninput_items) {
        std::vector<tag_t> tags;
        auto len_tag = get_len_tag(start_offset + to_read);
        if (!len_tag) {
            if (d_current_pdu_remain) {
                to_read = std::min(payload_max, d_current_pdu_remain);
                to_read = std::min(to_read, ninput_items - start_offset);
                d_current_pdu_remain -= to_read;
                DTL_LOG_DEBUG("jumbo consume: to_read={}, pdu_remain={}",
                              to_read,
                              d_current_pdu_remain);
            } else {
                DTL_LOG_DEBUG("Tag not found: to_read={}, pdu_remain={}",
                              to_read,
                              d_current_pdu_remain);
            }
            break;
        } else {
            int pdu_len = pmt::to_long(len_tag->value);
            if (to_read + pdu_len <= payload_max) {
                // Do not advance if there is not a full PDU in the buffer
                if (to_read + pdu_len <= ninput_items - start_offset) {
                    DTL_LOG_DEBUG("pdu detected: len={}", pdu_len);
                    to_read += pdu_len;
                } else {
                    break;
                }
            } else if (to_read == 0) {
                to_read = std::min(payload_max, ninput_items - start_offset);
                d_current_pdu_remain = pdu_len - to_read;
                DTL_LOG_DEBUG("jumbo pdu detected: len={}, to_read={}, remain={}",
                              pdu_len,
                              to_read,
                              d_current_pdu_remain);
                break;
            } else {
                // Stop. Do nothing!
                break;
            }
        }
    }
    return to_read;
}

} // namespace dtl
} // namespace gr