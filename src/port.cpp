#include "port.h"
#include <rte_ethdev.h>

PortBase::PortBase(const uint8_t port_id) : port_id_(port_id) {
  memset(&protocol_stats_, 0, sizeof(protocol_stats_));
}

uint8_t PortBase::GetPortId() const {
  return port_id_;
}

void PortBase::UpdateProtocolStats(const protocol_type protocol, const unsigned lcore_id) {
  switch (protocol) {
    case HTTP: {
      protocol_stats_[lcore_id].http.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case SIP: {
      protocol_stats_[lcore_id].sip.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case RTP: {
      protocol_stats_[lcore_id].rtp.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case RTSP: {
      protocol_stats_[lcore_id].rtsp.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case UNKNOWN:
    default: {
      break;
    }
  }
}

uint64_t PortBase::GetProtocolStats(const protocol_type protocol) const {
  uint64_t ret = 0;

  switch (protocol) {
    case HTTP: {
      for (uint8_t i = 0; i < kMAX_LCORES; ++i) {
        ret += protocol_stats_[i].http.load(std::memory_order_relaxed);
      }
      break;
    }
    case SIP: {
      for (uint8_t i = 0; i < kMAX_LCORES; ++i) {
        ret += protocol_stats_[i].sip.load(std::memory_order_relaxed);
      }
      break;
    }
    case RTP: {
      for (uint8_t i = 0; i < kMAX_LCORES; ++i) {
        ret += protocol_stats_[i].rtp.load(std::memory_order_relaxed);
      }
      break;
    }
    case RTSP: {
      for (uint8_t i = 0; i < kMAX_LCORES; ++i) {
        ret += protocol_stats_[i].rtsp.load(std::memory_order_relaxed);
      }
      break;
    }
    case UNKNOWN:
    default: {
      break;
    }
  }

  return ret;
}


PortEthernet::PortEthernet(const uint8_t port_id) : PortBase(port_id) {
}

void PortEthernet::SendOnePacket(rte_mbuf *m, PortQueue *queue) {
  queue->queue_[queue->count_++] = m;

  if (queue->count_ == kMAX_PKTS_IN_QUEUE) {
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
  queue->count_ = rte_eth_rx_burst(GetPortId(), 0, queue->queue_, kMAX_PKTS_IN_QUEUE);
}
