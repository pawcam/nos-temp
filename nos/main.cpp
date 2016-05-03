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

service_t *service;

void exit_cleanly(int sig) {
    cout << "Received signal " << sig << "; interrupting process" << endl;
    service->stop();
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

    string instanceId = getenv("SERVICE_INSTANCE_ID");
    string mqHost = getenv("MQ_HOST");
    uint16_t mqPort = lexical_cast<uint16_t>(getenv("MQ_PORT"));
    string mqUsername = getenv("MQ_USERNAME");
    string mqPassword = getenv("MQ_PASSWORD");
    string mqVHost = getenv("MQ_VHOST");
    string mqExchangeName = getenv("MQ_EXCHANGE_NAME");
    uint16_t orPort = lexical_cast<uint16_t>(getenv("OR_PORT"));
    string strORDefaultRoute = getenv("OR_DEFAULT_ROUTE");
    string mqQueueName = getenv("MQ_QUEUE_NAME");

    OELAdapterType *pOelAdapter = new OELAdapterType(orPort, INPUT, strORDefaultRoute);
    service = new service_t(instanceId, mqHost, mqPort, mqUsername, mqPassword, mqVHost, mqQueueName, mqExchangeName,
                            pOelAdapter);
    service->start();
    service->join();
    delete service;
    service = nullptr;
    exit(0);
}
