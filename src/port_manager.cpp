#include <rte_config.h>
#include <rte_ethdev.h>
#include <glog/logging.h>
#include "port_manager.h"

void PortManager::Initialize() {
  auto nb_ports = rte_eth_dev_count();
  LOG(INFO) << "Number of ports: " << (uint16_t)nb_ports;
}
