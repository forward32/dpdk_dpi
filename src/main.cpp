#include <glog/logging.h>
#include <atomic>
#include <csignal>
#include "packet_manager.h"

std::atomic<bool> terminated;

static void sigint_handler(int sig_num) {
  (void)sig_num;

  terminated.store(true, std::memory_order_relaxed);
}

static int launch_lcore(void *arg) {
  ((PacketManager*)arg)->RunProcessing();

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  terminated.store(false, std::memory_order_relaxed);
  signal(SIGINT, sigint_handler);

  PacketManager packet_manager;
  packet_manager.Initialize(&argc, &argv);

  rte_eal_mp_remote_launch(launch_lcore, (void *)(&packet_manager), SKIP_MASTER);

  unsigned lcore_id;
  RTE_LCORE_FOREACH_SLAVE(lcore_id) {
    if (rte_eal_wait_lcore(lcore_id) < 0) {
      LOG(ERROR) << "Wait failed for lcore_id=" << (uint16_t)lcore_id;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
