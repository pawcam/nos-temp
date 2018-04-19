#ifndef NEWORDERCALLBACKHANDLER_H
#define NEWORDERCALLBACKHANDLER_H

#include <twLib/or/NoopOR2CallbackHandler.h>
#include <OR2Lib/ORConfigReader.h>

namespace TW {
  class MQAdapter;
}

class NewOrderCallbackHandler : public TW::NoopOR2CallbackHandler {

public:
  NewOrderCallbackHandler(TW::MQAdapter *pMQAdapter, ORConfigReader::Config& config);

  virtual void statusUpdate(const TOptionID &optID, const string &strRoute,
                            const msg_StatusUpdate &stat) override;

  virtual void statusUpdate(const char *szStock, const string &strRoute, const msg_StatusUpdate &stat) override;

  virtual void statusUpdate(const msg_ComplexOrderWrapper &wrap, const string &strRoute,
                            const msg_StatusUpdate &stat) override;

  virtual void statusUpdate(const TFutureID &futID, const string &strRoute,
                            const msg_StatusUpdate &stat) override;

  virtual void statusUpdate(const TFutureOptionID &futOptID, const string &strRoute,
                            const msg_StatusUpdate &stat) override;


private:
  TW::MQAdapter *m_pMQAdapter;
  ORConfigReader::Config m_config;
  void publishStatusUpdate(const msg_StatusUpdate &message, const string &strRoute);
  void handleAccountNumberNotFound(int32_t nAccount) const;
};

#endif //NEWORDERCALLBACKHANDLER_H
