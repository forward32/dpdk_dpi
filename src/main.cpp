#include <glog/logging.h>
#include <csignal>
#include "packet_manager.h"
#include "cmd_args.h"

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

  auto ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
  }

  rte_config *cfg = rte_eal_get_configuration();
  LOG(INFO) << "Number of logical cores: " << cfg->lcore_count;

  argc -= ret;
  argv += ret;
  CmdArgs cmd_args = ParseArgs(argc, argv); // may throw

  PacketManager packet_manager(cmd_args.config_file, cmd_args.stats_interval);
  if (!packet_manager.Initialize()) {
    rte_exit(EXIT_FAILURE, "Can't initialize packet manager\n");
  }

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
