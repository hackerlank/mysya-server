#ifndef TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_H
#define TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_H

namespace tutorial {
namespace orcas {
namespace combat {
namespace server {
namespace ai {

class AutoStatus;
class CombatWarriorField;

class Auto {
 public:
  Auto();
  ~Auto();

  bool Initialize(CombatWarriorField *host);
  void Finalize();

  CombatWarriorField *GetHost();

  void SetTarget(::protocol::CombatEntityType type, int32_t id);
  ::protocol::CombatTarget &GetTarget();

  AutoStatus *GetPresentStatus();
  void GotoStatus(int status);

 private:
  CombatWarriorField *host_;
  ::protocol::CombatTarget target_;

  AutoStatus *present_status_;
  AutoStatusSearch status_search_;
  AutoStatusMove status_move_;
  AutoStatusAttack status_attack_;
};

}  // namespace ai
}  // namespace server
}  // namespace combat
}  // namespace orcas
}  // namespace tutorial

#endif  // TUTORIAL_ORCAS_COMBAT_SERVER_AI_AUTO_H
