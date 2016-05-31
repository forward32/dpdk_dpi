#include <string.h>
#include "common.h"

static constexpr auto http_min_len = 15;
static constexpr auto http_version = "HTTP/1.1";
static constexpr auto http_version_len = strlen(http_version);

static inline bool SearchHttpMethod(char *data, const char *method) {
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
  // Address + space
  while (*data++ != ' ');
  // Protocol version
  for (size_t i = 0; i < http_version_len; ++i) {
    if (*data++ != http_version[i]) {
      return false;
    }
  }

  return true;
}

protocol_type SearchHttp(rte_mbuf *m) {
  const uint16_t headers_len = m->l2_len + m->l3_len + m->l4_len;
  const uint16_t payload_len = m->pkt_len - headers_len;
  if (payload_len < http_min_len) return UNKNOWN;

  char *payload = rte_pktmbuf_mtod_offset(m, char *, headers_len);

  // Response case
  bool response = true;
  char *tmp_payload = payload;
  for (size_t i = 0; i < http_version_len; ++i) {
    if (*tmp_payload++ != http_version[i]) {
      response = false;
      break;
    }
  }
  if (response) {
    return HTTP;
  }

  // Methods
  if (SearchHttpMethod(payload, "OPTIONS")) return HTTP;
  if (SearchHttpMethod(payload, "GET")) return HTTP;
  if (SearchHttpMethod(payload, "HEAD")) return HTTP;
  if (SearchHttpMethod(payload, "POST")) return HTTP;
  if (SearchHttpMethod(payload, "PUT")) return HTTP;
  if (SearchHttpMethod(payload, "DELETE")) return HTTP;
  if (SearchHttpMethod(payload, "TRACE")) return HTTP;
  if (SearchHttpMethod(payload, "CONNECT")) return HTTP;

  return UNKNOWN;
}

