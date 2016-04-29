#ifndef TUTORIAL_ORCAS_COMBAT_SERVER_COMBAT_MESSAGE_HANDLER_H
#define TUTORIAL_ORCAS_COMBAT_SERVER_COMBAT_MESSAGE_HANDLER_H

#include <mysya/util/class_util.h>

namespace google {
namespace protobuf {

class Message;

}  // namespace protobuf
}  // namespace google

namespace tutorial {
namespace orcas {
namespace combat {

class TransportChannel;

namespace server {

class AppServer;

class CombatMessageHandler {
 public:
  explicit CombatMessageHandler(AppServer *app_server);
  ~CombatMessageHandler();

  void SetMessageHandlers();
  void ResetMessageHandlers();

 private:
  void OnMessageCombatDeployRequest(::tutorial::orcas::combat::TransportChannel *transport_channel,
      const ::google::protobuf::Message *message_pb);
  void OnMessageCombatArgentRequest(::tutorial::orcas::combat::TransportChannel *transport_channel,
      const ::google::protobuf::Message *message_pb);
  void OnMessageCombatConnectArgentRequest(::tutorial::orcas::combat::TransportChannel *transport_channel,
      const ::google::protobuf::Message *message_pb);

  AppServer *app_server_;
};

}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorialk

#endif  // TUTORIAL_ORCAS_COMBAT_SERVER_COMBAT_MESSAGE_HANDLER_H
