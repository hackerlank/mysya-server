#ifndef TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_STATUS_H
#define TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_STATUS_H

namespace google {
namespace protobuf {

class Message;

}  // namespace protobuf
}  // namespace google

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {
namespace ai {

class Auto;

class AutoStatus {
 public:
  typedef ::google::protobuf::Message ProtoMessage;

  enum type {
    SEARCH = 1;
    MOVE = 2;
    ATTACK = 3;
  };

  AutoStatus(Auto *host);
  virtual ~AutoStatus();

  virtual void Start() = 0;
  virtual void Stop() = 0;

  virtual void OnEvent(int type, ProtoMessage *data) = 0;

 protected:
  Auto *host_;
};

}  // namespace ai
}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial

#endif  // TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_STATUS_H
