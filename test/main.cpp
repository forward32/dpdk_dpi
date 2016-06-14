#include <glog/logging.h>
#include <gtest/gtest.h>
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_common.h>

int main(int argc, char *argv[])
{
  int eal_init_ret = rte_eal_init(argc, argv);
  if (eal_init_ret < 0) {
    rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
  }

  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
