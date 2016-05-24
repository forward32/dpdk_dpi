#ifndef PORT_
#define PORT_

#include <cstdint>
#include <rte_config.h>
#include <rte_mbuf.h>

static constexpr auto kMAX_PKTS_IN_QUEUE = 32;

struct PortQueue {
  PortQueue() : count_(0) {}

  uint16_t count_;
  rte_mbuf *queue_[kMAX_PKTS_IN_QUEUE];
};


class PortBase {
 public:
  explicit PortBase(const uint8_t);
  virtual ~PortBase() = default;

  PortBase(const PortBase &) = delete;
  PortBase &operator=(const PortBase &) = delete;
  PortBase(PortBase &&) = delete;
  PortBase &operator=(PortBase &&) = delete;

  virtual void SendOnePacket(rte_mbuf *, PortQueue *) = 0;
  virtual void SendAllPackets(PortQueue *) = 0;
  virtual void ReceivePackets(PortQueue *) = 0;

  uint8_t GetPortId() const;

 private:
  uint8_t port_id_;
};


class PortEthernet : public PortBase {
 public:
  explicit PortEthernet(const uint8_t);
  virtual ~PortEthernet() = default;

  PortEthernet(const PortEthernet &) = delete;
  PortEthernet &operator=(const PortEthernet &) = delete;
  PortEthernet (PortEthernet &&) = delete;
  PortEthernet &operator=(PortEthernet &&) = delete;

  virtual void SendOnePacket(rte_mbuf *, PortQueue *) override;
  virtual void SendAllPackets(PortQueue *) override;
  virtual void ReceivePackets(PortQueue *) override;
};

#endif // PORT_
