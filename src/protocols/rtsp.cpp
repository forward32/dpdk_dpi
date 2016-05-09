#include <string.h>
#include "common.h"

static inline bool SearchRtspMethod(uint8_t *data, const char *method_up, const char *method_lo) {
  const size_t method_len = strlen(method_up);

  return ((!memcmp(data, method_up, method_len) || !memcmp(data, method_lo, method_len)) &&
          (!memcmp(data + method_len + 1, "RTSP://", 7) || !memcmp(data + method_len + 1, "rtsp://", 7)));
}

protocol_type SearchRtsp(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  // minimum 15 bytes
  constexpr uint8_t rtsp_min_len = 15;
  if (payload_len < rtsp_min_len) return UNKNOWN;

  uint8_t *payload = rte_pktmbuf_mtod_offset(m, uint8_t *, headers_len);

  // Response case
  if (!memcmp(payload, "RTSP/1.0", 8) || !memcmp(payload, "rtsp/1.0", 8)) {
    return RTSP;
  }

  // Methods
  if (SearchRtspMethod(payload, "DESCRIBE", "describe")) return RTSP;
  if (SearchRtspMethod(payload, "OPTIONS", "options")) return RTSP;
  if (SearchRtspMethod(payload, "PLAY", "play")) return RTSP;
  if (SearchRtspMethod(payload, "PAUSE", "pause")) return RTSP;
  if (SearchRtspMethod(payload, "RECORD", "record")) return RTSP;
  if (SearchRtspMethod(payload, "REDIRECT", "redirect")) return RTSP;
  if (SearchRtspMethod(payload, "SETUP", "setup")) return RTSP;
  if (SearchRtspMethod(payload, "ANNOUNCE", "announce")) return RTSP;
  if (SearchRtspMethod(payload, "GET_PARAMETER", "get_parameter")) return RTSP;
  if (SearchRtspMethod(payload, "SET_PARAMETER", "set_parameter")) return RTSP;
  if (SearchRtspMethod(payload, "TEARDOWN", "teardown")) return RTSP;

  return UNKNOWN;
}
