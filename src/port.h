#ifndef PORT_
#define PORT_

#include <atomic>
#include "common.h"

static constexpr auto kMAX_PKTS_IN_QUEUE = 32;
static constexpr auto kMAX_LCORES = 16;

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
  void UpdateProtocolStats(const protocol_type, const unsigned);
  uint64_t GetProtocolStats(const protocol_type) const;

 private:
  struct ProtocolStats {
    std::atomic<uint64_t> http[kMAX_LCORES];
    std::atomic<uint64_t> sip[kMAX_LCORES];
    std::atomic<uint64_t> rtp[kMAX_LCORES];
    std::atomic<uint64_t> rtsp[kMAX_LCORES];
  };

  uint8_t port_id_;
  ProtocolStats protocol_stats_;
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
