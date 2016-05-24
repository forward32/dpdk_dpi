#ifndef COMMON_
#define COMMON_

#include <unordered_map>
#include <rte_config.h>
#include <rte_mbuf.h>

enum protocol_type: uint8_t {
  HTTP,
  SIP,
  RTP,
  RTSP,
  UNKNOWN,
};

static std::unordered_map<std::string, protocol_type> protocol_map = {
  {"HTTP", HTTP},
  {"SIP", SIP},
  {"RTP", RTP},
  {"RTSP", RTSP},
  {"UNKNOWN", UNKNOWN},
};

enum action_type: uint8_t {
  DROP,
  PUSH_VLAN,
  PUSH_MPLS,
  OUTPUT,
};

static std::unordered_map<uint8_t, uint8_t> action_priority = {
  {DROP, 0},
  {PUSH_VLAN, 1},
  {PUSH_MPLS, 1},
  {OUTPUT, 2},
};

bool ParseInt(const std::string &, unsigned long &);

namespace packet_modifier {
  bool PreparePacket(rte_mbuf *);
  void ExecutePushVlan(rte_mbuf *, const uint32_t);
  void ExecutePushMpls(rte_mbuf *, const uint32_t);
}

#endif // COMMON_
