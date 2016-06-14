#ifndef CONFIG_
#define CONFIG_

#include <vector>
#include <memory>
#include "action.h"

using Actions = std::vector<Action *>;

class Config {
 public:
  explicit Config(const std::string &);
  ~Config();

  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;
  Config(Config &&) = delete;
  Config &operator=(Config &&) = delete;

  bool Initialize();
  void GetActions(const uint16_t, Actions *&);

 protected:
  bool ParsePortAndProtocol(uint16_t &, std::string &);
  bool ParseActions(Actions &, std::string &);
  bool ParseVlanData(uint16_t &, uint8_t &, uint8_t &, uint16_t &, std::string &);
  bool ParseMplsData(uint32_t &, uint8_t &, uint8_t &, uint8_t &, std::string &);

 private:
  std::string config_name_;
  std::unordered_map<uint16_t, Actions> rules_;
};

#endif // CONFIG_
