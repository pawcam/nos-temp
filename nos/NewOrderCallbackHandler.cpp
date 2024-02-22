#include "NewOrderCallbackHandler.h"

#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/serialize/JsonifyOR2Client.hpp>
#include <twLib/util.h>

#include "kr/common_enums.h"

NewOrderCallbackHandler::NewOrderCallbackHandler(TW::MQAdapter *pMQAdapter, const ORConfigReader::Config& config)
  : m_pMQAdapter(pMQAdapter)
  , m_config(config) {}

void NewOrderCallbackHandler::routeStatus(uint32_t nRouteId, const std::string& strRoute, bool bUp)
{
  SX_WARN("routeStatus[routeId:%u][route:%s][up:%d]\n", nRouteId, strRoute, bUp);
}

void NewOrderCallbackHandler::statusUpdate(const TOptionID &UNUSED(optID), const std::string& strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const char *UNUSED(szStock), const std::string& strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const msg_ComplexOrderWrapper &UNUSED(wrap), const std::string& strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const TFutureID &UNUSED(futID), const std::string& strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const TFutureOptionID &UNUSED(futOptID), const std::string& strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::publishStatusUpdate(const msg_StatusUpdate &message, const std::string &strRoute) {
  auto&& strMessageTime = TW::to_iso8601_string(microsec_clock::universal_time());
  auto eExchangeStatus  = sx::ExchangeStatus(message.nExchangeStatus);

  const nlohmann::json j = {
    {"account-number",              message.szAccount},
    {"exchange-status",             serialize::tostring(eExchangeStatus)},
    {"id",                          message.nIdentifier},
    {"ext-global-order-number",     message.nGlobalOrderNum},
    {"destination-venue",           strRoute},
    {"in-flight-at",                strMessageTime},
    {"ext-exchange-order-number",   message.nExchangesOrderNum}
  };
  auto&& strRoutingKey = MQUtil::getOrderInFlightRoutingKey(message.szAccount, message.nIdentifier);
  m_pMQAdapter->publish(strRoutingKey, j);
}
