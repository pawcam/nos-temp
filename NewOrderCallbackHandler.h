#ifndef NEWORDERCALLBACKHANDLER_H
#define NEWORDERCALLBACKHANDLER_H

#include <twLib/or/NoopOR2CallbackHandler.h>
#include <OR2Lib/ORConfigReader.h>

#include <string>

namespace TW {
  class MQAdapter;
}

class NewOrderCallbackHandler final : public TW::NoopOR2CallbackHandler {

public:
  NewOrderCallbackHandler(TW::MQAdapter *pMQAdapter, const ORConfigReader::Config& config);

  void routeStatus(uint32_t nRouteId, const std::string& strRoute, bool bUp) override;

  void statusUpdate(const TOptionID &optID, const std::string& strRoute,
                            const msg_StatusUpdate &stat) override;

  void statusUpdate(const char *szStock, const std::string& strRoute, const msg_StatusUpdate &stat) override;

  void statusUpdate(const msg_ComplexOrderWrapper &wrap, const std::string& strRoute,
                            const msg_StatusUpdate &stat) override;

  void statusUpdate(const TFutureID &futID, const std::string& strRoute,
                            const msg_StatusUpdate &stat) override;

  void statusUpdate(const TFutureOptionID &futOptID, const std::string& strRoute,
                            const msg_StatusUpdate &stat) override;


private:
  TW::MQAdapter *m_pMQAdapter;
  const ORConfigReader::Config m_config;
  void publishStatusUpdate(const msg_StatusUpdate &message, const std::string& strRoute);
};

#endif //NEWORDERCALLBACKHANDLER_H
