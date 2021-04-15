#include <boost/lexical_cast.hpp>
#include <iostream>
#include <kr/sx_log.h>
#include <kr/CmdLine.h>
#include <OR2Lib/ORConfigReader.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/models/Future.h>
#include <twLib/models/FutureOption.h>
#include <twLib/SenderLocationReader.h>

#include "NewOrderCallbackHandler.h"
#include "NewOrderMessageHandler.h"

int main(int argc, char *argv[]) {
  sx_setArgCArgV(argc, argv);

#ifdef _DEBUG
  std::cout << "Enabling debug logging" << std::endl;
  sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif

  const string strMqHost = getenv("MQ_HOST");
  const uint16_t nMqPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
  const string strMqUsername = getenv("MQ_USERNAME");
  const string strMqPassword = getenv("MQ_PASSWORD");
  const string strMqVHost = getenv("MQ_VHOST");
  const string strMqDirectExchangeName = getenv("MQ_DIRECT_EXCHANGE_NAME");
  const string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
  const string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
  const string strMqQueueName = getenv("MQ_QUEUE_NAME");

  CCmdLine cmdLine;
  cmdLine.SplitLine(argc, argv);
  bool bDirectExchange = cmdLine.HasSwitch("--direct_exchange");

  std::vector <std::string> vFutureSymbolMappings = {
    "cme_db_future.out",
    "smalls_db_future.out",
    "cfe_db_future.out"
  };

  // load future options
  TW::Future::loadMultipleFutureSymbolMappings(vFutureSymbolMappings);
  TW::FutureOption::loadCmeSymbolMappingsCSV("cme_db_option.out");
  TW::FutureOption::loadSmallsSymbolMappingsCSV("smalls_db_option.out");

  ORConfigReader::Config config;
  ORConfigReader::read(std::string("Config.xml"), std::string(""), config);

  TW::SenderLocationReader locationReader;

  locationReader.readFile();

  sx_ThreadSafeLockUnlock lock;
  TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute, "OR2Adapter", 100, false, &lock);

  string strBindingKey = bDirectExchange ? strMqQueueName : "";
  TW::MQAdapter mqAdapter(strMqHost, nMqPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName, "MQAdapter", strBindingKey, bDirectExchange, strMqDirectExchangeName);

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
