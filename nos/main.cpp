#include <boost/lexical_cast.hpp>
#include <iostream>
#include <kr/sx_log.h>
#include <kr/CmdLine.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>

#include "NewOrderCallbackHandler.h"
#include "NewOrderMessageHandler.h"

int main(int argc, char *argv[]) {
  sx_setArgCArgV(argc, argv);

  CCmdLine cmdLine;
  cmdLine.SplitLine(argc, argv);

#ifdef _DEBUG
  std::cout << "Enabling debug logging" << std::endl;
  sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif
    const string strMqHost = getenv("MQ_HOST");
    const uint16_t nMqPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    const string strMqUsername = getenv("MQ_USERNAME");
    const string strMqPassword = getenv("MQ_PASSWORD");
    const string strMqVHost = getenv("MQ_VHOST");
    const string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    const string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    const string strMqQueueName = getenv("MQ_QUEUE_NAME");

  TW::MQAdapter mqAdapter(strMqHost, nMqPort, strMqUsername,
                         strMqPassword, strMqVHost, strMqQueueName,
                         strMqExchangeName);

  TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute);

  NewOrderCallbackHandler callbackHandler(&mqAdapter);

  NewOrderMessageHandler messageHandler = NewOrderMessageHandler(&or2Adapter, &mqAdapter);

  mqAdapter.setMessageHandler(&messageHandler);
  or2Adapter.setService(&callbackHandler);

  mqAdapter.start();
  or2Adapter.start();

  mqAdapter.join();
  or2Adapter.join();
  return 0;
}
