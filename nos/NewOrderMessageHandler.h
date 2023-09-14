#ifndef NEWORDERMESSAGEHANDLER_H
#define NEWORDERMESSAGEHANDLER_H

#include <twLib/mq/MessageHandler.h>

#include <OR2Lib/ORConfigReader.h>
#include <kr/MismatchedRootsMap.h>


namespace TW {
  class OR2Adapter;
  class MQAdapter;
  class JsonOrderInterpreter;
  class SenderLocationReader;
}

class NewOrderMessageHandler final : public TW::MessageHandler {

public:
  NewOrderMessageHandler(TW::OR2Adapter *pOR2Adapter, TW::MQAdapter *pMQAdapter, const ORConfigReader::Config &config, TW::SenderLocationReader* pSenderLocationReader,
                         bool bDefaultRoute = false);

  bool handleMessage(nlohmann::json &jMessage, std::string strTopic) override;

private:
  TW::OR2Adapter *m_pOR2Adapter;

  TW::MQAdapter *m_pMQAdapter;
  TW::SenderLocationReader *m_pSenderLocationReader;
  const ORConfigReader::Config m_config;
  MismatchedRootsMap m_rootMap;
  bool m_bDefaultRoute;

  void rejectMessageOrder(const TW::JsonOrderInterpreter &orderWrapper, const std::string &strRejectionMessage) const;
};


#endif //NEWORDERMESSAGEHANDLER_H
