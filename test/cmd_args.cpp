#include <gtest/gtest.h>
#include <stdexcept>
#include "cmd_args.h"

TEST(CmdArgs, InvalidArgs) {
  char arg0[] = "./dpdk_dpi";
  char arg1[] = "--stats-interval";
  char arg2[] = "a";
  char arg3[] = "--config";
  char arg4[] = "test_config.txt";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int argc = 5;

  EXPECT_THROW(ParseArgs(argc, argv), std::invalid_argument);
}

TEST(CmdArgs, ValidArgs) {
  char arg0[] = "./dpdk_dpi";
  char arg1[] = "--stats-interval";
  char arg2[] = "5";
  char arg3[] = "--config";
  char arg4[] = "test_config.txt";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int argc = 5;

  CmdArgs cmd_args = ParseArgs(argc, argv);
  ASSERT_EQ(cmd_args.stats_interval, 5);
  ASSERT_EQ(strcmp(cmd_args.config_file, "test_config.txt"), 0);
}
