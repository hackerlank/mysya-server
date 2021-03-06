#include "tutorial/orcas/combat/server/ai/auto_status_attack.h"

#include <functional>

#include <mysya/ioevent/logger.h>

#include "tutorial/orcas/combat/server/app_server.h"
#include "tutorial/orcas/combat/server/combat_field.h"
#include "tutorial/orcas/combat/server/combat_warrior_field.h"
#include "tutorial/orcas/combat/server/ai/ai_app.h"
#include "tutorial/orcas/combat/server/ai/auto.h"
#include "tutorial/orcas/combat/server/ai/building.h"
#include "tutorial/orcas/combat/server/ai/building_manager.h"
#include "tutorial/orcas/combat/server/event/cc/event.pb.h"
#include "tutorial/orcas/combat/server/event/cc/event_combat.pb.h"
#include "tutorial/orcas/combat/server/event/cc/event_scene.pb.h"
#include "tutorial/orcas/protocol/cc/warrior.pb.h"

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {
namespace ai {

AutoStatusAttack::AutoStatusAttack(Auto *host)
  : AutoStatus(host), timer_id_attack_(-1) {
  this->AttachEvent(event::EVENT_SCENE_MOVE_STEP, std::bind(
        &AutoStatusAttack::OnEventSceneMoveStep, this, std::placeholders::_1));
  this->AttachEvent(event::EVENT_COMBAT_DEATH, std::bind(
        &AutoStatusAttack::OnEventCombatDeath, this, std::placeholders::_1));
  this->AttachEvent(event::EVENT_COMBAT_CONVERT_CAMP, std::bind(
        &AutoStatusAttack::OnEventCombatConvertCamp, this, std::placeholders::_1));
}

AutoStatusAttack::~AutoStatusAttack() {
  this->DetachEvent(event::EVENT_SCENE_MOVE_STEP);
  this->DetachEvent(event::EVENT_COMBAT_DEATH);
  this->DetachEvent(event::EVENT_COMBAT_CONVERT_CAMP);
}

void AutoStatusAttack::Start() {
  this->SetAttackTimer();

  event::EventCombatLockTarget event;
  event.set_combat_id(this->host_->GetHost()->GetCombatField()->GetId());
  event.set_warrior_id(this->host_->GetId());
  *event.mutable_target() = this->host_->GetTarget();
  AiApp::GetInstance()->GetEventDispatcher()->Dispatch(event::EVENT_COMBAT_LOCK_TARGET, &event);

  const ::protocol::CombatEntity &target = this->host_->GetTarget();
  if (target.type() == ::protocol::COMBAT_ENTITY_TYPE_BUILDING) {
    Building *building = BuildingManager::GetInstance()->Get(
        this->host_->GetCombatId(), target.id());
    if (building != NULL) {
      building->IncTargetedNum();
    }
  }
}

void AutoStatusAttack::Stop() {
  const ::protocol::CombatEntity &target = this->host_->GetTarget();
  if (target.type() == ::protocol::COMBAT_ENTITY_TYPE_BUILDING) {
    Building *building = BuildingManager::GetInstance()->Get(
        this->host_->GetCombatId(), target.id());
    if (building != NULL) {
      building->DecTargetedNum();
    }
  }

  this->ResetAttackTimer();
  this->host_->ResetTarget();
}

AutoStatus::type AutoStatusAttack::GetType() const {
  return AutoStatus::ATTACK;
}

void AutoStatusAttack::SetAttackTimer() {
  CombatWarriorField *combat_warrior_field = this->host_->GetHost();
  if (combat_warrior_field == NULL) {
    MYSYA_ERROR("[AI] Auto::GetHost() failed.");
    return;
  }

  const ::protocol::CombatWarriorFields &fields = combat_warrior_field->GetFields();
  int attack_interval_ms = fields.attack_speed();

  this->timer_id_attack_ = AiApp::GetInstance()->GetHost()->StartTimer(
      attack_interval_ms, std::bind(&AutoStatusAttack::OnTimerAttack,
        this, std::placeholders::_1));
}

void AutoStatusAttack::ResetAttackTimer() {
  if (this->timer_id_attack_ != -1) {
    AiApp::GetInstance()->GetHost()->StopTimer(this->timer_id_attack_);
    this->timer_id_attack_ = -1;
  }
}

void AutoStatusAttack::OnTimerAttack(int64_t timer_id) {
  this->host_->AttackTarget();
}

void AutoStatusAttack::OnEventSceneMoveStep(const ProtoMessage *data) {
  event::EventSceneMoveStep *event = (event::EventSceneMoveStep *)data;

  const ::protocol::CombatEntity &target = this->host_->GetTarget();
  if (target.id() != event->warrior_id() ||
      target.type() != ::protocol::COMBAT_ENTITY_TYPE_WARRIOR) {
    return;
  }

  const ::protocol::CombatWarriorFields &warrior_fields =
    this->host_->GetHost()->GetFields();

  int target_distance = this->host_->GetTargetDistance();
  if (target_distance < 0) {
    MYSYA_ERROR("[AI] Auto::GetTargetDistance() failed.");
    return;
  }

  // TODO: if target_distance > search_range giveup target, and goto search status.
  if (target_distance > warrior_fields.attack_range()) {
    this->GotoStatus(AutoStatus::CHASE);
    return;
  }
}

void AutoStatusAttack::OnEventCombatDeath(const ProtoMessage *data) {
  event::EventCombatDeath *event = (event::EventCombatDeath *)data;

  const ::protocol::CombatEntity &target = this->host_->GetTarget();
  if (target.id() != event->target().id() || target.type() != event->target().type()) {
    return;
  }

  this->GotoStatus(AutoStatus::SEARCH);
}

void AutoStatusAttack::OnEventCombatConvertCamp(const ProtoMessage *data) {
  const event::EventCombatConvertCamp *event = (const event::EventCombatConvertCamp *)data;

  const ::protocol::CombatEntity &target = this->host_->GetTarget();
  if (target.id() != event->host().id() || target.type() != event->host().type()) {
    return;
  }

  this->GotoStatus(AutoStatus::SEARCH);
}

}  // namespace ai
}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial
