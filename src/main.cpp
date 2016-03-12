#include <glog/logging.h>
#include "packet_manager.h"

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  PacketManager packet_manager;
  packet_manager.Initialize(&argc, &argv);

  return 0;
}
