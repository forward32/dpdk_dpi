#ifndef PORT_
#define PORT_

#include <cstdint>

class PortBase {
 public:
  explicit PortBase(const uint8_t);
  ~PortBase() = default;

  PortBase(const PortBase &) = delete;
  PortBase &operator=(const PortBase &) = delete;
  PortBase(PortBase &&) = delete;
  PortBase &operator=(PortBase &&) = delete;

 private:
  uint8_t port_id_;
};

#endif // PORT_
