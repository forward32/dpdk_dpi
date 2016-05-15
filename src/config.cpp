#include <fstream>
#include <algorithm>
#include <glog/logging.h>
#include "config.h"

Config::Config(const std::string &file_name) : config_name_(file_name) {
}

Config::~Config() {
  for (auto it_map = rules_.cbegin(); it_map != rules_.cend(); ++it_map) {
    for (auto it_vec = it_map->second.cbegin(); it_vec != it_map->second.cend(); ++it_vec) {
      delete *it_vec;
    }
  }
}

bool Config::Initialize() {
  std::ifstream config(config_name_);

  if (!config.is_open()) {
    LOG(ERROR) << "Can't open config file " << config_name_;
    return false;
  }

  uint16_t line_counter = 1;
  std::string rule;
  while (std::getline(config, rule)) {
    rule.erase(std::remove(rule.begin(), rule.end(), ' '), rule.end());
    auto pos = rule.find(":");
    if (pos == std::string::npos) {
      LOG(ERROR) << "Parsing error: line " << line_counter;
      return false;
    }

    std::string left_part = rule.substr(0, pos);
    std::string right_part = rule.substr(pos+1);

    uint16_t rule_key = 0;
    if (!ParsePortAndProtocol(rule_key, left_part)) {
      LOG(ERROR) << "Can't parse port and protocol: line " << line_counter;
      return false;
    }

    if (rules_.find(rule_key) != rules_.end()) {
      LOG(ERROR) << "Invalid config - overlapping: line " << line_counter;
      return false;
    }

    Actions actions;
    if (!ParseActions(actions, right_part)) {
      LOG(ERROR) << "Can't parse actions: line " << line_counter;
      return false;
    }

    rules_[rule_key] = actions;
    ++line_counter;
  }

  LOG(INFO) << "Number of rules: " << rules_.size();

  return true;
}

void Config::GetActions(const uint16_t rule_key, Actions *&actions) {
  auto it = rules_.find(rule_key);
  actions = it != rules_.end() ? &it->second:nullptr;
}

bool Config::ParsePortAndProtocol(uint16_t &rule_key, std::string &str) {
  auto pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse port and protocol (delimeter not found)";
    return false;
  }

  std::string port_s = str.substr(0, pos);
  unsigned long port_id;
  if (!ParseInt(port_s, port_id)) {
    LOG(ERROR) << "Can't convert port, value=" << port_s;
    return false;
  }

  std::string protocol_s = str.substr(pos+1);
  if (protocol_map.find(protocol_s) == protocol_map.end()) {
    LOG(ERROR) << "Unknown or invalid protocol, value=" << protocol_s;
    return false;
  }
  uint8_t protocol = protocol_map[protocol_s];

  DLOG(INFO) << "Port:" << port_id << ",protocol:" << protocol_s;

  rule_key = (uint8_t)port_id | (protocol << 8);

  return true;
}

bool Config::ParseActions(Actions &actions, std::string &str) {
  static const std::string push_vlan_prefix = "PUSH-VLAN(";
  static const std::string push_mpls_prefix = "PUSH-MPLS(";
  static const std::string output_prefix = "OUTPUT(";

  size_t pos;
  while((pos = str.find(";")) != std::string::npos) {
    std::string action_s = str.substr(0, pos);
    auto action_s_len = action_s.length();

    if (action_s.find(push_vlan_prefix) != std::string::npos) {
      auto push_vlan_prefix_len = push_vlan_prefix.length();
      if (action_s.substr(0, push_vlan_prefix_len) != push_vlan_prefix || action_s[action_s_len-1] != ')') {
        LOG(ERROR) << "Invalid PUSH-VLAN action, value=" << action_s;
        return false;
      }
      action_s = action_s.substr(push_vlan_prefix_len, action_s_len-push_vlan_prefix_len-1);
      uint16_t tpid, vid;
      uint8_t pcp, cfi;
      if (!ParseVlanData(tpid, pcp, cfi, vid, action_s)) {
        return false;
      }
      PushVlanAction *push_vlan_action = new PushVlanAction;
      push_vlan_action->type = PUSH_VLAN;
      push_vlan_action->tpid = tpid;
      push_vlan_action->pcp = pcp;
      push_vlan_action->cfi = cfi;
      push_vlan_action->vid = vid;
      Action *action = reinterpret_cast<Action*>(push_vlan_action);
      actions.push_back(action);
      DLOG(INFO) << "Action PUSH-VLAN, tpid=" << tpid << ",pcp=" << (uint16_t)pcp << ",cfi=" << (uint16_t)cfi << ",vid=" << vid;
    }

    else if (action_s.find(push_mpls_prefix) != std::string::npos) {
      auto push_mpls_prefix_len = push_mpls_prefix.length();
      if (action_s.substr(0, push_mpls_prefix_len) != push_mpls_prefix || action_s[action_s_len-1] != ')') {
        LOG(ERROR) << "Invalid PUSH-MPLS action, value=" << action_s;
        return false;
      }
      action_s = action_s.substr(push_mpls_prefix_len, action_s_len-push_mpls_prefix_len-1);
      uint32_t label;
      uint8_t exp, ttl;
      if (!ParseMplsData(label, exp, ttl, action_s)) {
        return false;
      }
      PushMplsAction *push_mpls_action = new PushMplsAction;
      push_mpls_action->type = PUSH_MPLS;
      push_mpls_action->label = label;
      push_mpls_action->exp = exp;
      push_mpls_action->ttl = ttl;
      Action *action = reinterpret_cast<Action*>(push_mpls_action);
      actions.push_back(action);
      DLOG(INFO) << "Action PUSH-MPLS, label=" << label << ",exp=" << (uint16_t)exp << ",ttl=" << (uint16_t)ttl;
    }

    else if (action_s.find(output_prefix) != std::string::npos) {
      auto output_prefix_len = output_prefix.length();
      if (action_s.substr(0, output_prefix_len) != output_prefix || action_s[action_s_len-1] != ')') {
        LOG(ERROR) << "Invalid OUTPUT action, value=" << action_s;
        return false;
      }
      std::string port_id_s = action_s.substr(output_prefix_len, action_s_len-output_prefix_len-1);
      unsigned long port_id;
      if (!ParseInt(port_id_s, port_id)) {
        LOG(ERROR) << "Can't convert output port_id, value=" << port_id_s;
        return false;
      }
      OutputAction *output_action = new OutputAction;
      output_action->type = OUTPUT;
      output_action->port_id = (uint8_t)port_id;
      Action *action = reinterpret_cast<Action*>(output_action);
      actions.push_back(action);
      DLOG(INFO) << "Action OUTPUT, port_id=" << port_id;
    }

    else {
      LOG(ERROR) << "Unknown or invalid action, value=" << action_s;
      return false;
    }

    str = str.substr(pos+1);
  }

  if (actions.empty()) {
    LOG(ERROR) << "Action list is empty";
    return false;
  }

  auto last_priority = action_priority[actions[0]->type];
  for (unsigned i = 1; i < actions.size(); ++i) {
    auto current_priority = action_priority[actions[i]->type];
    if (current_priority < last_priority) {
      LOG(ERROR) << "Invalid actions order";
      return false;
    }
    last_priority = current_priority;
  }

  return true;
}

bool Config::ParseVlanData(uint16_t &tpid, uint8_t &pcp, uint8_t &cfi, uint16_t &vid, std::string &str) {
  auto pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse vlan tpid (delimeter not found)";
    return false;
  }
  std::string tpid_s = str.substr(0, pos);
  unsigned long ret;
  if (!ParseInt(tpid_s, ret)) {
    LOG(ERROR) << "Can't convert vlan tpid, value=" << tpid_s;
    return false;
  }
  tpid = (uint16_t)ret;
  if (tpid != 0x8100 && tpid != 0x88a8) {
    LOG(ERROR) << "Invalid vlan tpid value=" << tpid;
    return false;
  }

  str = str.substr(pos+1);
  pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse vlan pcp (delimeter not found)";
    return false;
  }
  std::string pcp_s = str.substr(0, pos);
  if (!ParseInt(pcp_s, ret)) {
    LOG(ERROR) << "Can't convert vlan pcp, value=" << pcp_s;
    return false;
  }
  pcp = (uint8_t)ret;
  if (pcp > 7) {
    LOG(ERROR) << "Invalid vlan pcp value=" << (uint16_t)pcp;
    return false;
  }

  str = str.substr(pos+1);
  pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse vlan cfi (delimeter not found)";
    return false;
  }
  std::string cfi_s = str.substr(0, pos);
  if (!ParseInt(cfi_s, ret)) {
    LOG(ERROR) << "Can't convert vlan cfi, value=" << cfi_s;
    return false;
  }
  cfi = (uint8_t)ret;
  if (cfi > 1) {
    LOG(ERROR) << "Invalid vlan cfi value=" << (uint16_t)cfi;
    return false;
  }

  std::string vid_s = str.substr(pos+1);
  if (!ParseInt(vid_s, ret)) {
    LOG(ERROR) << "Can't convert vlan vid, value=" << vid_s;
    return false;
  }
  vid = (uint16_t)ret;
  if (vid > 4094) {
    LOG(ERROR) << "Invalid vlan vid value=" << vid;
    return false;
  }

  return true;
}

bool Config::ParseMplsData(uint32_t &label, uint8_t &exp, uint8_t &ttl, std::string &str) {
  auto pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse mpls label (delimeter not found)";
    return false;
  }
  std::string label_s = str.substr(0, pos);
  unsigned long ret;
  if (!ParseInt(label_s, ret)) {
    LOG(ERROR) << "Can't convert mpls label, value=" << label_s;
    return false;
  }
  label = (uint32_t)ret;

  str = str.substr(pos+1);
  pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Can't parse mpls exp (delimeter not found)";
    return false;
  }
  std::string exp_s = str.substr(0, pos);
  if (!ParseInt(exp_s, ret)) {
    LOG(ERROR) << "Can't convert mpls exp, value=" << exp_s;
    return false;
  }
  exp = (uint8_t)ret;
  if (exp > 7) {
    LOG(ERROR) << "Invalid mpls exp value=" << (uint16_t)exp;
    return false;
  }

  std::string ttl_s = str.substr(pos+1);
  if (!ParseInt(ttl_s, ret)) {
    LOG(ERROR) << "Can't convert mpls ttl, value=" << ttl_s;
    return false;
  }
  ttl = (uint8_t)ret;

  return true;
}
