#include <atomic>
#include <cassert>
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <glog/logging.h>
#include "packet_manager.h"

extern std::atomic<bool> terminated;

#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

void PacketManager::Initialize(int *argc, char **argv[]) {
  auto ret = rte_eal_init(*argc, *argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
  }
  *argc -= ret;
  *argv += ret;

  rte_config *cfg = rte_eal_get_configuration();
  LOG(INFO) << "Number of logical cores: " << cfg->lcore_count;

  port_manager_.Initialize();
}

void PacketManager::RunProcessing() {
  static constexpr uint64_t timer_period = TIMER_MILLISECOND * 100;
  static const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
  uint64_t prev_tsc = 0, cur_tsc, diff_tsc, timer_tsc = 0;
  rte_eth_link link;

  auto lcore_id = rte_lcore_id();
  auto port = port_manager_.GetPort(lcore_id);
  assert(port != nullptr);
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
    rte_pktmbuf_free(queue->queue_[i]);
  }
  queue->count_ = 0;
}
