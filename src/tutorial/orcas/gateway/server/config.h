#ifndef TUTORIAL_ORCAS_GATEWAY_SERVER_CONFIG_H
#define TUTORIAL_ORCAS_GATEWAY_SERVER_CONFIG_H

#include <string>
#include <map>

#include <mysya/util/class_util.h>

namespace tutorial {
namespace orcas {
namespace gateway {
namespace server {

class Config {
 public:
  struct CombatServerConf {
    int32_t server_id_;
    std::string host_;
    int32_t port_;
  };

  typedef std::map<int, CombatServerConf> CombatServerMap;

  bool Load(const std::string &file);

  int server_id_;
  std::string listen_host_;
  int listen_port_;

  CombatServerMap combat_servers_;

 private:
  MYSYA_SINGLETON(Config);
};

}  // namespace server
}  // namespace gateway
}  // namespace orcas
}  // namespace tutorial

#endif  // TUTORIAL_ORCAS_GATEWAY_SERVER_CONFIG_H
