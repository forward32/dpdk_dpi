#include <rte_config.h>
#include <rte_mbuf.h>
#include <string.h>
#include "common.h"

static inline bool SearchSipMethod(uint8_t *data, const char *method_up, const char *method_lo) {
  const size_t method_len = strlen(method_up);

  return ((!memcmp(data, method_up, method_len) || !memcmp(data, method_lo, method_len)) &&
          (!memcmp(data + method_len + 1, "SIP:", 4) || !memcmp(data + method_len + 1, "sip:", 4)));
}

protocol_type SearchSip(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  // minimum 14 bytes
  constexpr uint8_t sip_min_len = 14;
  if (payload_len < sip_min_len) return UNKNOWN;

  uint8_t *payload = rte_pktmbuf_mtod_offset(m, uint8_t *, headers_len);

  // Response case
  if (!memcmp(payload, "SIP/2.0", 7) || !memcmp(payload, "sip/2.0", 7)) {
    return SIP;
  }

  // Methods
  if (SearchSipMethod(payload, "INVITE", "invite")) return SIP;
  if (SearchSipMethod(payload, "ACK", "ack")) return SIP;
  if (SearchSipMethod(payload, "BYE", "bye")) return SIP;
  if (SearchSipMethod(payload, "CANCEL", "cancel")) return SIP;
  if (SearchSipMethod(payload, "OPTIONS", "options")) return SIP;
  if (SearchSipMethod(payload, "REGISTER", "register")) return SIP;
  if (SearchSipMethod(payload, "PRACK", "prack")) return SIP;
  if (SearchSipMethod(payload, "SUBSCRIBE", "subscribe")) return SIP;
  if (SearchSipMethod(payload, "NOTIFY", "notify")) return SIP;
  if (SearchSipMethod(payload, "PUBLISH", "publish")) return SIP;
  if (SearchSipMethod(payload, "INFO", "info")) return SIP;
  if (SearchSipMethod(payload, "REFER", "refer")) return SIP;
  if (SearchSipMethod(payload, "MESSAGE", "message")) return SIP;
  if (SearchSipMethod(payload, "UPDATE", "update")) return SIP;

  // STUN and others case not supported

  return UNKNOWN;
}
