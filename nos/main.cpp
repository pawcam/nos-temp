#include <boost/lexical_cast.hpp>
#include <iostream>
#include <kr/sx_log.h>
#include <kr/CmdLine.h>
#include <OR2Lib/ORConfigReader.h>
#include <twLib/mq/MQAdapter.h>
#include <twLib/or/OR2Adapter.h>
#include <twLib/models/FutureOption.h>

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


    TW::FutureOption::loadCmeSymbolMappingCSV();

    ORConfigReader::Config config;
    ORConfigReader::read(std::string("Config.xml"), std::string(""), config);

    sx_ThreadSafeLockUnlock lock;
    TW::OR2Adapter or2Adapter(TW::OR2ClientMode::INPUT, strORDefaultRoute, "OR2Adapter", 100, false, &lock);

    TW::MQAdapter mqAdapter(strMqHost, nMqPort, strMqUsername,
                            strMqPassword, strMqVHost, strMqQueueName,
                            strMqExchangeName, "MQAdapter");
    NewOrderCallbackHandler callbackHandler(&mqAdapter);
    NewOrderMessageHandler messageHandler = NewOrderMessageHandler(&or2Adapter, &mqAdapter, config);

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
