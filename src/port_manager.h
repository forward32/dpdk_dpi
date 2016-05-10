#ifndef PORT_MANAGER_
#define PORT_MANAGER_

#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <memory>
#include <unordered_map>
#include "port.h"

class PortManager {
 public:
  PortManager();
  ~PortManager() = default;

  PortManager(const PortManager &) = delete;
  PortManager &operator=(const PortManager &) = delete;
  PortManager(PortManager &&) = delete;
  PortManager &operator=(PortManager &&) = delete;

  bool Initialize();
  std::shared_ptr<PortBase> GetPort(const unsigned) const;
  PortQueue *GetPortTxQueue(const unsigned, const uint8_t);
  unsigned GetStatsLcoreId() const;

 protected:
  bool InitializePort(const uint8_t, const unsigned) const;
  void CheckPortsLinkStatus(const uint8_t) const;

 private:
  std::unordered_map<unsigned, rte_mempool *> mempools_;
  std::unordered_map<unsigned, std::shared_ptr<PortBase>> ports_;
  PortQueue port_tx_table_[RTE_MAX_LCORE][RTE_MAX_ETHPORTS];
  unsigned stats_lcore_id_;
};

#endif // PORT_MANAGER_
