#include "port.h"

PortBase::PortBase(const uint8_t port_id) : port_id_(port_id) {
}


PortEthernet::PortEthernet(const uint8_t port_id) : PortBase(port_id) {
}

void PortEthernet::SendOnePacket(rte_mbuf *m) {
}

void PortEthernet::SendAllPackets() {
}

uint16_t PortEthernet::ReceivePackets(rte_mbuf **m) {
  return 0;
}
