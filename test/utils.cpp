#include "utils.h"
#include <rte_memcpy.h>
#include <rte_errno.h>

static constexpr auto kTEST_MEMPOOL_NAME = "TEST_MEMPOOL_NAME";
static constexpr auto kNB_MBUF = 4096;
static constexpr auto kMBUF_SIZE = 256;
static constexpr auto kCACHE_SIZE = 64;

static rte_mempool *GetMempoolForTest() {
  static rte_mempool *mp = nullptr;

  if (mp) {
    return mp;
  }

  mp = rte_mempool_create(
      kTEST_MEMPOOL_NAME,
      kNB_MBUF,
      kMBUF_SIZE,
      kCACHE_SIZE,
      sizeof(rte_pktmbuf_pool_private),
      rte_pktmbuf_pool_init, nullptr,
      rte_pktmbuf_init, nullptr,
      rte_socket_id(), 0);
  if (!mp) {
    rte_exit(EXIT_FAILURE, "Can't init mempool for tests, error=%d\n", rte_errno);
  }

  return mp;
}

rte_mbuf *InitPacket(const uint8_t data[], const uint16_t data_len) {
  rte_mbuf *m = rte_pktmbuf_alloc(GetMempoolForTest());
  rte_memcpy(rte_ctrlmbuf_data(m), data, data_len);
  m->pkt_len = data_len;
  m->data_len = data_len;

  return m;
}
