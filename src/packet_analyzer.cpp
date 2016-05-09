#include "packet_analyzer.h"

// List of search methods
extern protocol_type SearchSip(rte_mbuf *);
extern protocol_type SearchRtp(rte_mbuf *);
extern protocol_type SearchRtsp(rte_mbuf *);
// List end

PacketAnalyzer::PacketAnalyzer() {
  methods_ = {SearchSip, SearchRtp, SearchRtsp};
}

PacketAnalyzer &PacketAnalyzer::Instance() {
  static PacketAnalyzer instance;

  return instance;
}

protocol_type PacketAnalyzer::Analyze(rte_mbuf *m) const {
  protocol_type ret  = UNKNOWN;

  /*
   * Now it's very simple analyzer.
   * It returns first appropriate protocol.
   */
  for (auto it = methods_.cbegin(); ret == UNKNOWN && it != methods_.cend(); ++it) {
    ret = (*it)(m);
  }

  return ret;
}
