#include "common.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <glog/logging.h>

#define ETHER_TYPE_VLAN_8021AD 0x88a8

bool PreparePacket(rte_mbuf *m) {
  char *pkt_data = rte_ctrlmbuf_data(m);
  uint16_t *eth_type = (uint16_t *)(pkt_data + 2*ETHER_ADDR_LEN);
  m->l2_len = 2*ETHER_ADDR_LEN;

  // Skip VLAN tags
  while (*eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN) ||
         *eth_type == rte_cpu_to_be_16(ETHER_TYPE_VLAN_8021AD)) {
    eth_type += 2;
    m->l2_len += 4;
  }
  m->l2_len += 2;

  // If it's not IP packet - skip it
  uint8_t ip_proto = 0;
  switch (rte_cpu_to_be_16(*eth_type)) {
    case ETHER_TYPE_IPv4: {
      m->l3_len = sizeof(ipv4_hdr);
      ip_proto = ((ipv4_hdr *)(eth_type + 1))->next_proto_id;
      break;
    }
    case ETHER_TYPE_IPv6: {
      m->l3_len = sizeof(ipv6_hdr);
      ip_proto = ((ipv6_hdr *)(eth_type + 1))->proto;
      break;
    }
    default: {
      DLOG(WARNING) << "Packet is not IP - not supported";
      return false;
    }
  }

  // If it's not TCP or UDP packet - skip it
  switch (ip_proto) {
    case IPPROTO_TCP: {
      m->l4_len = sizeof(tcp_hdr);
      break;
    }
    case IPPROTO_UDP: {
      m->l4_len = sizeof(udp_hdr);
      break;
    }
    default: {
      DLOG(WARNING) << "Packet is not TCP/UDP - not supported";
      return false;
    }
  }

  return true;
}
