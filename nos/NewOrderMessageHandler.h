#ifndef NEWORDERMESSAGEHANDLER_H
#define NEWORDERMESSAGEHANDLER_H

#include <twLib/mq/MessageHandler.h>

#include <OR2Lib/ORConfigReader.h>
#include <kr/MismatchedRootsMap.h>


namespace TW {
  class OR2Adapter;
  class MQAdapter;
  class JsonOrderInterpreter;
}

class NewOrderMessageHandler : public TW::MessageHandler {

public:
  NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter, ORConfigReader::Config &config);

  virtual bool handleMessage(nlohmann::json &jMessage, std::string strTopic);

private:
  TW::OR2Adapter *m_pOR2Adapter;

  TW::MQAdapter *m_pMQAdapter;

  ORConfigReader::Config m_config;
  void rejectMessageOrder(const TW::JsonOrderInterpreter &orderWrapper, const std::string &strRejectionMessage) const;

  MismatchedRootsMap m_rootMap;
};


#endif //NEWORDERMESSAGEHANDLER_H
