#ifndef COMMON_
#define COMMON_

#include <unordered_map>

enum protocol_type: uint8_t {
  FTP,
  SIP,
  RSTP
};

static std::unordered_map<std::string, uint8_t> protocol_map = {
  {"FTP", FTP},
  {"SIP", SIP},
  {"RSTP", RSTP},
};

enum action_type: uint8_t {
  PUSH_VLAN,
  PUSH_MPLS,
  OUTPUT
};

static std::unordered_map<uint8_t, uint8_t> action_priority = {
  {PUSH_VLAN, 1},
  {PUSH_MPLS, 2},
  {OUTPUT, 3},
};

#endif // COMMON_
