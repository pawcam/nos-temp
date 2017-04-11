//
// Created by nriffe on 10/14/15.
//
#include "NewOrderMessageHandler.h"

#include <twLib/ExceptionTypes.h>
#include <twLib/JsonOrderInterpreter.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/mq/MQUtil.h>
#include <twLib/or/OR2Adapter.h>


NewOrderMessageHandler::NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter)
  : m_pOR2Adapter(pOR2Adapter), m_pMQAdapter(pMQAdapter)
{
  m_rootMap.initFromDBOption("db_option_combined.out", true);
}

bool NewOrderMessageHandler::handleMessage(nlohmann::json &jMessage, std::string UNUSED(strTopic)) {
  uint32_t nOrderNum = numeric_limits<uint32_t>::max();

  std::string strRejectionMessage;

  TW::JsonOrderInterpreter orderWrapper = TW::JsonOrderInterpreter(jMessage);
  const string strDestination = MQUtil::extractDestination(jMessage, m_pOR2Adapter->getDefaultRoute());

  try {
    sxORMsgWithType szMsg = orderWrapper.to_OR2MessageStruct(m_rootMap);
    if (szMsg.h.uchType == sxORMsgWithType::MSG_NEW_ORDER_WITH_ACCOUNT) {
      nOrderNum = m_pOR2Adapter->sendOrder(szMsg.u.nowa, strDestination);
    } else if (szMsg.h.uchType == sxORMsgWithType::MSG_COMPLEX_WRAPPER) { // Order is a spread (multi-leg)
      msg_ComplexOrderWrapper &comp = szMsg.u.comp;
      msg_NewOrderWithAccount *pNowa = (msg_NewOrderWithAccount *) comp.beginMsgStruct();
      nOrderNum = m_pOR2Adapter->sendOrder(*pNowa, comp.getLeg(0), comp.nLegs, strDestination);
    }
  } catch (const TW::invalid_order& e) {
    rejectMessageOrder(orderWrapper, e.what());
    return false;
  }

  // We get the bogus client order number in one of two circumstances:
  // 1) the client's sequence has topped out for the day, and the client needs to be restarted
  // 2) the client is not actually connected to anything that it can send orders to
  // In either case, the failure is an infrastructure failure, and not a semantic failure.
  bool isOrderBogus = (nOrderNum == numeric_limits<uint32_t>::max());
  std::string strRoutingKey;
  json j;
  if (isOrderBogus) {
    rejectMessageOrder(orderWrapper,  "Order sequence number has reached its limit or client lost connection.");
  } else {
    j = {
      {"account-number",          orderWrapper.getAccountNumber()},
      {"id",                      orderWrapper.getOrderId()},
      {"status",                  "Placed"},
      {"destination-venue",       strDestination}
    };
    strRoutingKey = MQUtil::getOrderPlacementRoutingKey(orderWrapper.getAccountNumber(),
                                                        orderWrapper.getOrderId());

    m_pMQAdapter->publish(strRoutingKey, j);
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
