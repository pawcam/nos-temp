#include "NewOrderMessageHandler.h"

#include <twLib/JsonOrderInterpreter.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/mq/MQUtil.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/SenderLocationReader.h>
#include <OR2Lib/Account.h>

#include <string>
#include <exception>

NewOrderMessageHandler::NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter, const ORConfigReader::Config &config, TW::SenderLocationReader *pSenderLocationReader,
                                               bool bDefaultRoute)
  : m_pOR2Adapter(pOR2Adapter)
  , m_pMQAdapter(pMQAdapter)
  , m_pSenderLocationReader(pSenderLocationReader)
  , m_config(config)
  , m_bDefaultRoute(bDefaultRoute)
{
  m_rootMap.initFromDBOption("db_option_combined.out", true);
}

bool NewOrderMessageHandler::handleMessage(nlohmann::json &jMessage, std::string strTopic)
{
  uint32_t nOrderNum = numeric_limits<uint32_t>::max();

  if(sx_logGetBit(sx_log::SX_LOG_DEBUG))
  {
    SX_DEBUG("[%s]Attempting to handle message for order %s\n", strTopic, jMessage.dump());
  }

  TW::JsonOrderInterpreter orderWrapper = TW::JsonOrderInterpreter(jMessage, m_pSenderLocationReader);
  const std::string strDestination = m_bDefaultRoute ? m_pOR2Adapter->getDefaultRoute() : MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  try {
    sxORMsgWithType szMsg = orderWrapper.to_OR2MessageStruct(m_rootMap);

    if (szMsg.h.uchType == sxORMsgWithType::MSG_INVALID) {
      SX_ERROR("Failed to create order message: %d\n", orderWrapper.getOrderId());
      rejectMessageOrder(orderWrapper, "Failed to create order message: " + std::to_string(orderWrapper.getOrderId()));
      return false;
    }

    if (orderWrapper.getAccountNumber().length() != (OR2::Account::t_account_size - 1)) {
      SX_ERROR("This account is invalid (%s) %d.\n", orderWrapper.getAccountNumber(), szMsg.u.nowa.nIdentifier);
      rejectMessageOrder(orderWrapper, "This account is invalid (" + orderWrapper.getAccountNumber() + "), " + std::to_string(szMsg.u.nowa.nIdentifier) + ".\n");
      return false;
    }

    if (szMsg.h.uchType == sxORMsgWithType::MSG_NEW_ORDER_WITH_ACCOUNT) {
      nOrderNum = m_pOR2Adapter->sendOrder(szMsg.u.nowa, strDestination);
      SX_DEBUG("Sent order (single): %d \n", szMsg.u.nowa.nIdentifier);
    }
    else if (szMsg.h.uchType == sxORMsgWithType::MSG_COMPLEX_WRAPPER) { // Order is a spread (multi-leg)
      msg_ComplexOrderWrapper &comp = szMsg.u.comp;
      auto pNowa = comp.toMsgStruct<msg_NewOrderWithAccount *>();
      nOrderNum = m_pOR2Adapter->sendOrder(*pNowa, comp.getLeg(0), comp.nLegs, strDestination);
      SX_DEBUG("Sent order (complex): %d \n", pNowa->nIdentifier);
    }
  } catch (const std::exception &e) {
    SX_ERROR("Failed to process order - JSON Parser Error %s\n", e.what());
    m_pMQAdapter->publishToErrorQueue("Failed to process order - JSON Parser Error " + std::string(e.what()));
    return false;
  }

  // We get the bogus client order number in one of two circumstances:
  // 1) the client's sequence has topped out for the day, and the client needs to be restarted
  // 2) the client is not actually connected to anything that it can send orders to
  // In either case, the failure is an infrastructure failure, and not a semantic failure.
  bool isOrderBogus = (nOrderNum == numeric_limits<uint32_t>::max());
  if (isOrderBogus) {
    SX_ERROR("OR:%s: Order sequence number has reached its limit or client lost connection\n", strDestination);
    rejectMessageOrder(orderWrapper, "OR:" + strDestination + ": Order sequence number has reached its limit or client lost connection");
  }

  return !isOrderBogus;
}

void NewOrderMessageHandler::rejectMessageOrder(const TW::JsonOrderInterpreter &orderWrapper, const std::string &strRejectionMessage) const {
  nlohmann::json j = {
    {"account-number",          orderWrapper.getAccountNumber()},
    {"id",                      orderWrapper.getOrderId()},
    {"status",                  "Rejected"},
    {"reject-detail",           strRejectionMessage }
  };
  const std::string strRoutingKey = MQUtil::getClientFailureRoutingKey(orderWrapper.getAccountNumber(),
                                                                 orderWrapper.getOrderId());
  m_pMQAdapter->publish(strRoutingKey, j);
}
