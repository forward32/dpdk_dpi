#include <gtest/gtest.h>
#include "utils.h"
#include "common.h"
#include <rte_byteorder.h>
#include <glog/logging.h>

using namespace packet_modifier;

TEST(PushVlanAction, PushIntoPacketWithoutVlan) {
  uint8_t data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x40, 0x11, // (ttl, proto)
    0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,

    0x80, 0x08, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00, // timestamp
    0x00, 0x00, 0x00, // ssrc (without one byte)
  };
  auto m = InitPacket(data, sizeof(data));
  ASSERT_EQ(PreparePacket(m), true);
  // Vlan tag data
  const uint16_t tpid = 0x8100;
  const uint16_t vid = 100;
  const uint8_t cfi = 0;
  const uint8_t pcp = 2;
  // Construct vlan tag
  uint32_t vlan_tag = vid;
  vlan_tag |= (uint32_t)cfi<<12;
  vlan_tag |= (uint32_t)pcp<<13;
  vlan_tag |= (uint32_t)tpid<<16;
  // Push vlan tag
  ExecutePushVlan(m, rte_cpu_to_be_32(vlan_tag));
  // Check vlan tag inside packet
  uint32_t pkt_vlan_tag = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12);
  pkt_vlan_tag = rte_cpu_to_be_32(pkt_vlan_tag);
  ASSERT_EQ((pkt_vlan_tag & 0xffff0000)>>16, tpid);
  ASSERT_EQ((pkt_vlan_tag & 0x0000e000)>>13, pcp);
  ASSERT_EQ((pkt_vlan_tag & 0x00001000)>>12, cfi);
  ASSERT_EQ((pkt_vlan_tag & 0x00000fff)>>0, vid);
  // Check packet ethertype
  uint16_t pkt_eth_type = *rte_pktmbuf_mtod_offset(m, uint16_t *, 12+4);
  ASSERT_EQ(rte_cpu_to_be_16(pkt_eth_type), 0x0800);
}

TEST(PushVlanAction, PushIntoPacketWithVlan) {
  uint8_t data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0xa8,
    0x20, 0x03, // pcp=1, cfi=0, vid=3
    0x08, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x40, 0x11, // (ttl, proto)
    0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,

    0x80, 0x08, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00, // timestamp
    0x00, 0x00, 0x00, // ssrc (without one byte)
  };
  auto m = InitPacket(data, sizeof(data));
  ASSERT_EQ(PreparePacket(m), true);
  // Vlan tag data
  const uint16_t tpid = 33024; // 8100
  const uint16_t vid = 100;
  const uint8_t cfi = 1;
  const uint8_t pcp = 3;
  // Construct vlan tag
  uint32_t vlan_tag = vid;
  vlan_tag |= (uint32_t)cfi<<12;
  vlan_tag |= (uint32_t)pcp<<13;
  vlan_tag |= (uint32_t)tpid<<16;
  // Push vlan tag
  ExecutePushVlan(m, rte_cpu_to_be_32(vlan_tag));
  // Check first vlan tag inside packet
  uint32_t pkt_vlan_tag = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12);
  pkt_vlan_tag = rte_cpu_to_be_32(pkt_vlan_tag);
  ASSERT_EQ((pkt_vlan_tag & 0xffff0000)>>16, tpid);
  ASSERT_EQ((pkt_vlan_tag & 0x0000e000)>>13, pcp);
  ASSERT_EQ((pkt_vlan_tag & 0x00001000)>>12, cfi);
  ASSERT_EQ((pkt_vlan_tag & 0x00000fff)>>0, vid);
  // Check second vlan tag inside packet
  pkt_vlan_tag = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12+4);
  pkt_vlan_tag = rte_cpu_to_be_32(pkt_vlan_tag);
  ASSERT_EQ((pkt_vlan_tag & 0xffff0000)>>16, 0x88a8);
  ASSERT_EQ((pkt_vlan_tag & 0x0000e000)>>13, 1);
  ASSERT_EQ((pkt_vlan_tag & 0x00001000)>>12, 0);
  ASSERT_EQ((pkt_vlan_tag & 0x00000fff)>>0, 3);
  // Check packet ethertype
  uint16_t pkt_eth_type = *rte_pktmbuf_mtod_offset(m, uint16_t *, 12+4+4);
  ASSERT_EQ(rte_cpu_to_be_16(pkt_eth_type), 0x0800);
}

TEST(PushMplsAction, PushIntoPacketWithoutVlan) {
  uint8_t data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x40, 0x11, // (ttl, proto)
    0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,

    0x80, 0x08, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00, // timestamp
    0x00, 0x00, 0x00, // ssrc (without one byte)
  };
  auto m = InitPacket(data, sizeof(data));
  ASSERT_EQ(PreparePacket(m), true);
  // Mpls label data
  const uint32_t label = 65793;
  const uint8_t exp = 3;
  const uint8_t stack = 1;
  const uint8_t ttl = 64;
  // Construct mpls label
  uint32_t mpls_label = ttl;
  mpls_label |= (uint32_t)stack<<8;
  mpls_label |= (uint32_t)exp<<9;
  mpls_label |= label<<12;
  // Push mpls label
  ExecutePushMpls(m, rte_cpu_to_be_32(mpls_label));
  // Check mpls_label inside packet
  uint32_t pkt_mpls_label = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12+2);
  pkt_mpls_label = rte_cpu_to_be_32(pkt_mpls_label);
  ASSERT_EQ((pkt_mpls_label & 0xfffff000)>>12, label);
  ASSERT_EQ((pkt_mpls_label & 0x00000100)>>8, stack);
  ASSERT_EQ((pkt_mpls_label & 0x00000e00)>>9, exp);
  ASSERT_EQ((pkt_mpls_label & 0x000000ff)>>0, ttl);
  // Check packet ethertype
  uint16_t pkt_eth_type = *rte_pktmbuf_mtod_offset(m, uint16_t *, 12);
  ASSERT_EQ(rte_cpu_to_be_16(pkt_eth_type), 0x8847);
}

TEST(PushMplsAction, PushIntoPacketWithVlan) {
  uint8_t data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0xa8,
    0x20, 0x03, // pcp=1, cfi=0, vid=3
    0x08, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x40, 0x11, // (ttl, proto)
    0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,

    0x80, 0x08, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00, // timestamp
    0x00, 0x00, 0x00, // ssrc (without one byte)
  };
  auto m = InitPacket(data, sizeof(data));
  ASSERT_EQ(PreparePacket(m), true);
  // Mpls label data
  const uint32_t label = 65793;
  const uint8_t exp = 3;
  const uint8_t stack = 1;
  const uint8_t ttl = 64;
  // Construct mpls label
  uint32_t mpls_label = ttl;
  mpls_label |= (uint32_t)stack<<8;
  mpls_label |= (uint32_t)exp<<9;
  mpls_label |= label<<12;
  // Push mpls label
  ExecutePushMpls(m, rte_cpu_to_be_32(mpls_label));
  // Check mpls_label inside packet
  uint32_t pkt_mpls_label = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12+4+2);
  pkt_mpls_label = rte_cpu_to_be_32(pkt_mpls_label);
  ASSERT_EQ((pkt_mpls_label & 0xfffff000)>>12, label);
  ASSERT_EQ((pkt_mpls_label & 0x00000100)>>8, stack);
  ASSERT_EQ((pkt_mpls_label & 0x00000e00)>>9, exp);
  ASSERT_EQ((pkt_mpls_label & 0x000000ff)>>0, ttl);
  // Check vlan tag
  uint32_t pkt_vlan_tag = *rte_pktmbuf_mtod_offset(m, uint32_t *, 12);
  pkt_vlan_tag = rte_cpu_to_be_32(pkt_vlan_tag);
  ASSERT_EQ((pkt_vlan_tag & 0xffff0000)>>16, 0x88a8);
  ASSERT_EQ((pkt_vlan_tag & 0x0000e000)>>13, 1);
  ASSERT_EQ((pkt_vlan_tag & 0x00001000)>>12, 0);
  ASSERT_EQ((pkt_vlan_tag & 0x00000fff)>>0, 3);
  // Check packet ethertype
  uint16_t pkt_eth_type = *rte_pktmbuf_mtod_offset(m, uint16_t *, 12+4);
  ASSERT_EQ(rte_cpu_to_be_16(pkt_eth_type), 0x8847);
}
