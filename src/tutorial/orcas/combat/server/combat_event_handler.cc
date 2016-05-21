#include "tutorial/orcas/combat/server/combat_event_handler.h"

#include <google/protobuf/message.h>
#include <mysya/ioevent/logger.h>

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {

CombatEventHandler::CombatEventHandler()
  : event_token_move_action_(0) {}
CombatEventHandler::~CombatEventHandler() {}

#define SCENE_APP() \
    SceneApp::GetInstance()

bool CombatEventHandler::Initialize() {
  this->event_token_build_action_ =
    SCENE_APP()->GetEventDispatcher()->Attach(event::EVENT_COMBAT_BUILD_ACTION,
        std::bind(&CombatEventHandler::EventCombatBuildAction, this, std::placeholders::_1));
  this->event_token_move_action_ =
    SCENE_APP()->GetEventDispatcher()->Attach(event::EVENT_COMBAT_MOVE_ACTION,
        std::bind(&CombatEventHandler::EventCombatMoveAction, this, std::placeholders::_1));

  return true;
}

void CombatEventHandler::Finalize() {
  SCENE_APP()->GetEventDispatcher()->Detach(this->event_token_move_action_);
  SCENE_APP()->GetEventDispatcher()->Detach(this->event_token_build_action_);
}

void CombatEventHandler::EventCombatBuildAction(const ProtoMessage *data) {
  const event::EventCombatBuildAction *event = (const EventCombatBuildAction *)data;

  CombatField *combat_field =
    CombatFieldManager::GetInstance()->Get(event->combat_id());
  if (combat_field == NULL) {
    MYSYA_ERROR("CombatFieldManager::Get(%d) failed.", event->combat_id());
    return;
  }

  combat_field->PushAction(event->action());

  ::protocol::MessageCombatActionSync message;
  *message.mutable_action() = event->action();
  combat_field->BroadcastMessage(::protocol::MESSAGE_COMBAT_ACTION_SYNC, message);
}

void CombatEventHandler::EventCombatMoveAction(const ProtoMessage *data) {
  const event::EventCombatMoveAction *event = (const EventCombatMoveAction *)data;

  CombatField *combat_field =
    CombatFieldManager::GetInstance()->Get(event->combat_id());
  if (combat_field == NULL) {
    MYSYA_ERROR("CombatFieldManager::Get(%d) failed.", event->combat_id());
    return;
  }

  combat_field->PushAction(event->action());

  ::protocol::MessageCombatActionSync message;
  *message.mutable_action() = event->action();
  combat_field->BroadcastMessage(::protocol::MESSAGE_COMBAT_ACTION_SYNC, message);
}

#undef SCENE_APP

}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial
