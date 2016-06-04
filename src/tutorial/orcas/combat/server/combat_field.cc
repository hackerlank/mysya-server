#include "tutorial/orcas/combat/server/combat_field.h"

#include <google/protobuf/message.h>
#include <mysya/ioevent/logger.h>

#include "tutorial/orcas/combat/protocol/cc/combat_message.pb.h"
#include "tutorial/orcas/combat/server/app_server.h"
#include "tutorial/orcas/combat/server/app_session.h"
#include "tutorial/orcas/combat/server/combat_building_field.h"
#include "tutorial/orcas/combat/server/combat_building_field_pool.h"
#include "tutorial/orcas/combat/server/combat_role_field.h"
#include "tutorial/orcas/combat/server/combat_role_field_manager.h"
#include "tutorial/orcas/combat/server/combat_warrior_field.h"
#include "tutorial/orcas/combat/server/combat_warrior_field_pool.h"
#include "tutorial/orcas/combat/server/require/cc/require.pb.h"
#include "tutorial/orcas/combat/server/require/cc/require_combat.pb.h"

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {

CombatField::CombatField()
  : id_(0), map_id_(0), id_alloctor_(0),
    max_time_(0), timer_id_over_(-1),
    app_session_(NULL) {}

CombatField::~CombatField() {}

bool CombatField::Initialize(int32_t map_id, int32_t max_time,
    AppServer *app_server, AppSession *session) {
  this->max_time_ = 0;
  this->app_server_ = app_server;
  this->app_session_ = session;
  this->app_session_->Add(this);
  this->id_alloctor_ = 0;
  this->map_id_ = map_id;
  this->max_time_ = max_time;
  this->timer_id_over_ = -1;

  return true;
}

void CombatField::Finalize() {
  this->ResetSettleTimer();

  if (this->app_session_ != NULL) {
    this->app_session_->Remove(this);
  }

  for (WarriorFieldHashmap::iterator iter = this->warriors_.begin();
      iter != this->warriors_.end(); ++iter) {
    CombatWarriorField *warrior = iter->second;
    warrior->Finalize();
    CombatWarriorFieldPool::GetInstance()->Deallocate(warrior);
  }
  this->warriors_.clear();

  for (BuildingFieldMap::iterator iter = this->buildings_.begin();
      iter != this->buildings_.end(); ++iter) {
    CombatBuildingField *building = iter->second;
    building->Finalize();
    CombatBuildingFieldPool::GetInstance()->Deallocate(building);
  }
  this->buildings_.clear();

  for (CombatRoleFieldSet::const_iterator iter = this->roles_.begin();
      iter != this->roles_.end(); ++iter) {
    CombatRoleField *role = CombatRoleFieldManager::GetInstance()->Remove(*iter);
    if (role != NULL) {
      CombatRoleFieldManager::GetInstance()->Deallocate(role);
    }
  }
  this->roles_.clear();

  this->argent_roles_.clear();

  this->actions_.Clear();

  this->app_session_ = NULL;
  this->app_server_ = NULL;
}

void CombatField::SetSettleTimer() {
  this->timer_id_over_ = this->app_server_->StartTimer(this->max_time_ * 1000,
      std::bind(&CombatField::OnTimerSettle, this, std::placeholders::_1), 1);
}

void CombatField::ResetSettleTimer() {
  if (this->timer_id_over_ != -1) {
    this->app_server_->StopTimer(this->timer_id_over_);
    this->timer_id_over_ = -1;
  }
}

int32_t CombatField::GetId() const {
  return this->id_;
}

void CombatField::SetId(int32_t value) {
  this->id_ = value;
}

int32_t CombatField::GetMapId() const {
  return this->map_id_;
}

int32_t CombatField::AllocateId() {
  return ++this->id_alloctor_;
}

int32_t CombatField::GetMaxTime() const {
  return this->max_time_;
}

void CombatField::SetMaxTime(int32_t value) {
  this->max_time_ = value;
}

const ::mysya::util::Timestamp &CombatField::GetBeginTimestamp() const {
  return this->begin_timestamp_;
}

void CombatField::SetBeginTimestamp(const ::mysya::util::Timestamp &value) {
  this->begin_timestamp_ = value;
}

int64_t CombatField::GetTimestampSec() const {
  return this->begin_timestamp_.DistanceSecond(
      this->app_server_->GetTimestamp());
}

int64_t CombatField::GetTimestampMsec() const {
  return this->begin_timestamp_.DistanceMillisecond(
      this->app_server_->GetTimestamp());
}

void CombatField::AddRole(uint64_t role_argent_id) {
  CombatRoleField *combat_role_field =
    CombatRoleFieldManager::GetInstance()->Get(role_argent_id);
  if (combat_role_field == NULL) {
    MYSYA_ERROR("CombatRoleFieldManager::Get(%lu) failed.", role_argent_id);
    return;
  }

  this->roles_.insert(role_argent_id);
  combat_role_field->SetCombatField(this);
}

void CombatField::RemoveRole(uint64_t role_argent_id) {
  CombatRoleField *combat_role_field =
    CombatRoleFieldManager::GetInstance()->Get(role_argent_id);
  if (combat_role_field == NULL) {
    MYSYA_ERROR("CombatRoleFieldManager::Get(%lu) failed.", role_argent_id);
    return;
  }

  this->roles_.erase(role_argent_id);
  combat_role_field->SetCombatField(NULL);
}

const CombatField::CombatRoleFieldSet &CombatField::GetRoles() const {
  return this->roles_;
}

void CombatField::AddBuilding(CombatBuildingField *building) {
  this->buildings_.insert(std::make_pair(building->GetId(), building));
}

CombatBuildingField *CombatField::RemoveBuilding(int32_t building_id) {
  CombatBuildingField *building = NULL;

  BuildingFieldMap::iterator iter = this->buildings_.find(building_id);
  if (iter != this->buildings_.end()) {
    building = iter->second;
    this->buildings_.erase(iter);
  }

  return building;
}

CombatBuildingField *CombatField::GetBuilding(int32_t building_id) {
  CombatBuildingField *building = NULL;

  BuildingFieldMap::iterator iter = this->buildings_.find(building_id);
  if (iter != this->buildings_.end()) {
    building = iter->second;
  }

  return building;
}

const CombatField::BuildingFieldMap &CombatField::GetBuildings() const {
  return this->buildings_;
}

void CombatField::AddWarrior(CombatWarriorField *warrior) {
  this->warriors_.insert(std::make_pair(warrior->GetId(), warrior));
}

CombatWarriorField *CombatField::RemoveWarrior(int32_t warrior_id) {
  CombatWarriorField *warrior = NULL;

  WarriorFieldHashmap::iterator iter = this->warriors_.find(warrior_id);
  if (iter != this->warriors_.end()) {
    warrior = iter->second;
    this->warriors_.erase(iter);
  }

  return warrior;
}

CombatWarriorField *CombatField::GetWarrior(int32_t warrior_id) {
  CombatWarriorField *warrior = NULL;

  WarriorFieldHashmap::iterator iter = this->warriors_.find(warrior_id);
  if (iter != this->warriors_.end()) {
    warrior = iter->second;
  }

  return warrior;
}

const CombatField::WarriorFieldHashmap &CombatField::GetWarriors() const {
  return this->warriors_;
}

void CombatField::ResetAppSession() {
  this->app_session_ = NULL;
}

int CombatField::SendMessage(const ::google::protobuf::Message &message) {
  if (this->app_session_ == NULL) {
    return -1;
  }

  return this->app_session_->SendMessage(message);
}

int CombatField::BroadcastMessage(int type, const ::google::protobuf::Message &data) {
  ::tutorial::orcas::combat::protocol::MessageCombatArgentSync message;
  message.set_combat_id(this->id_);
  message.set_type(type);
  message.set_data(data.SerializeAsString());

  for (CombatRoleFieldSet::const_iterator iter = this->roles_.begin();
      iter != this->roles_.end(); ++iter) {
    CombatRoleField *role = CombatRoleFieldManager::GetInstance()->Get(*iter);
    if (role == NULL) {
      MYSYA_ERROR("CombatRoleFieldManager::Get(%lu) failed.", *iter);
      return -1;
    }
    message.set_role_argent_id(*iter);
    role->SendMessage(message);
  }
  return 0;
}

void CombatField::PushAction(const ::protocol::CombatAction &action) {
  *this->actions_.add_action() = action;
}

const ::protocol::CombatActionSequence &CombatField::GetActions() const {
  return this->actions_;
}

bool CombatField::RequireSettle() {
  require::RequireCombatSettle message;
  message.set_combat_id(this->GetId());
  if (this->app_server_->GetRequireDispatcher()->Dispatch(
        require::REQUIRE_COMBAT_SETTLE, &message) == -1) {
    MYSYA_ERROR("REQUIRE_COMBAT_SETTLE combat_id(%d) failed.",
        this->GetId());
    return false;
  }

  return true;
}

void CombatField::ExportStatusImage(::protocol::CombatStatusImage &image) const {
  for (BuildingFieldMap::const_iterator iter = this->buildings_.begin();
      iter != this->buildings_.end(); ++iter) {
    *image.add_building() = iter->second->GetFields();
  }

  for (WarriorFieldHashmap::const_iterator iter = this->warriors_.begin();
      iter != this->warriors_.end(); ++iter) {
    *image.add_warrior() = iter->second->GetFields();
  }

  for (CombatRoleFieldSet::const_iterator iter = this->roles_.begin();
      iter != this->roles_.end(); ++iter) {
    CombatRoleField *combat_role_field =
      CombatRoleFieldManager::GetInstance()->Get(*iter);
    if (combat_role_field == NULL) {
      MYSYA_ERROR("CombatRoleFieldManager::Get(%d) failed.", *iter);
      continue;
    }

    ::protocol::CombatRoleFields *role_fields = image.add_role();
    role_fields->set_id(combat_role_field->GetArgentId());
    role_fields->set_name(combat_role_field->GetName());
    role_fields->set_camp_id(combat_role_field->GetCampId());
  }
}

void CombatField::OnTimerSettle(int32_t id) {
  this->RequireSettle();
}

}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial
