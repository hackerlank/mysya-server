#include "tutorial/orcas/combat/server/combat_warrior_field.h"

#include <mysya/ioevent/logger.h>

#include "tutorial/orcas/combat/server/combat_role_field.h"

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {

CombatWarriorField::CombatWarriorField() : host_(NULL) {}
CombatWarriorField::~CombatWarriorField() {}

bool CombatWarriorField::Initialize(int32_t id, CombatRoleField *host,
    const ::protocol::WarriorDescription &description) {
  this->host_ =  host;

  this->fields_.set_id(id);
  this->fields_.set_conf_id(description.id());
  this->fields_.set_type(description.type());
  this->fields_.set_host_id(this->host_->GetArgentId());
  this->fields_.set_camp_id(this->host_->GetCampId());
  this->fields_.set_hp(description.max_hp());
  this->fields_.set_max_hp(description.max_hp());
  this->fields_.set_attack(description.attack());
  this->fields_.set_defence(description.defence());
  this->fields_.set_attack_speed(description.attack_speed());
  this->fields_.set_move_speed(description.move_speed());
  this->fields_.set_attack_range(description.attack_range());
  this->fields_.set_origin_pos_x(0);
  this->fields_.set_origin_pos_y(0);
  this->fields_.set_dest_pos_x(0);
  this->fields_.set_dest_pos_y(0);

  this->server_fields_.set_max_hp_add_value(0);
  this->server_fields_.set_max_hp_add_per(0);
  this->server_fields_.set_attack_add_value(0);
  this->server_fields_.set_attack_add_per(0);
  this->server_fields_.set_defence_add_value(0);
  this->server_fields_.set_defence_add_per(0);
  this->server_fields_.set_move_speed_add_value(0);
  this->server_fields_.set_move_speed_add_per(0);
  this->server_fields_.set_attack_speed_add_value(0);
  this->server_fields_.set_attack_speed_add_per(0);

  this->GenerateFields();

  return true;
}

void CombatWarriorField::Finalize() {
  this->host_ = NULL;
  this->fields_.Clear();
  this->server_fields_.Clear();
}

int32_t CombatWarriorField::GetId() const {
  return this->fields_.id();
}

::protocol::CombatWarriorFields &CombatWarriorField::GetFields() {
  return this->fields_;
}

::protocol::CombatWarriorServerFields &CombatWarriorField::GetServerFields() {
  return this->server_fields_;
}

void CombatWarriorField::GenerateFields() {
  const ::protocol::WarriorDescription *description =
    this->host_->GetWarriorDescription(this->fields_.conf_id());
  if (description == NULL) {
    MYSYA_ERROR("CombatRoleField[%lu]::GetWarriorDescription(%d) failed.",
        this->host_->GetArgentId(), this->fields_.conf_id());
    return;
  }

#define GENERATE_COMBAT_FIELD(base, add , per) \
  ((base + add) * (10000 + per) / 10000)

  this->fields_.set_max_hp(
      GENERATE_COMBAT_FIELD(description->max_hp(),
        this->server_fields_.max_hp_add_value(),
        this->server_fields_.max_hp_add_per()));
  this->fields_.set_attack(
      GENERATE_COMBAT_FIELD(description->attack(),
        this->server_fields_.attack_add_value(),
        this->server_fields_.attack_add_per()));
  this->fields_.set_defence(
      GENERATE_COMBAT_FIELD(description->defence(),
        this->server_fields_.defence_add_value(),
        this->server_fields_.defence_add_per()));
  this->fields_.set_attack_speed(
      GENERATE_COMBAT_FIELD(description->attack_speed(),
        this->server_fields_.move_speed_add_value(),
        this->server_fields_.move_speed_add_per()));
  this->fields_.set_move_speed(
      GENERATE_COMBAT_FIELD(description->move_speed(),
        this->server_fields_.attack_speed_add_value(),
        this->server_fields_.attack_speed_add_per()));

#undef GENERATE_COMBAT_FIELD
}

}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial
