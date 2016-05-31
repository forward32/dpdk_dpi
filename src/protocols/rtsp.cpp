#include <string.h>
#include "common.h"

static constexpr auto rtsp_min_len = 15;
static constexpr auto rtsp_version = "RTSP/1.0";
static constexpr auto rtsp_version_len = strlen(rtsp_version);
static constexpr auto rtsp_prefix_up = "RTSP://";
static constexpr auto rtsp_prefix_lo = "rtsp://";
static constexpr auto rtsp_prefix_len = strlen(rtsp_prefix_up);

static inline bool SearchRtspMethod(char *data, const char *method) {
  const size_t method_len = strlen(method);
  // Method type
  for (size_t i = 0; i < method_len; ++i) {
    if (*data++ != method[i]) {
      return false;
    }
  }
  // Space
  if (*data++ != ' ') {
    return false;
  }
  // Protocol prefix
  for (size_t i = 0; i < rtsp_prefix_len; ++i) {
    if (*data != rtsp_prefix_up[i]) {
      if (*data != rtsp_prefix_lo[i]) {
        return false;
      }
    }
    ++data;
  }

  return true;
}

protocol_type SearchRtsp(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  if (payload_len < rtsp_min_len) return UNKNOWN;

  char *payload = rte_pktmbuf_mtod_offset(m, char *, headers_len);

  // Response case
  bool response = true;
  char *tmp_payload = payload;
  for (size_t i = 0; i < rtsp_version_len; ++i) {
    if (*tmp_payload++ != rtsp_version[i]) {
      response = false;
      break;
    }
  }
  if (response) {
    return RTSP;
  }

  // Methods
  if (SearchRtspMethod(payload, "DESCRIBE")) return RTSP;
  if (SearchRtspMethod(payload, "OPTIONS")) return RTSP;
  if (SearchRtspMethod(payload, "PLAY")) return RTSP;
  if (SearchRtspMethod(payload, "PAUSE")) return RTSP;
  if (SearchRtspMethod(payload, "RECORD")) return RTSP;
  if (SearchRtspMethod(payload, "REDIRECT")) return RTSP;
  if (SearchRtspMethod(payload, "SETUP")) return RTSP;
  if (SearchRtspMethod(payload, "ANNOUNCE")) return RTSP;
  if (SearchRtspMethod(payload, "GET_PARAMETER")) return RTSP;
  if (SearchRtspMethod(payload, "SET_PARAMETER")) return RTSP;
  if (SearchRtspMethod(payload, "TEARDOWN")) return RTSP;

  return UNKNOWN;
}
