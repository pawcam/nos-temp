#include "NewOrderCallbackHandler.h"
#include <boost/lexical_cast.hpp>

#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/serialize/JsonifyOR2Client.hpp>
#include <twLib/util.h>


using namespace std;
using namespace boost;
NewOrderCallbackHandler::NewOrderCallbackHandler(TW::MQAdapter *pMQAdapter, ORConfigReader::Config& config)
  : m_pMQAdapter(pMQAdapter)
  , m_config(config) {}

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
  const TW::AcctNumber_t strAcct = m_config.accountIdToAccountNumber(message.nAcct);
  if (strAcct == "") {
    handleAccountNumberNotFound(message.nAcct);
    return;
  }

  const uint32_t &nIdentifier = message.nIdentifier;
  const uint32_t &nGlobalOrderNum = message.nGlobalOrderNum;
  const uint64_t &nExchangesOrderNum = message.nExchangesOrderNum;
  const string strMessageTime = TW::to_iso8601_string(microsec_clock::universal_time());
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
  const string strRoutingKey = MQUtil::getOrderInFlightRoutingKey(strAcct, nIdentifier);
  m_pMQAdapter->publish(strRoutingKey, j);
}

void NewOrderCallbackHandler::handleAccountNumberNotFound(int32_t nAccount) const
{
  const std::string errorMessage = boost::str(boost::format("Account number not found for ID = %d") % nAccount);

  SX_WARN("%s\n", errorMessage);
  m_pMQAdapter->publishToErrorQueue(errorMessage);
}