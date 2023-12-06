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

struct sOutputData
{
    char* targetObjRef;
    char* targetValue;
};

typedef struct sOutputData* OutputData;

class TASE2ServerException : public std::exception // NOSONAR
{
  public:
    explicit TASE2ServerException (const std::string& context)
        : m_context (context)
    {
    }

    const std::string&
    getContext (void)
    {
        return m_context;
    };

  private:
    const std::string m_context;
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

    Semaphore outputQueueLock = nullptr;
    LinkedList outputQueue = nullptr;

    Tase2_Server m_server = nullptr;
    Tase2_DataModel m_model = nullptr;

    std::string m_modelPath;

    bool m_started;
    std::string m_name;
    TASE2Config* m_config = nullptr;

    std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> >
        m_modelEntries;

    int (*m_oper) (char* operation, int paramCount, char* names[],
                   char* parameters[], ControlDestination destination, ...)
        = NULL;

    bool createTLSConfiguration ();

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
    FRIEND_TEST (ControlTest, NormalConnection);
    FRIEND_TEST (ControlTest, SendSpcCommand);
};

class ServerDatapointPair
{
  public:
    TASE2Server* server;
    TASE2Datapoint* dp;
};

#endif
