#ifndef COMMON_
#define COMMON_

#include <unordered_map>
#include <rte_config.h>
#include <rte_mbuf.h>

enum protocol_type: uint8_t {
  FTP = 0,
  SIP,
  RTP,
  RTSP,
  UNKNOWN,
};

static std::unordered_map<std::string, protocol_type> protocol_map = {
  {"FTP", FTP},
  {"SIP", SIP},
  {"RTP", RTP},
  {"RTSP", RTSP},
  {"UNKNOWN", UNKNOWN},
};

enum action_type: uint8_t {
  PUSH_VLAN = 0,
  PUSH_MPLS,
  OUTPUT
};

static std::unordered_map<uint8_t, uint8_t> action_priority = {
  {PUSH_VLAN, 1},
  {PUSH_MPLS, 1},
  {OUTPUT, 2},
};

bool PreparePacket(rte_mbuf *);

#endif // COMMON_
