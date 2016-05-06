#ifndef PACKET_ANALYZER_
#define PACKET_ANALYZER_

#include <functional>
#include <vector>
#include "common.h"

class PacketAnalyzer {
 using SearchMethod = std::function<protocol_type(rte_mbuf *)>;

 public:
  PacketAnalyzer();
  ~PacketAnalyzer() = default;

  PacketAnalyzer(const PacketAnalyzer &) = delete;
  PacketAnalyzer &operator=(const PacketAnalyzer &) = delete;
  PacketAnalyzer(PacketAnalyzer &&) = delete;
  PacketAnalyzer &operator=(PacketAnalyzer &&) = delete;

  static PacketAnalyzer &Instance();
  protocol_type Analyze(rte_mbuf *) const;

 private:
  std::vector<SearchMethod> methods_;
};

#endif // PACKET_ANALYZER_
