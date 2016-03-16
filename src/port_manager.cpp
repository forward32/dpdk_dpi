#include <glog/logging.h>
#include <cassert>
#include <rte_cycles.h>
#include "port_manager.h"

/* Mempool settings */
#define kMEMPOOL_NAME "PKT_MEMPOOL"
#define kNB_MBUF 8192
#define kCACHE_SIZE 32
/* Queues settings */
#define kNB_RX 1
#define kNB_TX 1
#define kNB_RXD 128
#define kNB_TXD 512

void PortManager::Initialize() {
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
        rte_exit(EXIT_FAILURE, "Can't find core for each port\n");
      }
    }

    auto socket_id = rte_lcore_to_socket_id(lcore_id);
    if (mempools_.find(socket_id) == mempools_.end()) {
      rte_mempool *mp = rte_pktmbuf_pool_create(kMEMPOOL_NAME, kNB_MBUF, kCACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
      if (!mp) {
        rte_exit(EXIT_FAILURE, "Can't create mempool for socket_id=%d\n", (uint16_t)socket_id);
      }
      mempools_.emplace(socket_id, mp);
      LOG(INFO) << "Mempool for socket_id=" << (uint16_t)socket_id << " allocated";
    }

    InitializePort(i, socket_id);

    ports_.emplace(lcore_id, std::make_shared<PortBase>(i));
    LOG(INFO) << "Port mapping: port_id=" << (uint16_t)i << "->lcore_id=" << (uint16_t)lcore_id;

    ++lcore_id;
  }

  CheckPortsLinkStatus(nb_ports);
}

std::shared_ptr<PortBase> PortManager::GetPort(const unsigned lcore_id) const {
  return ports_.at(lcore_id);
}

void PortManager::InitializePort(const uint8_t port_id, const unsigned socket_id) const {
  rte_eth_conf port_conf{};
  // Tune rx
  port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
  port_conf.rx_adv_conf.rss_conf.rss_key = NULL; // use defaut DPDK-key
  port_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
  // Tune tx
  port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
  auto ret = rte_eth_dev_configure(port_id, kNB_RX, kNB_TX, &port_conf);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Can't configure port %d, error=%d\n", (uint16_t)port_id, ret);
  }

  rte_mempool *mp = mempools_.at(socket_id);
  assert(mp != nullptr);
  ret = rte_eth_rx_queue_setup(port_id, 0, kNB_RXD, socket_id, nullptr, mp);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Can't setup rx-queue for port %d, error=%d\n", (uint16_t)port_id, ret);
  }
  ret = rte_eth_tx_queue_setup(port_id, 0, kNB_TXD, socket_id, nullptr);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Can't setup tx-queue for port %d, error=%d\n", (uint16_t)port_id, ret);
  }

  ret = rte_eth_dev_start(port_id);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Can't start port %d, error=%d\n", (uint16_t)port_id, ret);
  }

  rte_eth_promiscuous_enable(port_id);
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
