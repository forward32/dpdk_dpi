#include <fstream>
#include <algorithm>
#include <glog/logging.h>
#include "config.h"

Config::Config(const std::string &file_name) : config_name_(file_name) {
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

bool Config::ParsePortAndProtocol(uint16_t &rule_key, std::string &str) {
  auto pos = str.find(",");
  if (pos == std::string::npos) {
    LOG(ERROR) << "Delimeter ',' not found";
    return false;
  }

  std::string port_s = str.substr(0, pos);
  port_s.erase(std::remove(port_s.begin(), port_s.end(), ' '), port_s.end());
  std::string protocol_s = str.substr(pos+1);
  protocol_s.erase(std::remove(protocol_s.begin(), protocol_s.end(), ' '), protocol_s.end());

  uint8_t port;
  try {
    size_t end_pos;
    auto ret = std::stoul(port_s.c_str(), &end_pos, 10);
    if (end_pos != port_s.length()) {
      LOG(ERROR) << "Can't convert port, value=" << port_s;
      return false;
    }
    port = (uint8_t)ret;
  }
  catch (const std::logic_error &err) {
    LOG(ERROR) << "Exception: " << err.what();
    return false;
  }

  if (protocol_map.find(protocol_s) == protocol_map.end()) {
    LOG(ERROR) << "Unknown or invalid protocol, value=" << protocol_s;
    return false;
  }
  uint8_t protocol = protocol_map[protocol_s];

  DLOG(INFO) << "Port:" << (uint16_t)port << ",protocol:" << protocol_s;

  rule_key = port | (protocol << 8);

  return true;
}

bool Config::ParseActions(Actions &actions, std::string &str) {
  static const std::string drop_action_s = "DROP";
  static const std::string push_vlan_action_s = "PUSH_VLAN(";
  static const std::string push_mpls_action_s = "PUSH_MPLS(";
  static const std::string output_action_s = "OUTPUT(";

  // Small trick
  if (str.length() > 0) {
    str += ",";
  }

  size_t pos;
  while((pos = str.find(",")) != std::string::npos) {
    std::string action_s = str.substr(0, pos);
    action_s.erase(std::remove(action_s.begin(), action_s.end(), ' '), action_s.end());
    auto action_s_len = action_s.length();

    if (action_s.find(drop_action_s) != std::string::npos) {
      if (action_s_len != drop_action_s.length()) {
        LOG(ERROR) << "Invalid drop action, value=" << action_s;
        return false;
      }
      DropAction *drop_action = new DropAction;
      drop_action->type = DROP;
      Action *action = reinterpret_cast<Action*>(drop_action);
      actions.push_back(std::shared_ptr<Action>(action));
      DLOG(INFO) << "Action DROP";
    }

    else if (action_s.find(push_vlan_action_s) != std::string::npos) {
    }

    else if (action_s.find(push_mpls_action_s) != std::string::npos) {
    }

    else if (action_s.find(output_action_s) != std::string::npos) {
      auto output_action_s_len = output_action_s.length();
      if (action_s.substr(0, output_action_s_len) != output_action_s || action_s[action_s_len-1] != ')') {
        LOG(ERROR) << "Invalid output action, value=" << action_s;
        return false;
      }
      std::string port_id_s = action_s.substr(output_action_s_len, action_s_len-output_action_s_len-1);
      uint8_t port_id;
      try {
        size_t end_pos;
        auto ret = std::stoul(port_id_s.c_str(), &end_pos, 10);
        if (end_pos != port_id_s.length()) {
          LOG(ERROR) << "Can't convert output port_id, value=" << port_id_s;
          return false;
        }
        port_id = (uint8_t)ret;
      }
      catch (const std::logic_error &err) {
        LOG(ERROR) << "Exception: " << err.what();
        return false;
      }
      OutputAction *output_action = new OutputAction;
      output_action->type = OUTPUT;
      output_action->port_id = port_id;
      Action *action = reinterpret_cast<Action*>(output_action);
      actions.push_back(std::shared_ptr<Action>(action));
      DLOG(INFO) << "Action OUTPUT, port_id=" << (uint16_t)port_id;
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
