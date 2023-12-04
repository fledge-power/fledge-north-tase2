#include "tase2_config.hpp"
#include "tase2_datapoint.hpp"
#include <libtase2/tase2_common.h>
#include <libtase2/tase2_model.h>
#include <libtase2/tase2_server.h>
#include <tase2.hpp>

#include <stdbool.h>
#include <string>
#include <vector>

TASE2Server::TASE2Server () : m_started (false), m_config (new TASE2Config ())
{
    Logger::getLogger ()->setMinLevel ("debug");
}

TASE2Server::~TASE2Server ()
{
    stop ();

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
}

void
TASE2Server::start ()
{
    if (!m_server)
    {
        Tase2Utility::log_error ("No server, can't start");
        return;
    }
    Tase2_Server_setLocalIpAddress (m_server, m_config->ServerIp ().c_str ());

    Tase2_Server_setTcpPort (m_server, m_config->TcpPort ());

    Tase2_Server_start (m_server);
}

void
TASE2Server::stop ()
{
    if (m_model)
    {
        Tase2_DataModel_destroy (m_model);
    }
    if (m_server)
    {
        Tase2_Server_destroy (m_server);
    }
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
                else if (objDp->getName () == "dp_ts_validity")
                {
                    std::string tsValidity
                        = objDp->getData ().toStringValue ();
                    if (tsValidity == "invalid")
                    {
                    }
                }
            }

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
                Tase2_IndicationPoint_setReal (ip, value->toDouble ());
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
                Tase2_IndicationPoint_setRealQ (ip, value->toDouble (),
                                                dataFlags);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
            case REALQTIME: {
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
                    ip, value->toDouble (), dataFlags, timestamp / 1000);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
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
                    ip, value->toDouble (), dataFlags, timestamp);
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
            case STATEQTIME: {
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
                    timestamp / 1000);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
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
            case DISCRETEQTIME: {
                Tase2Utility::log_debug ("Datapoint is DISCRETEQTIME %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for DISCRETEQTIME",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setDiscreteQTimeStamp (
                    ip, value->toInt (), dataFlags, timestamp / 1000);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
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
            case STATESUPQTIME: {
                Tase2Utility::log_debug ("Datapoint is STATESUPQTIME %s",
                                         dp->toJSONProperty ().c_str ());
                if (value->getType () != DatapointValue::T_INTEGER)
                {
                    Tase2Utility::log_debug (
                        "Skipping datapoint: %s, reason: value type is not "
                        "T_INTEGER for STATESUPQTIME",
                        dp->toJSONProperty ().c_str ());
                    continue;
                }
                Tase2_IndicationPoint ip = t2dp->getIndicationPoint ();
                Tase2_IndicationPoint_setStateSupplementalQTimeStamp (
                    ip,
                    static_cast<Tase2_DataStateSupplemental> (value->toInt ()),
                    dataFlags, timestamp / 1000);
                Tase2_Server_updateOnlineValue (m_server, (Tase2_DataPoint)ip);
                break;
            }
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