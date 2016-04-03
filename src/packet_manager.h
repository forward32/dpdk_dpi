#ifndef PACKET_MANAGER_
#define PACKET_MANAGER_

#include "port_manager.h"
#include "config.h"

class PacketManager {
 public:
  explicit PacketManager(const std::string &);
  ~PacketManager() = default;

  PacketManager(const PacketManager &) = delete;
  PacketManager &operator=(const PacketManager &) = delete;
  PacketManager(PacketManager &&) = delete;
  PacketManager &operator=(PacketManager &&) = delete;

  void Initialize(int *, char **[]);
  void RunProcessing();

 protected:
  void ProcessPackets(PortQueue *);

 private:
  Config config_;
  PortManager port_manager_;
};

#endif // PACKET_MANAGER_
