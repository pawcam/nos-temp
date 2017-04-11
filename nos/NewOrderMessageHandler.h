#ifndef NEWORDERMESSAGEHANDLER_H
#define NEWORDERMESSAGEHANDLER_H

#include <twLib/mq/MessageHandler.h>

#include <kr/MismatchedRootsMap.h>


namespace TW {
  class OR2Adapter;
  class MQAdapter;
  class JsonOrderInterpreter;
}

class NewOrderMessageHandler : public TW::MessageHandler {

public:
  NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter);

  virtual bool handleMessage(nlohmann::json &jMessage, std::string strTopic);

private:
  TW::OR2Adapter *m_pOR2Adapter;

  TW::MQAdapter *m_pMQAdapter;

  void rejectMessageOrder(const TW::JsonOrderInterpreter &orderWrapper, const std::string &strRejectionMessage) const;

  MismatchedRootsMap m_rootMap;
};


#endif //NEWORDERMESSAGEHANDLER_H
