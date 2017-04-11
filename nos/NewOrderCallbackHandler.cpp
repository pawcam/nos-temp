#include "NewOrderCallbackHandler.h"
#include <boost/lexical_cast.hpp>

#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/serialize/JsonifyOR2Client.hpp>
#include <twLib/util.h>


using namespace std;
using namespace boost;
NewOrderCallbackHandler::NewOrderCallbackHandler(TW::MQAdapter *pMQAdapter)
  : m_pMQAdapter(pMQAdapter) { }

void NewOrderCallbackHandler::statusUpdate(const TOptionID &UNUSED(optID), const string &strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const char *UNUSED(szStock), const string &strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const msg_ComplexOrderWrapper &UNUSED(wrap), const string &strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const TFutureID &UNUSED(futID), const string &strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::statusUpdate(const TFutureOptionID &UNUSED(futOptID), const string &strRoute,
                                           const msg_StatusUpdate &stat) {
  publishStatusUpdate(stat, strRoute);
}

void NewOrderCallbackHandler::publishStatusUpdate(const msg_StatusUpdate &message, const string &strRoute) {
  const TW::AcctNumber_t &nAcctNumber = TW::accountIdToAccountNumber(message.nAcct);
  const uint32_t &nIdentifier = message.nIdentifier;
  const uint32_t &nGlobalOrderNum = message.nGlobalOrderNum;
  const uint64_t &nExchangesOrderNum = message.nExchangesOrderNum;
  const string strMessageTime = TW::to_iso8601_string(microsec_clock::universal_time());
  const sxORMsgWithType::ExchangeStatus eExchangeStatus = (sxORMsgWithType::ExchangeStatus)message.nExchangeStatus;

  const nlohmann::json j = {
    {"account-number",              nAcctNumber},
    {"exchange-status",             serialize::tostring(eExchangeStatus)},
    {"id",                          nIdentifier},
    {"ext-global-order-number",     nGlobalOrderNum},
    {"destination-venue",           strRoute},
    {"in-flight-at",                strMessageTime},
    {"ext-exchange-order-number",   nExchangesOrderNum}
  };
  const string strRoutingKey = MQUtil::getOrderInFlightRoutingKey(nAcctNumber, nIdentifier);
  m_pMQAdapter->publish(strRoutingKey, j);
}

