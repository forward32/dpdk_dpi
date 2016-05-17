#ifndef ACTION_
#define ACTION_

#include "common.h"

struct Action {
  action_type type;
};

struct PushVlanAction {
  action_type type;
  uint32_t vlan_tag;
};

struct PushMplsAction {
  action_type type;
  uint32_t mpls_label;
};

struct OutputAction {
  action_type type;
  uint8_t port_id;
};

#endif // ACTION_
