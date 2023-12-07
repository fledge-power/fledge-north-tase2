#include "tase2_config.hpp"
#include "tase2_datapoint.hpp"
#include <libtase2/tase2_common.h>
#include <libtase2/tase2_endpoint.h>
#include <libtase2/tase2_model.h>
#include <libtase2/tase2_server.h>
#include <tase2.hpp>

#include <stdbool.h>
#include <string>
#include <vector>

static uint64_t
GetCurrentTimeInMs ()
{
    struct timeval now;

    gettimeofday (&now, nullptr);

    return ((uint64_t)now.tv_sec * 1000LL) + (now.tv_usec / 1000);
}

TASE2Server::TASE2Server () : m_started (false), m_config (new TASE2Config ())
{
    Logger::getLogger ()->setMinLevel ("debug");
}

TASE2Server::~TASE2Server ()
{
    stop ();
    if (m_monitoringThread)
    {
        m_monitoringThread->join ();
        delete m_monitoringThread;
    }

    removeAllOutstandingCommands ();

    delete m_config;
}

void
TASE2Server::setJsonConfig (const std::string& stackConfig,
                            const std::string& dataExchangeConfig,
                            const std::string& tlsConfig,
                            const std::string& modelConfig)
{
    m_model = Tase2_DataModel_create ();
    m_config->importModelConfig (modelConfig, m_model);
    m_config->importExchangeConfig (dataExchangeConfig, m_model);
    m_config->importProtocolConfig (stackConfig);
    m_config->importTlsConfig (tlsConfig);

    m_server = Tase2_Server_create (m_model, nullptr);

    for (const auto& blt : m_config->getBilateralTables ())
    {
        Tase2Utility::log_debug ("Adding Bilateral Table '%s' to the server",
                                 Tase2_BilateralTable_getID (blt));
        Tase2_Server_addBilateralTable (m_server, blt);
    }

    Tase2_Server_setOperateHandler (m_server, operateHandler, this);
    Tase2_Server_setSelectHandler (m_server, selectHandler, this);
    Tase2_Server_setSetTagHandler (m_server, setTagHandler, this);
}

void
TASE2Server::start ()
{
    m_started = true;
    m_monitoringThread
        = new std::thread (&TASE2Server::_monitoringThread, this);
}

void
TASE2Server::_monitoringThread ()
{
    Tase2Utility::log_debug ("Monitoring thread called");

    if (!m_server)
    {
        Tase2Utility::log_error ("No server, can't start");
        return;
    }
    Tase2_Server_setLocalIpAddress (m_server, m_config->ServerIp ().c_str ());

    Tase2_Server_setTcpPort (m_server, m_config->TcpPort ());

    Tase2_Server_start (m_server);

    while (m_started)
    {
        /* check timeouts for outstanding commands */
        m_outstandingCommandsLock.lock ();

        uint64_t currentTime = Hal_getTimeInMs ();

        for (auto it = m_outstandingCommands.begin ();
             it != m_outstandingCommands.end ();)
        {
            if ((*it)->hasTimedOut (currentTime))
            {
                Tase2Utility::log_warn (
                    "command %s:%s timeout", (*it)->Domain ().c_str (),
                    (*it)->Name ().c_str ()); // LCOV_EXCL_LINE

                delete *it;
                it = m_outstandingCommands.erase (it);
            }
            else
            {
                ++it;
            }
        }

        m_outstandingCommandsLock.unlock ();

        Thread_sleep (100);
    }
}

void
TASE2Server::addToOutstandingCommands (const std::string& domain,
                                       const std::string& name, bool isSelect)
{
    m_outstandingCommandsLock.lock ();

    TASE2OutstandingCommand* outstandingCommand = new TASE2OutstandingCommand (
        domain, name, m_config->CmdExecTimeout (), isSelect);

    m_outstandingCommands.push_back (outstandingCommand);

    m_outstandingCommandsLock.unlock ();
}

void
TASE2Server::removeAllOutstandingCommands ()
{
    m_outstandingCommandsLock.lock ();

    std::vector<TASE2OutstandingCommand*>::iterator it;

    for (auto oc : m_outstandingCommands)
    {
        TASE2OutstandingCommand* outstandingCommand = *it;
        delete oc;
    }

    m_outstandingCommands.clear ();

    m_outstandingCommandsLock.unlock ();
}

void
TASE2Server::stop ()
{
    m_started = false;
    if (m_model)
    {
        Tase2_DataModel_destroy (m_model);
    }
    if (m_server)
    {
        Tase2_Server_destroy (m_server);
    }
}

Tase2_HandlerResult
TASE2Server::selectHandler (void* parameter, Tase2_ControlPoint controlPoint)
{
    auto server = (TASE2Server*)parameter;

    Tase2Utility::log_debug ("Received operate command\n");

    Tase2_ControlPointType type = Tase2_ControlPoint_getType (controlPoint);

    std::string domain
        = Tase2_Domain_getName (Tase2_ControlPoint_getDomain (controlPoint));
    std::string name = Tase2_ControlPoint_getName (controlPoint);

    std::string scope = domain == "vcc" ? "vcc" : "domain";

    Tase2Utility::log_debug ("Received select for %s:%s\n", domain, name);

    switch (type)
    {
    case TASE2_CONTROL_TYPE_COMMAND: {
        server->forwardCommand (scope, domain, name, "Command",
                                GetCurrentTimeInMs (), nullptr, true);
        break;
    }
    case TASE2_CONTROL_TYPE_SETPOINT_DESCRETE: {
        server->forwardCommand (scope, domain, name, "SetPointDiscrete",
                                GetCurrentTimeInMs (), nullptr, true);
        break;
    }
    case TASE2_CONTROL_TYPE_SETPOINT_REAL: {
        server->forwardCommand (scope, domain, name, "SetPointReal",
                                GetCurrentTimeInMs (), nullptr, true);
        break;
    }
    }

    return TASE2_RESULT_SUCCESS;
}

Tase2_HandlerResult
TASE2Server::setTagHandler (void* parameter, Tase2_ControlPoint controlPoint,
                            Tase2_TagValue value, const char* reason)
{
    Tase2Utility::log_debug (
        "Set tag value %i for control %s\n", value,
        Tase2_DataPoint_getName ((Tase2_DataPoint)controlPoint));

    Tase2Utility::log_debug ("   reason given by client: %s\n", reason);

    return TASE2_RESULT_SUCCESS;
}

Tase2_HandlerResult
TASE2Server::operateHandler (void* parameter, Tase2_ControlPoint controlPoint,
                             Tase2_OperateValue value)
{
    auto server = (TASE2Server*)parameter;
    Tase2Utility::log_debug ("Received operate command\n");

    Tase2_ControlPointType type = Tase2_ControlPoint_getType (controlPoint);

    std::string domain
        = Tase2_Domain_getName (Tase2_ControlPoint_getDomain (controlPoint));
    std::string name = Tase2_ControlPoint_getName (controlPoint);

    std::string scope = domain == "vcc" ? "vcc" : "domain";

    switch (type)
    {
    case TASE2_CONTROL_TYPE_COMMAND: {
        server->forwardCommand (scope, domain, name, "Command",
                                GetCurrentTimeInMs (), &value, false);
        break;
    }
    case TASE2_CONTROL_TYPE_SETPOINT_DESCRETE: {
        server->forwardCommand (scope, domain, name, "SetPointDiscrete",
                                GetCurrentTimeInMs (), &value, false);
        break;
    }
    case TASE2_CONTROL_TYPE_SETPOINT_REAL: {
        server->forwardCommand (scope, domain, name, "SetPointReal",
                                GetCurrentTimeInMs (), &value, false);
        break;
    }
    }
    return TASE2_RESULT_SUCCESS;
}

enum CommandParameters
{
    TYPE,
    SCOPE,
    DOMAIN,
    NAME,
    VALUE,
    SELECT,
    TS
};

void
TASE2Server::forwardCommand (const std::string& scope,
                             const std::string& domain,
                             const std::string& name, const std::string& type,
                             uint64_t ts, Tase2_OperateValue* value,
                             bool select)
{

    std::shared_ptr<TASE2Datapoint> t2dp
        = m_config->getDatapointByReference (domain, name);

    if (!t2dp || !t2dp->inExchangedDefinitions ())
    {
        Tase2Utility::log_debug (
            "Skipping command: %s %s, reason: datapoints is not in "
            "Exchanged Definitions",
            domain.c_str (), name.c_str ());
        return;
    }

    int parameterCount = 7;
    std::string tsStr = std::to_string (ts);
    char* s_scope = (char*)scope.c_str ();
    char* s_type = (char*)type.c_str ();
    char* s_domain = (char*)domain.c_str ();
    char* s_name = (char*)name.c_str ();
    char* s_val = "";
    char* s_select = (char*)(select ? "1" : "0");
    char* s_ts = (char*)tsStr.c_str ();

    std::string val;

    char* parameters[parameterCount];
    char* names[parameterCount];

    names[TYPE] = (char*)"co_type";
    names[SCOPE] = (char*)"co_scope";
    names[DOMAIN] = (char*)"co_domain";
    names[NAME] = (char*)"co_name";
    names[VALUE] = (char*)"co_value";
    names[SELECT] = (char*)"co_se";
    names[TS] = (char*)"co_ts";

    Tase2Utility::log_debug ("%s", type.c_str ());
    if (!select)
    {
        if (type == "Command")
        {
            val = std::to_string (value->commandValue);
        }
        else if (type == "SetPointDiscrete")
        {
            val = std::to_string (value->discreteValue);
        }
        else if (type == "SetPointReal")
        {
            val = std::to_string (value->realValue);
        }
    }
    s_val = (char*)val.c_str ();

    parameters[TYPE] = s_type;
    parameters[SCOPE] = s_scope;
    parameters[DOMAIN] = s_domain;
    parameters[NAME] = s_name;
    parameters[VALUE] = s_val;
    parameters[SELECT] = s_select;
    parameters[TS] = s_ts;

    addToOutstandingCommands (domain, name, select);

    m_oper ((char*)"TASE2Command", parameterCount, names, parameters,
            DestinationBroadcast, NULL);
}

void
TASE2Server::handleActCon (const std::string& domain, const std::string& name)
{
    m_outstandingCommandsLock.lock ();

    for (std::vector<TASE2OutstandingCommand*>::iterator it
         = m_outstandingCommands.begin ();
         it != m_outstandingCommands.end (); it++)
    {
        TASE2OutstandingCommand* outstandingCommand = *it;

        if (outstandingCommand->Domain () == domain
            && outstandingCommand->Name () == name)
        {
            m_outstandingCommands.erase (it);

            Tase2Utility::log_debug (
                "Outstanding command %i:%i confirmation  "
                "-> remove",
                outstandingCommand->Domain (),
                outstandingCommand->Name ()); // LCOV_EXCL_LINE

            delete outstandingCommand;

            break; // LCOV_EXCL_LINE
        }
    }

    m_outstandingCommandsLock.unlock ();
}

uint32_t
TASE2Server::send (const std::vector<Reading*>& readings)
{
    int n = 0;

    int readingsSent = 0;

    for (const auto& reading : readings)
    {
        std::vector<Datapoint*> const& dataPoints = reading->getReadingData ();
        std::string assetName = reading->getAssetName ();

        for (Datapoint* dp : dataPoints)
        {
            if (dp->getName () != "data_object")
            {
                Tase2Utility::log_debug ("Skipping datapoint: %s, reason: "
                                         "name is not 'data_object'",
                                         dp->getName ().c_str ());
                continue;
            }

            Tase2Utility::log_debug ("Send dp -> %s",
                                     dp->toJSONProperty ().c_str ());
            readingsSent++;

            if (!Tase2_Server_isRunning (m_server))
            {
                Tase2Utility::log_debug (
                    "Skipping datapoint: %s, reason: server is not running",
                    dp->toJSONProperty ().c_str ());
                continue;
            }

            DatapointValue dpv = dp->getData ();

            std::vector<Datapoint*> const* sdp = dpv.getDpVec ();

            std::string domain = "";
            std::string name = "";
            int type = -1;
            Tase2_DataFlags dataFlags = 0;

            bool hasTs = false;
            uint64_t timestamp = 0;
            bool hasTsValidity = false;

            DatapointValue* value = nullptr;

            for (Datapoint* objDp : *sdp)
            {
                DatapointValue attrVal = objDp->getData ();

                if (objDp->getName () == "do_domain")
                {
                    domain = attrVal.toStringValue ();
                }
                else if (objDp->getName () == "do_name")
                {
                    name = attrVal.toStringValue ();
                }

                else if (objDp->getName () == "do_type")
                {
                    type = TASE2Datapoint::getDpTypeFromString (
                        attrVal.toStringValue ());
                }
                else if (objDp->getName () == "do_value")
                {
                    value = new DatapointValue (attrVal);
                }
                else if (objDp->getName () == "do_validity")
                {
                    std::string validity = objDp->getData ().toStringValue ();
                    if (validity == "valid")
                        dataFlags |= TASE2_DATA_FLAGS_VALIDITY_VALID;
                    else if (validity == "held")
                        dataFlags |= TASE2_DATA_FLAGS_VALIDITY_HELD;
                    else if (validity == "suspect")
                        dataFlags |= TASE2_DATA_FLAGS_VALIDITY_SUSPECT;
                    else if (validity == "invalid")
                        dataFlags |= TASE2_DATA_FLAGS_VALIDITY_NOTVALID;
                }
                else if (objDp->getName () == "do_cs")
                {
                    std::string currentSource
                        = objDp->getData ().toStringValue ();
                    if (currentSource == "telemetered")
                    {
                        dataFlags
                            |= TASE2_DATA_FLAGS_CURRENT_SOURCE_TELEMETERED;
                    }
                    else if (currentSource == "entered")
                    {

                        dataFlags |= TASE2_DATA_FLAGS_CURRENT_SOURCE_ENTERED;
                    }
                    else if (currentSource == "calculated")
                    {
                        dataFlags
                            |= TASE2_DATA_FLAGS_CURRENT_SOURCE_CALCULATED;
                    }
                    else if (currentSource == "estimated")
                    {
                        dataFlags |= TASE2_DATA_FLAGS_CURRENT_SOURCE_ESTIMATED;
                    }
                }
                else if (objDp->getName () == "do_quality_normal_value")
                {
                    std::string normalValue
                        = objDp->getData ().toStringValue ();

                    if (normalValue == "normal")
                    {
                        dataFlags |= TASE2_DATA_FLAGS_NORMAL_VALUE;
                    }
                }
                else if (objDp->getName () == "do_ts")
                {
                    timestamp = (uint64_t)attrVal.toInt ();
                    hasTs = true;
                }
                else if (objDp->getName () == "do_ts_validity")
                {
                    std::string tsValidity
                        = objDp->getData ().toStringValue ();
                    if (tsValidity == "invalid")
                    {
                    }
                }
            }

            handleActCon (domain, name);

            DPTYPE dpType;
            if (type == -1)
            {
                Tase2Utility::log_debug (
                    "Skipping datapoint: %s, reason: type is -1",
                    dp->toJSONProperty ().c_str ());
                continue;
            }
            dpType = static_cast<DPTYPE> (type);

            std::shared_ptr<TASE2Datapoint> t2dp
                = m_config->getDatapointByReference (domain, name);

            if (!t2dp)
            {
                Tase2Utility::log_debug (
                    "Skipping datapoint: %s, reason: t2dp is null",
                    dp->toJSONProperty ().c_str ());
                continue;
            }

            if (!t2dp->inExchangedDefinitions ())
            {
                Tase2Utility::log_debug (
                    "Skipping datapoint: %s, reason: datapoints is not in "
                    "Exchanged Definitions",
                    dp->toJSONProperty ().c_str ());
                continue;
            }

            if (t2dp->getType () != dpType)
            {
                Tase2Utility::log_debug (
                    "Skipping datapoint: %s, reason: t2dp type mismatch",
                    dp->toJSONProperty ().c_str ());
                continue;
            }
            switch (dpType)
            {
            case REAL: {
                Tase2Utility::log_debug ("Datapoint is REAL %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_FLOAT)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_FLOAT for REAL",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setReal (ip, (float)value->toDouble ());
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case REALQ: {
                Tase2Utility::log_debug ("Datapoint is REALQ %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_FLOAT)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_FLOAT for REALQ",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setRealQ (ip, (float)value->toDouble (),
                                                dataFlags);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case REALQTIME:
            case REALQTIMEEXT: {
                Tase2Utility::log_debug ("Datapoint is REALQTIME %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_FLOAT)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_FLOAT for REALQTIME",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setRealQTimeStamp (
                    ip, (float)value->toDouble (), dataFlags, timestamp);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATE: {
                Tase2Utility::log_debug ("Datapoint is STATE %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATE",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setState (
                    ip, static_cast<Tase2_DataState> (value->toInt ()));
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATEQ: {
                Tase2Utility::log_debug ("Datapoint is STATEQ %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATEQ",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setState (
                    ip, static_cast<Tase2_DataState> (value->toInt ()
                                                      | dataFlags));
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATEQTIME:
            case STATEQTIMEEXT: {
                Tase2Utility::log_debug ("Datapoint is STATEQTIME %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATEQTIME",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setStateTimeStamp (
                    ip,
                    static_cast<Tase2_DataState> (value->toInt () | dataFlags),
                    timestamp);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case DISCRETE: {
                Tase2Utility::log_debug ("Datapoint is DISCRETE %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for DISCRETE",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setDiscrete (ip, value->toInt ());
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case DISCRETEQ: {
                Tase2Utility::log_debug ("Datapoint is DISCRETEQ %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for DISCRETEQ",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setDiscreteQ (ip, value->toInt (),
                                                    dataFlags);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case DISCRETEQTIME:
            case DISCRETEQTIMEEXT: {
                Tase2Utility::log_debug ("Datapoint is DISCRETEQTIMEEXT %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for DISCRETEQTIMEEXT",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setDiscreteQTimeStamp (
                    ip, value->toInt (), dataFlags, timestamp);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATESUP: {
                Tase2Utility::log_debug ("Datapoint is STATESUP %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATESUP",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setStateSupplemental (
                    ip, static_cast<Tase2_DataStateSupplemental> (
                            value->toInt ()));
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATESUPQ: {
                Tase2Utility::log_debug ("Datapoint is STATESUPQ %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATESUPQ",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setStateSupplementalQ (
                    ip,
                    static_cast<Tase2_DataStateSupplemental> (value->toInt ()),
                    dataFlags);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case STATESUPQTIME:
            case STATESUPQTIMEEXT: {
                Tase2Utility::log_debug ("Datapoint is STATESUPQTIMEEXT %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATESUPQTIMEEXT",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setStateSupplementalQTimeStamp (
                    ip,
                    static_cast<Tase2_DataStateSupplemental> (value->toInt ()),
                    dataFlags, timestamp);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            }
            delete value;
        }
        n++;
    }

    return 0;
}

void
TASE2Server::configure (const ConfigCategory* config)
{
    Tase2Utility::log_info ("configure called"); // LCOV_EXCL_LINE

    if (config->itemExists ("name"))
        m_name = config->getValue ("name"); // LCOV_EXCL_LINE
    else
        Tase2Utility::log_error (
            "Missing name in configuration"); // LCOV_EXCL_LINE

    if (config->itemExists ("protocol_stack") == false)
    {
        Tase2Utility::log_error (
            "Missing protocol configuration"); // LCOV_EXCL_LINE
        return;
    }

    if (config->itemExists ("exchanged_data") == false)
    {
        Tase2Utility::log_error (
            "Missing exchange data configuration"); // LCOV_EXCL_LINE
        return;
    }

    if (config->itemExists ("model_conf") == false)
    {
        Tase2Utility::log_error ("Missing model configuration");
        return;
    }

    std::string protocolStack = config->getValue ("protocol_stack");

    std::string dataExchange = config->getValue ("exchanged_data");

    std::string modelConf = config->getValue ("model_conf");

    if (protocolStack.empty ())
    {
        protocolStack = config->getDefault ("protocol_stack");
    }
    if (dataExchange.empty ())
    {
        dataExchange = config->getDefault ("exchanged_data");
    }
    if (modelConf.empty ())
    {
        modelConf = config->getDefault ("model_conf");
    }

    std::string tlsConfig = "";

    if (config->itemExists ("tls_conf") == false)
    {
        Tase2Utility::log_error (
            "Missing TLS configuration"); // LCOV_EXCL_LINE
    }
    else
    {
        tlsConfig = config->getValue ("tls_conf");
    }

    setJsonConfig (protocolStack, dataExchange, tlsConfig, modelConf);
}

void
TASE2Server::registerControl (
    int (*operation) (char* operation, int paramCount, char* names[],
                      char* parameters[], ControlDestination destination, ...))
{
    m_oper = operation;

    Tase2Utility::log_warn ("RegisterControl is called"); // LCOV_EXCL_LINE
}
