#ifndef _TASE2SERVER_H
#define _TASE2SERVER_H

#include <config_category.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <libtase2/tase2_common.h>
#include <logger.h>
#include <memory>
#include <plugin_api.h>
#include <reading.h>

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "libtase2/hal_thread.h"
#include "libtase2/hal_time.h"
#include "libtase2/tase2_server.h"
#include "tase2_config.hpp"
#include "tase2_datapoint.hpp"
#include "tase2_utility.hpp"

class TASE2OutstandingCommand
{
  public:
    TASE2OutstandingCommand (const std::string& domain,
                             const std::string& name, int cmdExecTimeout,
                             bool isSelect);
    ~TASE2OutstandingCommand () = default;

    bool hasTimedOut (uint64_t currentTime);

    std::string
    Domain ()
    {
        return m_domain;
    };
    std::string
    Name ()
    {
        return m_name;
    };

    bool
    isSelect ()
    {
        return m_select;
    };

  private:
    std::string m_domain;
    std::string m_name;

    bool m_select;

    int m_cmdExecTimeout;

    uint64_t m_commandRcvdTime = 0;
    uint64_t m_nextTimeout = 0;

    int m_state = 0; /* 0 - idle/complete, 1 - waiting for ACT-CON, 2 - waiting
                        for ACT-TERM */
};

class TASE2Server
{
  public:
    TASE2Server ();
    ~TASE2Server ();

    void setJsonConfig (const std::string& stackConfig,
                        const std::string& dataExchangeConfig,
                        const std::string& tlsConfig,
                        const std::string& modelConfig);

    void start ();

    void
    setModelPath (const std::string& path)
    {
        m_modelPath = path;
    };
    void configure (const ConfigCategory* conf);
    void handleActCon (const std::string& domain, const std::string& name);
    uint32_t send (const std::vector<Reading*>& readings);
    void stop ();
    void registerControl (int (*operation) (char* operation, int paramCount,
                                            char* names[], char* parameters[],
                                            ControlDestination destination,
                                            ...));

    void updateDatapointInServer (std::shared_ptr<TASE2Datapoint>,
                                  bool timeSynced);
    const std::string getObjRefFromID (const std::string& id);
    TASE2Config*
    getConfig ()
    {
        return m_config;
    };
    Datapoint* buildOperation (DPTYPE type, Tase2_OperateValue ctlVal,
                               bool test, bool isSelect,
                               const std::string& label, uint64_t timestamp,
                               bool hasSelect);

  private:
    std::vector<std::pair<TASE2Server*, TASE2Datapoint*>*>* sdpObjects
        = nullptr;

    std::vector<TASE2OutstandingCommand*> m_outstandingCommands;
    std::mutex m_outstandingCommandsLock;

    Semaphore outputQueueLock = nullptr;
    LinkedList outputQueue = nullptr;

    Tase2_Server m_server = nullptr;
    Tase2_DataModel m_model = nullptr;

    TLSConfiguration m_tlsConfig = nullptr;

    std::string m_modelPath;

    bool m_started;
    std::string m_name;
    TASE2Config* m_config = nullptr;

    bool m_passive = false;

    Tase2_Endpoint m_endpoint = nullptr;

    std::thread* m_monitoringThread = nullptr;

    std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> >
        m_modelEntries;

    int (*m_oper) (char* operation, int paramCount, char* names[],
                   char* parameters[], ControlDestination destination, ...)
        = NULL;

    bool createTLSConfiguration ();
    void _monitoringThread ();

    void addToOutstandingCommands (const std::string& domain,
                                   const std::string& name, bool isSelect);

    void removeAllOutstandingCommands ();

    static Tase2_HandlerResult selectHandler (void* parameter,
                                              Tase2_ControlPoint controlPoint);

    static Tase2_HandlerResult setTagHandler (void* parameter,
                                              Tase2_ControlPoint controlPoint,
                                              Tase2_TagValue value,
                                              const char* reason);

    void forwardCommand (const std::string& scope, const std::string& domain,
                         const std::string& name, const std::string& type,
                         uint64_t ts, Tase2_OperateValue* value, bool select);
    static Tase2_HandlerResult operateHandler (void* parameter,
                                               Tase2_ControlPoint controlPoint,
                                               Tase2_OperateValue value);

    FRIEND_TEST (ConnectionHandlerTest, NormalConnection);
    FRIEND_TEST (ControlTest, OutstandingCommandSuccess);
    FRIEND_TEST (ControlTest, OutstandingCommandFailure);
    FRIEND_TEST (DatasetTest, CreateDatasetAndUpdate);
    FRIEND_TEST (ConnectionHandlerTest, NormalConnectionActive);
    friend class DatasetTest;
};

class ServerDatapointPair
{
  public:
    TASE2Server* server;
    TASE2Datapoint* dp;
};

#endif
