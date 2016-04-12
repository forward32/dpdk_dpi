#include <atomic>
#include <cassert>
#include <glog/logging.h>
#include "packet_manager.h"
#include <rte_cycles.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>

extern std::atomic<bool> terminated;

#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

#define ETHER_TYPE_VLAN_8021AD 0x88a8

/*
 * Additional functions
 */
static bool PreparePacket(rte_mbuf *m) {
  char *pkt_data = rte_ctrlmbuf_data(m);
  uint16_t *eth_type = (uint16_t *)(pkt_data + 2*ETHER_ADDR_LEN);
  m->l2_len = 2*ETHER_ADDR_LEN;

  // Skip VLAN tags
  while (*eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN) ||
         *eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN_8021AD)) {
    eth_type += 2;
    m->l2_len += 4;
  }
  m->l2_len += 2;

  // If it's not IP packet - skip it
  uint8_t ip_proto = 0;
  switch (rte_cpu_to_be_16(*eth_type)) {
    case ETHER_TYPE_IPv4: {
      m->l3_len = sizeof(ipv4_hdr);
      ip_proto = ((ipv4_hdr *)(eth_type + 1))->next_proto_id;
      break;
    }
    case ETHER_TYPE_IPv6: {
      m->l3_len = sizeof(ipv6_hdr);
      ip_proto = ((ipv6_hdr *)(eth_type + 1))->proto;
      break;
    }
    default: {
      DLOG(WARNING) << "Packet is not IP - not supported";
      return false;
    }
  }

  // If it's not TCP or UDP packet - skip it
  switch (ip_proto) {
    case IPPROTO_TCP: {
      m->l4_len = sizeof(tcp_hdr);
      break;
    }
    case IPPROTO_UDP: {
      m->l4_len = sizeof(udp_hdr);
      break;
    }
    default: {
      DLOG(WARNING) << "Packet is not TCP/UDP - not supported";
      return false;
    }
  }

  return true;
}
/*
 * Additional functions end
 */


PacketManager::PacketManager(const std::string &config_name) : config_(config_name) {
}

bool PacketManager::Initialize() {
  if (!config_.Initialize()) {
    return false;
  }

  if (!port_manager_.Initialize()) {
    return false;
  }

  return true;
}

void PacketManager::RunProcessing() {
  static constexpr uint64_t timer_period = TIMER_MILLISECOND * 100;
  static const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
  uint64_t prev_tsc = 0, cur_tsc, diff_tsc, timer_tsc = 0;
  rte_eth_link link;

  auto lcore_id = rte_lcore_id();
  auto port = port_manager_.GetPort(lcore_id);
  if (!port) {
    LOG(WARNING) << "No task for lcore_id=" << (uint16_t)lcore_id;
    return;
  }
  auto port_id = port->GetPortId();
  auto tx_queue = port_manager_.GetPortTxQueue(lcore_id, port_id);
  assert(tx_queue != nullptr);
  PortQueue rx_queue;
  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " started";

  while(!terminated.load(std::memory_order_relaxed)) {
    cur_tsc = rte_rdtsc();
    diff_tsc = cur_tsc - prev_tsc;
    if (diff_tsc >= drain_tsc) {
      // Flush port tx-queue
      port->SendAllPackets(tx_queue);
      prev_tsc = cur_tsc;

      timer_tsc += diff_tsc;
      // Update port link status
      if (timer_tsc >= timer_period) {
        rte_eth_link_get_nowait(port->GetPortId(), &link);
        timer_tsc = 0;
      }
    }

    // TODO: print info about link status changing
    // TODO: print statistics

    // Read packets from port rx-queue
    if (link.link_status) {
      port->ReceivePackets(&rx_queue);
      ProcessPackets(&rx_queue);
    }
  }

  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " finished";
}

void PacketManager::ProcessPackets(PortQueue *queue) {
  for (uint16_t i = 0; i < queue->count_; ++i) {
    DLOG(INFO) << "Process single packet at lcore_id=" << (uint16_t)rte_lcore_id();
    auto m = queue->queue_[i];
    if (PreparePacket(m)) {
      DLOG(INFO) << "L2_len=" << m->l2_len;
      DLOG(INFO) << "L3_len=" << m->l3_len;
      DLOG(INFO) << "L4_len=" << m->l4_len;
    }
    rte_pktmbuf_free(m);
  }
  queue->count_ = 0;
}
