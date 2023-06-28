#include <boost/lexical_cast.hpp>
#include <iostream>
#include <kr/sx_log.h>
#include <kr/CmdLine.h>
#include <OR2Lib/ORConfigReader.h>
#include <twLib/util.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/models/Future.h>
#include <twLib/models/FutureOption.h>
#include <twLib/SenderLocationReader.h>

#include "NewOrderCallbackHandler.h"
#include "NewOrderMessageHandler.h"

#include <kr/static_assertions.h>

int main(int argc, char *argv[]) {
  sx_setArgCArgV(argc, argv);

#ifdef _DEBUG
  std::cout << "Enabling debug logging" << std::endl;
  sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif

  // Required Environment Variables
  const std::string strMqHost = getenv("MQ_HOST");
  const uint16_t nMqPort = static_cast<uint16_t>(strtol(getenv("MQ_PORT"), nullptr/*endptr*/, 10/*base*/));
  const std::string strMqUsername = getenv("MQ_USERNAME");
  const std::string strMqPassword = getenv("MQ_PASSWORD");
  const std::string strMqVHost = getenv("MQ_VHOST");
  const std::string strMqDirectExchangeName = getenv("MQ_DIRECT_EXCHANGE_NAME");
  const std::string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
  const std::string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
  const std::string strMqQueueName = getenv("MQ_QUEUE_NAME");
  // Optional Environment Variables
  bool bMqSslEnabled = (TW::getEnv("MQ_SSL_ENABLED", "false") == "true" ? true : false);
  const string strMqSslCaCertPath = TW::getEnv("MQ_SSL_CA_CERT_PATH", "");
  const string strMqSslClientCertPath = TW::getEnv("MQ_SSL_CLIENT_CERT_PATH", "");
  const string strMqSslClientKeyPath = TW::getEnv("MQ_SSL_CLIENT_KEY_PATH", "");
  bool bMqSslVerifyHostname = (TW::getEnv("MQ_SSL_VERIFY_HOSTNAME", "false") == "true" ? true : false);

  CCmdLine cmdLine;
  cmdLine.SplitLine(argc, argv);
  bool bDirectExchange = cmdLine.HasSwitch("--direct_exchange");
  const std::string strSenderLocationFile = cmdLine.GetSafeArgument("--senderLocationFile", 0, "SenderLocation.xml");

  std::vector <std::string> vFutureSymbolMappings = {
    "cme_db_future.out",
    "smalls_db_future.out",
    "cfe_db_future.out"
  };
  TW::Future::loadMultipleFutureSymbolMappings(vFutureSymbolMappings);

  std::vector <std::string> vFutureOptionSymbolMappings = {
    "cme_db_option.out",
    "smalls_db_option.out"
  };
  TW::FutureOption::loadMultipleFutureOptionSymbolMappings(vFutureOptionSymbolMappings);

  ORConfigReader::Config config;
  ORConfigReader::read(std::string("Config.xml"), std::string(""), config);

  TW::SenderLocationReader locationReader;

  locationReader.readFile(strSenderLocationFile);

  sx_ThreadSafeLockUnlock lock;
  TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute, "NOS", 100, false, &lock);

  const std::string strBindingKey = bDirectExchange ? strMqQueueName : "";
  TW::MQAdapter mqAdapter(strMqHost, nMqPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName, "MQAdapter", strBindingKey, bDirectExchange, strMqDirectExchangeName,
                            bMqSslEnabled, strMqSslCaCertPath, strMqSslClientKeyPath, strMqSslClientCertPath, bMqSslVerifyHostname);

  NewOrderCallbackHandler callbackHandler(&mqAdapter, config);
  NewOrderMessageHandler messageHandler = NewOrderMessageHandler(&or2Adapter, &mqAdapter, config, &locationReader, bDirectExchange);

  or2Adapter.setService(&callbackHandler);
  mqAdapter.setMessageHandler(&messageHandler);

  or2Adapter.start();
  lock.Lock(__FILE__, __LINE__);
  lock.Unlock();
  mqAdapter.start();

  or2Adapter.join();
  mqAdapter.join();
  return 0;
}
