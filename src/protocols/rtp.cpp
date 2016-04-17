#include <rte_config.h>
#include <rte_mbuf.h>
#include "common.h"

protocol_type SearchRtp(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  // minimum 12 bytes
  uint16_t rtp_min_len = 12;
  if (payload_len < rtp_min_len) return UNKNOWN;

  uint8_t *payload = rte_pktmbuf_mtod_offset(m, uint8_t *, headers_len);
  // current version is 2
  if (!(payload[0] & 0x80)) return UNKNOWN;

  uint32_t ssrc = ((uint32_t *)(payload))[8];
  // ssrc can't be 0
  if (ssrc == 0) return UNKNOWN;

  uint8_t payload_type = payload[1] & 0x7f;
  // RFC 3551: valid are 0-34 and 96-127
  if ((payload_type > 34 && payload_type < 96) || (payload_type > 127)) return UNKNOWN;

  uint8_t csrc_count = payload[0] & 0x0f;
  rtp_min_len += csrc_count*4;
  if (payload_len < rtp_min_len) return UNKNOWN;

  uint8_t extension = payload[0] & 0x10;
  if (extension) {
    // profile-specific id
    rtp_min_len += 2;
    uint16_t extension_len = *(uint16_t *)(payload + rtp_min_len);
    // extension header len
    rtp_min_len += 2;

    if (payload_len < rtp_min_len + extension_len) return UNKNOWN;
  }

  return RTP;
}
