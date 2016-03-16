#include <atomic>
#include <cassert>
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_common.h>
#include <glog/logging.h>
#include "packet_manager.h"

extern std::atomic<bool> terminated;

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
  auto lcore_id = rte_lcore_id();
  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " started";

  auto port = port_manager_.GetPort(lcore_id);
  assert(port != nullptr);
  while(!terminated.load(std::memory_order_relaxed)) {
    // TODO: implement main logic
  }

  LOG(INFO) << "Processing at lcore_id=" << (uint16_t)lcore_id << " finished";
}
