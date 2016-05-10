#include <glog/logging.h>
#include <cassert>
#include <rte_cycles.h>
#include "port_manager.h"

/* Mempool settings */
static constexpr auto kMEMPOOL_NAME = "PKT_MEMPOOL";
static constexpr auto kNB_MBUF = 8192;
static constexpr auto kCACHE_SIZE = 32;
/* Queues settings */
static constexpr auto kNB_RX = 1;
static constexpr auto kNB_TX = 1;
static constexpr auto kNB_RXD = 128;
static constexpr auto kNB_TXD = 512;

PortManager::PortManager() : stats_lcore_id_(RTE_MAX_LCORE) {}

bool PortManager::Initialize() {
  auto nb_ports = rte_eth_dev_count();
  LOG(INFO) << "Number of ports: " << (uint16_t)nb_ports;

  /* Find core for each port.
   * Create mempool on each socket.
   * Initialize each port. */
  unsigned master_lcore = rte_get_master_lcore();
  unsigned lcore_id = 0;
  for (uint8_t i = 0; i < nb_ports; ++i) {
    while (!rte_lcore_is_enabled(lcore_id) || lcore_id == master_lcore) {
      lcore_id = rte_get_next_lcore(lcore_id, true, false);
      if (lcore_id >= RTE_MAX_LCORE) {
        LOG(ERROR) << "Can't find core for each port";
        return false;
      }
    }

    auto socket_id = rte_lcore_to_socket_id(lcore_id);
    if (mempools_.find(socket_id) == mempools_.end()) {
      rte_mempool *mp = rte_pktmbuf_pool_create(kMEMPOOL_NAME, kNB_MBUF, kCACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
      if (!mp) {
        LOG(ERROR) << "Can't create mempool for socket_id=" << (uint16_t)socket_id;
        return false;
      }
      mempools_.emplace(socket_id, mp);
      LOG(INFO) << "Mempool for socket_id=" << (uint16_t)socket_id << " allocated";
    }

    if (!InitializePort(i, socket_id)) {
      return false;
    }

    ports_.emplace(lcore_id, std::make_shared<PortEthernet>(i));
    LOG(INFO) << "Port mapping: port_id=" << (uint16_t)i << "->lcore_id=" << (uint16_t)lcore_id;

    ++lcore_id;
  }

  stats_lcore_id_ = --lcore_id; // last slave lcore
  LOG(INFO) << "Note: lcore_id=" << (uint16_t)lcore_id << " will be used for statistics (if required)";

  CheckPortsLinkStatus(nb_ports);

  return true;
}

std::shared_ptr<PortBase> PortManager::GetPort(const unsigned lcore_id) const {
  try {
    return ports_.at(lcore_id);
  }
  catch (const std::out_of_range &err) {
    return std::shared_ptr<PortBase>();
  }
}

PortQueue *PortManager::GetPortTxQueue(const unsigned lcore_id, const uint8_t port_id) {
  return &port_tx_table_[lcore_id][port_id];
}

unsigned PortManager::GetStatsLcoreId() const {
  return stats_lcore_id_;
}

bool PortManager::InitializePort(const uint8_t port_id, const unsigned socket_id) const {
  rte_eth_conf port_conf{};
  // Tune rx
  port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
  port_conf.rx_adv_conf.rss_conf.rss_key = NULL; // use defaut DPDK-key
  port_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
  // Tune tx
  port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
  auto ret = rte_eth_dev_configure(port_id, kNB_RX, kNB_TX, &port_conf);
  if (ret < 0) {
    LOG(ERROR) << "Can't configure port " << (uint16_t)port_id << ", error=" << ret;
    return false;
  }

  rte_mempool *mp = mempools_.at(socket_id);
  assert(mp != nullptr);
  ret = rte_eth_rx_queue_setup(port_id, 0, kNB_RXD, socket_id, nullptr, mp);
  if (ret < 0) {
    LOG(ERROR) << "Can't setup rx-queue for port " << (uint16_t)port_id << ", error=" << ret;
    return false;
  }
  ret = rte_eth_tx_queue_setup(port_id, 0, kNB_TXD, socket_id, nullptr);
  if (ret < 0) {
    LOG(ERROR) << "Can't setup tx-queue for port " << (uint16_t)port_id << ", error=" << ret;
    return false;
  }

  ret = rte_eth_dev_start(port_id);
  if (ret < 0) {
    LOG(ERROR) << "Can't start port " << (uint16_t)port_id << ", error=" << ret;
    return false;
  }

  rte_eth_promiscuous_enable(port_id);

  return true;
}

void PortManager::CheckPortsLinkStatus(const uint8_t nb_ports) const {
  constexpr uint8_t CHECK_INTERVAL = 100; // 100ms
  constexpr uint8_t MAX_CHECK_TIME = 90;  // 9s (90 * 100ms)
  bool show_flag = false;

  LOG(INFO) << "Checking ports link status...";
  for (uint8_t count = 0; count <= MAX_CHECK_TIME; ++count) {
    bool all_ports_up = true;
    for (uint8_t i = 0; i < nb_ports; ++i) {
      rte_eth_link link{};
      rte_eth_link_get_nowait(i, &link);
      // Show link status if flag set
      if (show_flag == 1) {
        if (link.link_status) {
          LOG(INFO) << "port_id=" << (uint16_t)i
                    << " [Link Up] - speed " << (uint16_t)link.link_speed << " Mbps "
                    << ((link.link_duplex == ETH_LINK_FULL_DUPLEX) ? ("full-duplex") : ("half-duplex"));
        }
        else {
          LOG(INFO) << "dpdk_port_id=" << (uint16_t)i << " [Link Down]";
        }
        continue;
      }
      if (link.link_status == 0) {
        all_ports_up = false;
        break;
      }
    }

    if (show_flag) {
      break;
    }

    if (!all_ports_up) {
      rte_delay_ms(CHECK_INTERVAL);
    }

    // Set the show_flag if all ports up or timeout
    if (all_ports_up || count == (MAX_CHECK_TIME - 1)) {
      show_flag = true;
    }
  }

  LOG(INFO) << "Done";
}
