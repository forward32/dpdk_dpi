#include <string.h>
#include "common.h"

static constexpr auto sip_min_len = 14;
static constexpr auto sip_version = "SIP/2.0";
static constexpr auto sip_version_len = strlen(sip_version);
static constexpr auto sip_prefix_up = "SIP:";
static constexpr auto sip_prefix_lo = "sip:";
static constexpr auto sip_prefix_len = strlen(sip_prefix_up);

static inline bool SearchSipMethod(char *data, const char *method) {
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
  for (size_t i = 0; i < sip_prefix_len; ++i) {
    if (*data != sip_prefix_up[i]) {
      if (*data != sip_prefix_lo[i]) {
        return false;
      }
    }
    ++data;
  }

  return true;
}

protocol_type SearchSip(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  if (payload_len < sip_min_len) return UNKNOWN;

  char *payload = rte_pktmbuf_mtod_offset(m, char *, headers_len);

  // Response case
  bool response = true;
  char *tmp_payload = payload;
  for (size_t i = 0; i < sip_version_len; ++i) {
    if (*tmp_payload++ != sip_version[i]) {
      response = false;
      break;
    }
  }
  if (response) {
    return SIP;
  }

  // Methods
  if (SearchSipMethod(payload, "INVITE")) return SIP;
  if (SearchSipMethod(payload, "ACK")) return SIP;
  if (SearchSipMethod(payload, "BYE")) return SIP;
  if (SearchSipMethod(payload, "CANCEL")) return SIP;
  if (SearchSipMethod(payload, "OPTIONS")) return SIP;
  if (SearchSipMethod(payload, "REGISTER")) return SIP;
  if (SearchSipMethod(payload, "PRACK")) return SIP;
  if (SearchSipMethod(payload, "SUBSCRIBE")) return SIP;
  if (SearchSipMethod(payload, "NOTIFY")) return SIP;
  if (SearchSipMethod(payload, "PUBLISH")) return SIP;
  if (SearchSipMethod(payload, "INFO")) return SIP;
  if (SearchSipMethod(payload, "REFER")) return SIP;
  if (SearchSipMethod(payload, "MESSAGE")) return SIP;
  if (SearchSipMethod(payload, "UPDATE")) return SIP;

  return UNKNOWN;
}
