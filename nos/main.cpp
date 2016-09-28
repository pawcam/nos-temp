#include <iostream>

#include <signal.h>

#include <boost/lexical_cast.hpp>
#include <json/json.hpp>
#include <kr/sx_pl.h>
#include <kr/CmdLine.h>
#include <twMagic/service/MessageRelayService.h>
#include <twMagic/neworders/NewOrderService.h>

using namespace std;
using namespace boost;
using namespace TW;

typedef MessageRelayService<NewOrderService> service_t;
typedef service_t::OELAdapterType OELAdapterType;

service_t *pService;

void exit_cleanly(int nSig) {
    cout << "Received signal " << nSig << "; interrupting process" << endl;
    pService->stop();
}

int main(int argc, char *argv[]) {
    signal(SIGINT, exit_cleanly);

    sx_setArgCArgV(argc, argv);

    CCmdLine cmdLine;
    cmdLine.SplitLine(argc, argv);

#ifdef _DEBUG
    cout << "Enabling debug logging" << endl;
    sx_log::Instance().setBit(sx_log::SX_LOG_DEBUG, true);
#endif

    string strInstanceId = getenv("SERVICE_INSTANCE_ID");
    string strMqHost = getenv("MQ_HOST");
    uint16_t nMqPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    string strMqUsername = getenv("MQ_USERNAME");
    string strMqPassword = getenv("MQ_PASSWORD");
    string strMqVHost = getenv("MQ_VHOST");
    string strMqExchangeName = getenv("MQ_EXCHANGE_NAME");
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string strMqQueueName = getenv("MQ_QUEUE_NAME");

    OELAdapterType *pOelAdapter = new OELAdapterType(INPUT, strORDefaultRoute);
    pService = new service_t(strInstanceId, strMqHost, nMqPort, strMqUsername, strMqPassword, strMqVHost, strMqQueueName, strMqExchangeName,
                            pOelAdapter);
    pService->start();
    pService->join();
    delete pService;
    pService = nullptr;
    exit(0);
}
