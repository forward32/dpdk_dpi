#include "common.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <glog/logging.h>

#define ETHER_TYPE_VLAN_8021AD 0x88a8

bool ParseInt(const std::string &str, unsigned long &ret) {
  try {
    size_t end_pos;
    ret = std::stoul(str.c_str(), &end_pos, 10);
    if (end_pos != str.length()) {
      return false;
    }
  }
  catch (const std::logic_error &err) {
    return false;
  }

  return true;
}

namespace packet_modifier{
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
      ipv4_hdr *ipv4 = (ipv4_hdr *)(eth_type + 1);
      m->l3_len = 4*(ipv4->version_ihl & 0x0F);
      ip_proto = ipv4->next_proto_id;
      break;
    }
    case ETHER_TYPE_IPv6: {
      m->l3_len = sizeof(ipv6_hdr); // always 40 bytes
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
      tcp_hdr *tcp = rte_pktmbuf_mtod_offset(m, tcp_hdr *, m->l2_len + m->l3_len);
      m->l4_len = 4*((tcp->data_off & 0xF0) >> 4);
      break;
    }
    case IPPROTO_UDP: {
      m->l4_len = sizeof(udp_hdr); // always 8 bytes
      break;
    }
    default: {
      DLOG(WARNING) << "Packet is not TCP/UDP - not supported";
      return false;
    }
  }

  return true;
}

void ExecutePushVlan(rte_mbuf *m, const uint32_t vlan_tag) {
  if (rte_vlan_insert(&m) != 0) {
    LOG(WARNING) << "Can't insert vlan";
    return;
  }

  constexpr uint8_t mac_addr_len = 6;
  char *dst_data = rte_pktmbuf_mtod_offset(m, char *, mac_addr_len*2);
  rte_memcpy(dst_data, &vlan_tag, sizeof(vlan_tag));
}

void ExecutePushMpls(rte_mbuf *m, const uint32_t mpls_label) {
  char *src_data = rte_pktmbuf_mtod(m, char *);
  char *dst_data = (char *)rte_pktmbuf_prepend(m, sizeof(mpls_label));
  if (!dst_data) {
    LOG(WARNING) << "Can't insert mpls";
    return;
  }

  memmove(dst_data, src_data, m->l2_len);
  rte_memcpy(dst_data+m->l2_len, &mpls_label, sizeof(mpls_label));
  const uint16_t mpls_ethertype = rte_cpu_to_be_16(0x8847);
  rte_memcpy(dst_data+m->l2_len-2, &mpls_ethertype, 2);
}
}
