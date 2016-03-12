#ifndef PORT_MANAGER_
#define PORT_MANAGER_

class PortManager {
 public:
  PortManager() = default;
  ~PortManager() = default;

  PortManager(const PortManager &) = delete;
  PortManager &operator=(const PortManager &) = delete;
  PortManager(PortManager &&) = delete;
  PortManager &operator=(PortManager &&) = delete;

  void Initialize();
};

#endif // PORT_MANAGER_
