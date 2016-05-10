#include <rte_config.h>
#include <rte_cycles.h>
#include <atomic>
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
  auto port = port_manager_.GetPort(lcore_id);
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
      ProcessPackets(&rx_queue);
    }
  }

  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " finished";
}

void PacketManager::ProcessPackets(PortQueue *queue) {
  PacketAnalyzer &analyzer = PacketAnalyzer::Instance();

  for (uint16_t i = 0; i < queue->count_; ++i) {
    DLOG(INFO) << "Process single packet at lcore_id=" << (uint16_t)rte_lcore_id();
    auto m = queue->queue_[i];
    if (PreparePacket(m)) {
      DLOG(INFO) << "L2_len=" << m->l2_len;
      DLOG(INFO) << "L3_len=" << m->l3_len;
      DLOG(INFO) << "L4_len=" << m->l4_len;

      auto ret = analyzer.Analyze(m);
      // TODO: execute actions for this protocol type
    }
    rte_pktmbuf_free(m);
  }

  queue->count_ = 0;
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
  }

  os << "====================\n";
  LOG(INFO) << os.str();
}
