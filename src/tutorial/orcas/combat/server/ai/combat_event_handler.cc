#include "tutorial/orcas/combat/server/ai/combat_event_handler.h"

#include <google/protobuf/message.h>
#include <mysya/ioevent/logger.h>

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {
namespace ai {

#define AI_APP() \
    AiApp::GetInstance()

CombatEventHandler::CombatEventHandler()
  : event_token_build_action_(0),
    event_token_move_action_(0) {}
CombatEventHandler::~CombatEventHandler() {}

bool CombatEventHandler::Initialize() {
  this->event_token_build_action_ =
    AI_APP()->GetEventDispatcher()->Attach(event::EVENT_COMBAT_BUILD_ACTION,
        std::bind(&CombatEventHandler::OnEventCombatBuildAction, this, std::placeholders::_1));
  this->event_token_build_action_ =
    AI_APP()->GetEventDispatcher()->Attach(event::EVENT_COMBAT_BUILD_ACTION,
        std::bind(&CombatEventHandler::OnEventCombatMoveAction, this, std::placeholders::_1));

  return true;

}

void CombatEventHandler::Finalize() {
  AI_APP()->GetEventDispatcher()->Detach(this->event_token_build_action_);
  AI_APP()->GetEventDispatcher()->Detach(this->event_token_move_action_);
}

void CombatEventHandler::OnEventCombatBuildAction(const ProtoMessage *data) {
  const event::EventCombatBuildAction *event = (const event::EventCombatBuildAction *)data;

  CombatField *combat_field =
    CombatFieldManager::GetInstance()->Get(event->combat_id());
  if (combat_field == NULL) {
    MYSYA_ERROR("[AI] CombatFieldManager::Get(%d) failed.", event->combat_id());
    return;
  }

  int32_t warrior_id = event->action().build_action().fields().id();
  CombatWarriorField *combat_warrior_field = combat_field->GetWarrior(warrior_id);
  if (combat_warrior_field == NULL) {
    MYSYA_ERROR("[AI] CombatField::GetWarrior(%d) failed.", warrior_id);
    return;
  }

  Auto *autoz = AutoManager::GetInstance()->Allocate();
  if (autoz == NULL) {
    MYSYA_ERROR("[AI] AutoManager;:Allocate() failed.");
    return;
  }

  if (autoz->Initialize(combat_warrior_field) == false) {
    MYSYA_ERROR("[AI] Auto::Initialize() failed.");
    AutoManager::GetInstance()->Deallocate(autoz);
    return;
  }

  if (AutoManager::GetInstance()->Add(autoz) == false) {
    MYSYA_ERROR("[AI] AutoManager::Add() failed.");
    autoz->Finalize();
    AutoManager::GetInstance()->Deallocate(autoz);
    return;
  }
}

void CombatEventHandler::OnEventCombatMoveAction(const ProtoMessage *data) {
}

#undef AI_APP

}  // namespace ai
}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial