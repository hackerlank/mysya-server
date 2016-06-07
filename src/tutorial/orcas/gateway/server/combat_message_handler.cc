#include "tutorial/orcas/gateway/server/combat_message_handler.h"

#include <google/protobuf/message.h>
#include <mysya/ioevent/logger.h>

#include "tutorial/orcas/combat/transport_channel.h"
#include "tutorial/orcas/combat/client/combat_session.h"
#include "tutorial/orcas/combat/client/combat_sessions.h"
#include "tutorial/orcas/combat/protocol/cc/combat_message.pb.h"
#include "tutorial/orcas/gateway/server/actor.h"
#include "tutorial/orcas/gateway/server/app_server.h"
#include "tutorial/orcas/gateway/server/combat_actor.h"
#include "tutorial/orcas/gateway/server/combat_actor_manager.h"
#include "tutorial/orcas/gateway/server/combat_manager.h"
#include "tutorial/orcas/protocol/cc/message.pb.h"

namespace tutorial {
namespace orcas {
namespace gateway {
namespace server {

using namespace ::tutorial::orcas::combat;

CombatMessageHandler::CombatMessageHandler(AppServer *host)
  : host_(host) {}
CombatMessageHandler::~CombatMessageHandler() {}

void CombatMessageHandler::SetMessageHandlers() {
  this->host_->GetCombatClients().GetMessageDispatcher()->SetMessageCallback(
      protocol::MessageCombatDeployResponse().GetTypeName(), std::bind(
        &CombatMessageHandler::OnMessageCombatDeployResponse, this,
        std::placeholders::_1, std::placeholders::_2));
  this->host_->GetCombatClients().GetMessageDispatcher()->SetMessageCallback(
      protocol::MessageCombatConnectArgentResponse().GetTypeName(), std::bind(
        &CombatMessageHandler::OnMessageCombatConnectArgentResponse, this,
        std::placeholders::_1, std::placeholders::_2));
  this->host_->GetCombatClients().GetMessageDispatcher()->SetMessageCallback(
      protocol::MessageCombatBeginResponse().GetTypeName(), std::bind(
        &CombatMessageHandler::OnMessageCombatBeginResponse, this,
        std::placeholders::_1, std::placeholders::_2));
  this->host_->GetCombatClients().GetMessageDispatcher()->SetMessageCallback(
      protocol::MessageCombatArgentSync().GetTypeName(), std::bind(
        &CombatMessageHandler::OnMessageCombatArgentSync, this,
        std::placeholders::_1, std::placeholders::_2));
  this->host_->GetCombatClients().GetMessageDispatcher()->SetMessageCallback(
      protocol::MessageCombatSettlementSync().GetTypeName(), std::bind(
        &CombatMessageHandler::OnMessageCombatSettlementSync, this,
        std::placeholders::_1, std::placeholders::_2));
}

void CombatMessageHandler::ResetMessageHandlers() {
  this->host_->GetCombatClients().GetMessageDispatcher()->ResetMessageCallback(
      protocol::MessageCombatDeployResponse().GetTypeName());
  this->host_->GetCombatClients().GetMessageDispatcher()->ResetMessageCallback(
      protocol::MessageCombatConnectArgentResponse().GetTypeName());
  this->host_->GetCombatClients().GetMessageDispatcher()->ResetMessageCallback(
      protocol::MessageCombatBeginResponse().GetTypeName());
  this->host_->GetCombatClients().GetMessageDispatcher()->ResetMessageCallback(
      protocol::MessageCombatArgentSync().GetTypeName());
  this->host_->GetCombatClients().GetMessageDispatcher()->ResetMessageCallback(
      protocol::MessageCombatSettlementSync().GetTypeName());
}

void CombatMessageHandler::OnMessageCombatDeployResponse(
    TransportChannel *channel, const ProtoMessage *data) {
  client::CombatSession *session = (client::CombatSession *)channel;
  protocol::MessageCombatDeployResponse *message =
    (protocol::MessageCombatDeployResponse *)data;

  Combat *combat = CombatManager::GetInstance()->GetPending(message->host_id());
  if (combat == NULL) {
    MYSYA_ERROR("CombatManager::GetPending(%d) failed.", message->host_id());
    return;
  }

  CombatActor *left_combat_actor = combat->GetLeft();
  CombatActor *right_combat_actor = combat->GetRight();

  if (left_combat_actor == NULL || right_combat_actor == NULL) {
    MYSYA_ERROR("left_combat_actor/right_combat_actor is null.");
    return;
  }

  if (message->result_type() != protocol::COMBAT_DEPLOY_RESULT_TYPE_COMPLETE) {
    ::protocol::MessageCombatResponse response;
    response.set_result(false);

    if (left_combat_actor->GetActor() != NULL) {
      left_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    if (right_combat_actor->GetActor() != NULL) {
      right_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    CombatManager::GetInstance()->Deallocate(combat);

    return;
  }

  combat->SetCombatArgentId(message->combat_id());
  combat->SetCombatServerId(session->GetServerId());

  CombatManager::GetInstance()->RemovePending(combat->GetId());
  CombatManager::GetInstance()->AddCombat(combat);

  // 发送连接请求
  protocol::MessageCombatConnectArgentRequest connect_message;
  connect_message.set_combat_id(message->combat_id());
  connect_message.set_role_argent_id(left_combat_actor->GetCombatArgentId());
  session->SendMessage(connect_message);
  connect_message.set_role_argent_id(right_combat_actor->GetCombatArgentId());
  session->SendMessage(connect_message);

  MYSYA_DEBUG("MessageCombatDeployResponse success.");
}

void CombatMessageHandler::OnMessageCombatConnectArgentResponse(
    TransportChannel *channel, const ProtoMessage *data) {
  client::CombatSession *session = (client::CombatSession *)channel;
  protocol::MessageCombatConnectArgentResponse *message =
    (protocol::MessageCombatConnectArgentResponse *)data;

  Combat *combat = CombatManager::GetInstance()->GetCombat(
      session->GetServerId(), message->combat_id());
  if (combat == NULL) {
    MYSYA_ERROR("CombatManager::GetCombat(%d, %d) failed.",
        session->GetServerId(), message->combat_id());
    return;
  }

  CombatActor *left_combat_actor = combat->GetLeft();
  CombatActor *right_combat_actor = combat->GetRight();

  if (left_combat_actor == NULL || right_combat_actor == NULL) {
    MYSYA_ERROR("left_combat_actor/right_combat_actor is null.");
    return;
  }

  if (message->ret_code() != protocol::MessageCombatConnectArgentResponse::ERROR_CODE_COMPLETE) {
    ::protocol::MessageCombatResponse response;
    response.set_result(false);

    if (left_combat_actor->GetActor() != NULL) {
      left_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    if (right_combat_actor->GetActor() != NULL) {
      right_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    CombatManager::GetInstance()->Deallocate(combat);

    return;
  }

  combat->SetConnectedNum(combat->GetConnectedNum() + 1);
  if (combat->GetConnectedNum() >= 2) {
    protocol::MessageCombatBeginRequest begin_message;
    begin_message.set_combat_id(message->combat_id());
    session->SendMessage(begin_message);
  }
}

void CombatMessageHandler::OnMessageCombatBeginResponse(
    TransportChannel *channel, const ProtoMessage *data) {
  client::CombatSession *session = (client::CombatSession *)channel;
  protocol::MessageCombatBeginResponse *message =
    (protocol::MessageCombatBeginResponse *)data;

  Combat *combat = CombatManager::GetInstance()->GetCombat(
      session->GetServerId(), message->combat_id());
  if (combat == NULL) {
    MYSYA_ERROR("CombatManager::GetCombat(%d, %d) failed.",
        session->GetServerId(), message->combat_id());
    return;
  }

  CombatActor *left_combat_actor = combat->GetLeft();
  CombatActor *right_combat_actor = combat->GetRight();

  if (left_combat_actor == NULL || right_combat_actor == NULL) {
    MYSYA_ERROR("left_combat_actor/right_combat_actor is null.");
    return;
  }

  ::protocol::MessageCombatResponse response;

  if (message->ret_code() != protocol::MessageCombatConnectArgentResponse::ERROR_CODE_COMPLETE) {
    response.set_result(false);

    if (left_combat_actor->GetActor() != NULL) {
      left_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    if (right_combat_actor->GetActor() != NULL) {
      right_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    CombatManager::GetInstance()->Deallocate(combat);
  } else {
    response.set_result(true);
    // response.set_map_id(combat->GetMapId());
    *response.mutable_status_image() = message->status_image();
    response.mutable_combat_description()->set_map_id(combat->GetMapId());
    response.mutable_combat_description()->set_max_time(combat->GetMaxTime());

    response.set_host_id(left_combat_actor->GetCombatArgentId());
    response.set_camp_id(left_combat_actor->GetCampId());

    typedef CombatActor::WarriorDescriptionMap WarriorDescriptionMap;

    const WarriorDescriptionMap &left_combat_warriors = left_combat_actor->GetWarriors();
    for (WarriorDescriptionMap::const_iterator iter = left_combat_warriors.begin();
        iter != left_combat_warriors.end(); ++iter) {
      response.add_combat_warrior_id(iter->second.id());
    }

    if (left_combat_actor->GetActor() != NULL) {
      left_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }

    response.set_host_id(right_combat_actor->GetCombatArgentId());
    response.set_camp_id(right_combat_actor->GetCampId());

    response.clear_combat_warrior_id();
    const WarriorDescriptionMap &right_combat_warriors = right_combat_actor->GetWarriors();
    for (WarriorDescriptionMap::const_iterator iter = right_combat_warriors.begin();
        iter != right_combat_warriors.end(); ++iter) {
      response.add_combat_warrior_id(iter->second.id());
    }

    if (right_combat_actor->GetActor() != NULL) {
      right_combat_actor->GetActor()->SendMessage(
          ::protocol::MESSAGE_COMBAT_RESPONSE, response);
    }
  }
}

void CombatMessageHandler::OnMessageCombatArgentSync(TransportChannel *channel,
    const ProtoMessage *data) {
  client::CombatSession *session = (client::CombatSession *)channel;
  protocol::MessageCombatArgentSync *message = (protocol::MessageCombatArgentSync *)data;

  Combat *combat = CombatManager::GetInstance()->GetCombat(
      session->GetServerId(), message->combat_id());
  if (combat == NULL) {
    MYSYA_ERROR("CombatManager::GetCombat(%d, %d) failed.",
        session->GetServerId(), message->combat_id());
    return;
  }

  MYSYA_DEBUG("CombatMessageHandler::OnMessageCombatArgentSync rold_argent_id(%d).",
      message->role_argent_id());

  if (message->has_role_argent_id() == false) {
    combat->BroadcastMessage(message->type(), message->data());
  } else {
    CombatActor *combat_actor = combat->GetActorByArgentId(message->role_argent_id());
    if (combat_actor == NULL) {
      MYSYA_ERROR("CombatField::GetActorByArgentId(%lu) failed.", message->role_argent_id());
      return;
    }

    if (combat_actor->GetActor() == NULL) {
      MYSYA_ERROR("CombatActor::GetActor() failed.");
      return;
    }

    combat_actor->GetActor()->SendMessage(message->type(), message->data());

    MYSYA_DEBUG("Actor(%p) send message(%d)", combat_actor->GetActor(), message->type());
  }
}

void CombatMessageHandler::OnMessageCombatSettlementSync(TransportChannel *channel,
    const ProtoMessage *data) {
  MYSYA_DEBUG("CombatMessageHandler::OnMessageCombatSettlementSync.");

  client::CombatSession *session = (client::CombatSession *)channel;
  protocol::MessageCombatSettlementSync *message =
    (protocol::MessageCombatSettlementSync *)data;

  Combat *combat = CombatManager::GetInstance()->RemoveCombat(
      session->GetServerId(), message->combat_id());
  if (combat == NULL) {
    MYSYA_ERROR("CombatManager::ReomveCombat(%d) failed.", message->combat_id());
    return;
  }

  ::protocol::MessageCombatSettlementSync client_message;
  *client_message.mutable_settlement() = message->settlement();
  combat->BroadcastMessage(::protocol::MESSAGE_COMBAT_SETTLEMENT_SYNC, client_message);

  CombatManager::GetInstance()->Deallocate(combat);
}

}  // namespace server
}  // namespace gateway
}  // namespace orcas
}  // namespace tutorial
