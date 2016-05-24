#include <rte_config.h>
#include <rte_cycles.h>
#include <cassert>
#include <glog/logging.h>
#include "packet_manager.h"
#include "packet_analyzer.h"

extern std::atomic<bool> terminated;

static constexpr auto kTIMER_MILLISECOND = 2000000ULL; /* around 1ms at 2 Ghz */
static constexpr auto kBURST_TX_DRAIN_US = 100; /* TX drain every ~100us */

PacketManager::PacketManager(const std::string &config_name, const uint16_t stats_interval)
    : config_(config_name),
      stats_interval_(stats_interval) {}

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
  static constexpr uint64_t timer_period = kTIMER_MILLISECOND * 100;
  static const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * kBURST_TX_DRAIN_US;
  static const uint64_t stats_interval_tsc = stats_interval_ * 1000 *kTIMER_MILLISECOND;
  uint64_t prev_tsc = rte_rdtsc(), cur_tsc, diff_tsc, timer_tsc = 0, timer_stats_tsc = 0;
  rte_eth_link link;

  auto lcore_id = rte_lcore_id();
  auto port = port_manager_.GetPortByCore(lcore_id);
  if (!port) {
    LOG(WARNING) << "No task for lcore_id=" << (uint16_t)lcore_id;
    return;
  }
  auto port_id = port->GetPortId();
  auto tx_queue = port_manager_.GetPortTxQueue(lcore_id, port_id);
  assert(tx_queue != nullptr);
  PortQueue rx_queue;
  auto lcore_stats_id = port_manager_.GetStatsLcoreId();
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

      // Print statistics
      if (stats_interval_tsc > 0) {
        if (lcore_id == lcore_stats_id) {
          timer_stats_tsc += diff_tsc;
          if (timer_stats_tsc > stats_interval_tsc) {
            PrintStats();
            timer_stats_tsc = 0;
          }
        }
      }
    }

    // TODO: print info about link status changing

    // Read packets from port rx-queue
    if (link.link_status) {
      port->ReceivePackets(&rx_queue);
      ProcessPackets(&rx_queue, port_id);
    }
  }

  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " finished";
}

void PacketManager::ProcessPackets(PortQueue *queue, const uint8_t port_id) {
  PacketAnalyzer &analyzer = PacketAnalyzer::Instance();
  auto lcore_id = rte_lcore_id();
  auto port = port_manager_.GetPortByIndex(port_id);

  for (uint16_t i = 0; i < queue->count_; ++i) {
    DLOG(INFO) << "Process single packet from port_id=" << (uint16_t)port_id;
    auto m = queue->queue_[i];
    if (packet_modifier::PreparePacket(m)) {
      DLOG(INFO) << "L2_len=" << m->l2_len;
      DLOG(INFO) << "L3_len=" << m->l3_len;
      DLOG(INFO) << "L4_len=" << m->l4_len;

      protocol_type protocol = analyzer.Analyze(m);
      port->UpdateProtocolStats(protocol, lcore_id);
      const uint16_t rule_key = port_id | (protocol << 8);
      Actions *actions;
      config_.GetActions(rule_key , actions);

      if (!actions) {
        continue;
      }

      for (auto it = actions->cbegin(); it != actions->cend(); ++it) {
        switch ((*it)->type) {
          case DROP: {
             break;
          }
          case PUSH_VLAN: {
            auto vlan_data = reinterpret_cast<PushVlanAction*>(*it);
            packet_modifier::ExecutePushVlan(m, vlan_data->vlan_tag);
            break;
          }
          case PUSH_MPLS: {
            auto mpls_data = reinterpret_cast<PushMplsAction*>(*it);
            packet_modifier::ExecutePushMpls(m, mpls_data->mpls_label);
            break;
          }
          case OUTPUT: {
            auto output_data = reinterpret_cast<OutputAction*>(*it);
            rte_mbuf *m_copy = port_manager_.CopyMbuf(m);
            this->ExecuteOutput(m_copy, output_data->port_id);
            break;
          }
        }
      }
    }
    rte_pktmbuf_free(m);
  }

  queue->count_ = 0;
}

void PacketManager::ExecuteOutput(rte_mbuf *m, const uint8_t port_id) {
  auto port = port_manager_.GetPortByIndex(port_id);
  auto tx_queue = port_manager_.GetPortTxQueue(rte_lcore_id(), port_id);
  port->SendOnePacket(m, tx_queue);
}

void PacketManager::PrintStats() const {
  std::ostringstream os;
  os << "\n=====Statistcics=====\n";

  auto nb_ports = rte_eth_dev_count();
  rte_eth_stats stats;
  for (uint8_t i = 0; i < nb_ports; ++i) {
    rte_eth_stats_get(i, &stats);
    os << "port_id=" << (uint16_t)i << "\n";
    os << " - Pkts in: " << stats.ipackets << "\n";
    os << " - Pkts out: " << stats.opackets << "\n";

    auto port = port_manager_.GetPortByIndex(i);
    os << "     HTTP: " << port->GetProtocolStats(HTTP) << "\n";
    os << "     SIP: " << port->GetProtocolStats(SIP) << "\n";
    os << "     RTP: " << port->GetProtocolStats(RTP) << "\n";
    os << "     RTSP: " << port->GetProtocolStats(RTSP) << "\n";
  }

  os << "====================\n";
  LOG(INFO) << os.str();
}
