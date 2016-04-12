#include <glog/logging.h>
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
