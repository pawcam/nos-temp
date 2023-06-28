#include "NewOrderCallbackHandler.h"

#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/serialize/JsonifyOR2Client.hpp>
#include <twLib/util.h>

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
  const TW::AcctNumber_t strAcct = message.szAccount;
  const uint32_t &nIdentifier = message.nIdentifier;
  const uint32_t &nGlobalOrderNum = message.nGlobalOrderNum;
  const uint64_t &nExchangesOrderNum = message.nExchangesOrderNum;
  const std::string strMessageTime = TW::to_iso8601_string(microsec_clock::universal_time());
  const sxORMsgWithType::ExchangeStatus eExchangeStatus = (sxORMsgWithType::ExchangeStatus)message.nExchangeStatus;

  const nlohmann::json j = {
    {"account-number",              strAcct},
    {"exchange-status",             serialize::tostring(eExchangeStatus)},
    {"id",                          nIdentifier},
    {"ext-global-order-number",     nGlobalOrderNum},
    {"destination-venue",           strRoute},
    {"in-flight-at",                strMessageTime},
    {"ext-exchange-order-number",   nExchangesOrderNum}
  };
  const std::string strRoutingKey = MQUtil::getOrderInFlightRoutingKey(strAcct, nIdentifier);
  m_pMQAdapter->publish(strRoutingKey, j);
}
