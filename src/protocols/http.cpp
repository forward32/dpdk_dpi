#include <string.h>
#include "common.h"

static inline bool SearchHttpMethod(uint8_t *data, const char *method_up, const char *method_lo) {
  const size_t method_len = strlen(method_up);
  //  Method type
  if (memcmp(data, method_up, method_len) && memcmp(data, method_lo, method_len)) {
    return false;
  }
  // Space
  data += method_len;
  if (memcmp(data, " ", 1)) {
    return false;
  }
  // Address + space
  while (memcmp(++data, " ", 1));
  // Protocol number
  if (memcmp(++data, "HTTP/1.1", 8) && memcmp(data, "http/1.1", 8)) {
    return false;
  }

  return true;
}

protocol_type SearchHttp(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  // minimum 15 bytes
  constexpr uint8_t http_min_len = 15;
  if (payload_len < http_min_len) return UNKNOWN;

  uint8_t *payload = rte_pktmbuf_mtod_offset(m, uint8_t *, headers_len);

  // Response case
  if (!memcmp(payload, "HTTP/1.1", 8) || !memcmp(payload, "http/1.1", 8)) {
    return HTTP;
  }

  // Methods
  if (SearchHttpMethod(payload, "OPTIONS", "options")) return HTTP;
  if (SearchHttpMethod(payload, "GET", "get")) return HTTP;
  if (SearchHttpMethod(payload, "HEAD", "head")) return HTTP;
  if (SearchHttpMethod(payload, "POST", "post")) return HTTP;
  if (SearchHttpMethod(payload, "PUT", "put")) return HTTP;
  if (SearchHttpMethod(payload, "DELETE", "delete")) return HTTP;
  if (SearchHttpMethod(payload, "TRACE", "trace")) return HTTP;
  if (SearchHttpMethod(payload, "CONNECT", "connect")) return HTTP;

  return UNKNOWN;
}

