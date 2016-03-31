#include "port.h"
#include <rte_ethdev.h>

PortBase::PortBase(const uint8_t port_id) : port_id_(port_id) {
}

uint8_t PortBase::GetPortId() const {
  return port_id_;
}


PortEthernet::PortEthernet(const uint8_t port_id) : PortBase(port_id) {
}

void PortEthernet::SendOnePacket(rte_mbuf *m, PortQueue *queue) {
  queue->queue_[queue->count_++] = m;

  if (queue->count_ == MAX_PKTS_IN_QUEUE) {
    SendAllPackets(queue);
  }
}

void PortEthernet::SendAllPackets(PortQueue *queue) {
  if (queue->count_ == 0) {
    return;
  }

  uint16_t sended = 0;
  while (sended != queue->count_) {
    auto ret = rte_eth_tx_burst(GetPortId(), 0, queue->queue_ + sended, queue->count_ - sended);
    if (ret == 0) {
      break;
    }
    sended += ret;
  }

  if (sended < queue->count_) {
    do {
      rte_pktmbuf_free(queue->queue_[sended]);
    } while (++sended < queue->count_);
  }

  queue->count_ = 0;
}

void PortEthernet::ReceivePackets(PortQueue *queue) {
  queue->count_ = rte_eth_rx_burst(GetPortId(), 0, queue->queue_, MAX_PKTS_IN_QUEUE);
}
