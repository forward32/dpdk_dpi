#ifndef PORT_
#define PORT_

#include <cstdint>
#include <rte_config.h>
#include <rte_mbuf.h>

class PortBase {
 public:
  explicit PortBase(const uint8_t);
  ~PortBase() = default;

  PortBase(const PortBase &) = delete;
  PortBase &operator=(const PortBase &) = delete;
  PortBase(PortBase &&) = delete;
  PortBase &operator=(PortBase &&) = delete;

  virtual void SendOnePacket(rte_mbuf *) = 0;
  virtual void SendAllPackets() = 0;
  virtual uint16_t ReceivePackets(rte_mbuf **) = 0;

 private:
  uint8_t port_id_;
};


class PortEthernet : public PortBase {
 public:
  explicit PortEthernet(const uint8_t);

  ~PortEthernet() = default;

  PortEthernet(const PortEthernet &) = delete;
  PortEthernet &operator=(const PortEthernet &) = delete;
  PortEthernet (PortEthernet &&) = delete;
  PortEthernet &operator=(PortEthernet &&) = delete;

  virtual void SendOnePacket(rte_mbuf *) override;
  virtual void SendAllPackets() override;
  virtual uint16_t ReceivePackets(rte_mbuf **) override;
};

#endif // PORT_
