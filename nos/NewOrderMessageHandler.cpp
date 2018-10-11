//
// Created by nriffe on 10/14/15.
//
#include "NewOrderMessageHandler.h"

#include <twLib/ExceptionTypes.h>
#include <twLib/JsonOrderInterpreter.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/mq/MQUtil.h>
#include <twLib/or/OR2Adapter.h>


NewOrderMessageHandler::NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter, ORConfigReader::Config &config)
  : m_pOR2Adapter(pOR2Adapter), m_pMQAdapter(pMQAdapter), m_config(config)
{
  m_rootMap.initFromDBOption("db_option_combined.out", true);
}

bool NewOrderMessageHandler::handleMessage(nlohmann::json &jMessage, std::string UNUSED(strTopic)) {
  uint32_t nOrderNum = numeric_limits<uint32_t>::max();

  std::string strRejectionMessage;

  SX_DEBUG("Attempting to handle message for order %s\n", jMessage.dump());

  TW::JsonOrderInterpreter orderWrapper = TW::JsonOrderInterpreter(jMessage);
  const string strDestination = MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());


  try {
    sxORMsgWithType szMsg = orderWrapper.to_OR2MessageStruct(m_rootMap);
      // The OEL doesn't know about this account, reject this order
      if(m_config.hmAccountMap.find(orderWrapper.getAccountNumber()) == m_config.hmAccountMap.end()) {
        rejectMessageOrder(orderWrapper, "This account is unknown, it may not be configured for this product");
        return false;
      }

    if (szMsg.h.uchType == sxORMsgWithType::MSG_NEW_ORDER_WITH_ACCOUNT) {
      nOrderNum = m_pOR2Adapter->sendOrder(szMsg.u.nowa, strDestination);
      SX_DEBUG("Sent order (single): %d \n", szMsg.u.nowa.nIdentifier);
    } else if (szMsg.h.uchType == sxORMsgWithType::MSG_COMPLEX_WRAPPER) { // Order is a spread (multi-leg)
      msg_ComplexOrderWrapper &comp = szMsg.u.comp;
      msg_NewOrderWithAccount *pNowa = (msg_NewOrderWithAccount *) comp.beginMsgStruct();
      nOrderNum = m_pOR2Adapter->sendOrder(*pNowa, comp.getLeg(0), comp.nLegs, strDestination);
      SX_DEBUG("Sent order (complex): %d \n", pNowa->nIdentifier);
    }
  } catch (const TW::invalid_order& e) {
    SX_DEBUG("Failed to process btorder - Exception: %s \n", e.what());
    rejectMessageOrder(orderWrapper, e.what());
    return false;
  } catch (const std::exception& e) {
    SX_DEBUG("Failed to process order - JSON Parser Error %s \n", e.what());
    m_pMQAdapter->publishToErrorQueue(std::string("Failed to process order - JSON Parser Error ") + e.what());
    return false;
  }

  // We get the bogus client order number in one of two circumstances:
  // 1) the client's sequence has topped out for the day, and the client needs to be restarted
  // 2) the client is not actually connected to anything that it can send orders to
  // In either case, the failure is an infrastructure failure, and not a semantic failure.
  bool isOrderBogus = (nOrderNum == numeric_limits<uint32_t>::max());
  if (isOrderBogus) {
    const std::string strRejectionMessage =  "OR:" + strDestination + ": Order sequence number has reached its limit or client lost connection.";
    rejectMessageOrder(orderWrapper,  strRejectionMessage);
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
  std::string strRoutingKey = MQUtil::getClientFailureRoutingKey(orderWrapper.getAccountNumber(),
                                                                 orderWrapper.getOrderId());
  m_pMQAdapter->publish(strRoutingKey, j);
}
