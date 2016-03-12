#ifndef PACKET_MANAGER_
#define PACKET_MANAGER_

#include "port_manager.h"

class PacketManager {
 public:
  PacketManager() = default;
  ~PacketManager() = default;

  PacketManager(const PacketManager &) = delete;
  PacketManager &operator=(const PacketManager &) = delete;
  PacketManager(PacketManager &&) = delete;
  PacketManager &operator=(PacketManager &&) = delete;

  void Initialize(int *, char **[]);

 private:
  PortManager port_manager_;
};

#endif // PACKET_MANAGER_
