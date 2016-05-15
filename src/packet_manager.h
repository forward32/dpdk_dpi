#ifndef PACKET_MANAGER_
#define PACKET_MANAGER_

#include "port_manager.h"
#include "config.h"

class PacketManager {
 public:
  PacketManager(const std::string &, const uint16_t);
  ~PacketManager() = default;

  PacketManager(const PacketManager &) = delete;
  PacketManager &operator=(const PacketManager &) = delete;
  PacketManager(PacketManager &&) = delete;
  PacketManager &operator=(PacketManager &&) = delete;

  bool Initialize();
  void RunProcessing();

 protected:
  void ProcessPackets(PortQueue *, const uint8_t);
  void PrintStats() const;

 private:
  Config config_;
  PortManager port_manager_;
  uint16_t stats_interval_;
};

#endif // PACKET_MANAGER_
