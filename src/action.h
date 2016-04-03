#ifndef ACTION_
#define ACTION_

#include "common.h"

struct Action {
  action_type type;
};

struct PushVlanAction {
  action_type type;
  uint16_t tpid;
  uint8_t pcp;
  uint8_t cfi;
  uint16_t vid;
};

struct PushMplsAction {
  action_type type;
  uint32_t label;
  uint8_t exp;
  uint8_t ttl;
};

struct OutputAction {
  action_type type;
  uint8_t port_id;
};

#endif // ACTION_
