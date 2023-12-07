#include <arpa/inet.h>

#include <memory>
#include <string>

#include "tase2.hpp"
#include "tase2_config.hpp"
#include "tase2_datapoint.hpp"

using namespace rapidjson;

#define JSON_EXCHANGED_DATA "exchanged_data"
#define JSON_DATAPOINTS "datapoints"
#define JSON_PROTOCOLS "protocols"
#define JSON_LABEL "label"

#define PROTOCOL_TASE2 "tase2"
#define JSON_PROT_NAME "name"
#define JSON_PROT_REF "ref"
#define JSON_PROT_CDC "cdc"

TASE2Config::TASE2Config () = default;

TASE2Config::~TASE2Config () = default;

bool
TASE2Config::isValidIPAddress (const std::string& addrStr)
{
    // see
    // https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
    struct sockaddr_in sa;
    int result = inet_pton (AF_INET, addrStr.c_str (), &(sa.sin_addr));

    return (result == 1);
}

void
TASE2Config::importModelConfig (const std::string& modelConfig,
                                Tase2_DataModel model)
{
    Document document;
    if (document.Parse (const_cast<char*> (modelConfig.c_str ()))
            .HasParseError ())
    {
        Tase2Utility::log_fatal ("Parsing error in model configuration");
        Tase2Utility::log_debug ("Parsing error in model configuration\n");
        return;
    }

    if (!document.IsObject ())
    {
        Tase2Utility::log_error ("Model configuration is not an object");
        return;
    }

    if (!document.HasMember ("model_conf")
        || !document["model_conf"].IsObject ())
    {
        Tase2Utility::log_error (
            "Missing or invalid 'model_conf' object in model configuration");
        return;
    }

    const Value& modelConf = document["model_conf"];

    if (!modelConf.HasMember ("vcc") || !modelConf["vcc"].IsObject ())
    {
        Tase2Utility::log_error (
            "Missing or invalid 'vcc' object in 'model_conf'");
        return;
    }

    const Value& vccValue = modelConf["vcc"];

    if (!vccValue.HasMember ("datapoints")
        || !vccValue["datapoints"].IsArray ())
    {
        Tase2Utility::log_error (
            "Missing or invalid 'datapoints' array in 'vcc'");
        return;
    }

    const Value& vccDatapoints = vccValue["datapoints"];
    Tase2_Domain vcc = Tase2_DataModel_getVCC (model);

    for (const Value& datapoint : vccDatapoints.GetArray ())
    {
        if (!datapoint.IsObject ())
        {
            Tase2Utility::log_error ("DATAPOINT NOT AN OBJECT");
            return;
        }
        if (!datapoint.HasMember ("name") || !datapoint["name"].IsString ())
        {
            Tase2Utility::log_error ("DATAPOINT HAS NO NAME");
            return;
        }
        if (!datapoint.HasMember ("type") || !datapoint["type"].IsString ())
        {
            Tase2Utility::log_error ("DATAPOINT HAS NO TYPE");
            return;
        }

        DPTYPE type = TASE2Datapoint::getDpTypeFromString (
            datapoint["type"].GetString ());

        if (type == DP_TYPE_UNKNOWN)
        {
            Tase2Utility::log_error ("Invalid dp type %s",
                                     datapoint["type"].GetString ());
            continue;
        }

        auto t2dp = std::make_shared<TASE2Datapoint> (
            datapoint["name"].GetString (),
            TASE2Datapoint::getDpTypeFromString (
                datapoint["type"].GetString ()));

        m_modelEntries["vcc"].insert ({ t2dp->getLabel (), t2dp });

        if (TASE2Datapoint::isCommand (t2dp->getType ()))
        {
            if (!datapoint.HasMember ("mode")
                || !datapoint["mode"].IsString ())
            {
                Tase2Utility::log_error (
                    "CONTROL POINT HAS NO mode attribute ");
                continue;
            }
            if (!datapoint.HasMember ("hasTag")
                || !datapoint["hasTag"].IsBool ())
            {
                Tase2Utility::log_error (
                    "CONTROL POINT HAS NO hasTag attribute ");
                continue;
            }

            if (!datapoint.HasMember ("checkBackId")
                || !datapoint["checkBackId"].IsInt ())
            {
                Tase2Utility::log_error (
                    "CONTROL POINT HAS NO checkBackId attribute ");
                continue;
            }

            auto deviceClass = static_cast<Tase2_DeviceClass> (
                datapoint["mode"].GetString () == "sbo");

            bool hasTag = datapoint["hasTag"].GetBool ();

            auto checkBackId = (int16_t)datapoint["checkBackId"].GetInt ();

            Tase2_ControlPointType contType
                = TASE2Datapoint::toControlPointType (t2dp->getType ());

            t2dp->setControlPoint (Tase2_Domain_addControlPoint (
                vcc, t2dp->getLabel ().c_str (), contType, deviceClass, hasTag,
                checkBackId));
        }
        else
        {
            if (!datapoint.HasMember ("hasCOV")
                || !datapoint["hasCOV"].IsBool ())
            {
                Tase2Utility::log_error (
                    "INDICATION POINT HAS NO hasCOV attribute ");
                return;
            }

            bool hasCOV = datapoint["hasCOV"].GetBool ();
            Tase2_QualityClass qClass
                = TASE2Datapoint::getQualityClass (t2dp->getType ());
            Tase2_TimeStampClass tsClass
                = TASE2Datapoint::getTimeStampClass (t2dp->getType ());
            Tase2_IndicationPointType indType
                = TASE2Datapoint::toIndicationPointType (t2dp->getType ());
            t2dp->setIndicationPoint (Tase2_Domain_addIndicationPoint (
                vcc, t2dp->getLabel ().c_str (), indType, qClass, tsClass,
                hasCOV, true));
        }
    }

    if (!modelConf.HasMember ("icc") || !modelConf["icc"].IsArray ())
    {
        Tase2Utility::log_error (
            "Missing or invalid 'icc' array in 'model_conf'");
        return;
    }

    const Value& iccArray = modelConf["icc"];

    for (const Value& iccValue : iccArray.GetArray ())
    {
        if (!iccValue.HasMember ("name") || !iccValue["name"].IsString ())
        {
            Tase2Utility::log_error ("ICC MISSING NAME");
            return;
        }

        Tase2_Domain icc
            = Tase2_DataModel_addDomain (model, iccValue["name"].GetString ());

        m_domains[iccValue["name"].GetString ()] = icc;

        if (!iccValue.HasMember ("datapoints")
            || !iccValue["datapoints"].IsArray ())
        {
            Tase2Utility::log_error ("ICC MISSING DATAPOINTS");
            return;
        }

        const Value& iccDatapoints = iccValue["datapoints"];

        for (const Value& datapoint : iccDatapoints.GetArray ())
        {
            if (!datapoint.IsObject ())
            {
                Tase2Utility::log_error ("DATAPOINT NOT AN OBJECT");
                return;
            }
            if (!datapoint.HasMember ("name")
                || !datapoint["name"].IsString ())
            {
                Tase2Utility::log_error ("DATAPOINT HAS NO NAME");
                return;
            }
            if (!datapoint.HasMember ("type")
                || !datapoint["type"].IsString ())
            {
                Tase2Utility::log_error ("DATAPOINT HAS NO TYPE");
                return;
            }

            DPTYPE type = TASE2Datapoint::getDpTypeFromString (
                datapoint["type"].GetString ());

            if (type == DP_TYPE_UNKNOWN)
            {
                Tase2Utility::log_error ("Invalid dp type %s",
                                         datapoint["type"].GetString ());
                continue;
            }

            auto t2dp = std::make_shared<TASE2Datapoint> (
                datapoint["name"].GetString (),
                TASE2Datapoint::getDpTypeFromString (
                    datapoint["type"].GetString ()));

            if (TASE2Datapoint::isCommand (t2dp->getType ()))
            {
                if (!datapoint.HasMember ("mode")
                    || !datapoint["mode"].IsString ())
                {
                    Tase2Utility::log_error (
                        "CONTROL POINT HAS NO mode attribute ");
                    continue;
                }
                if (!datapoint.HasMember ("hasTag")
                    || !datapoint["hasTag"].IsBool ())
                {
                    Tase2Utility::log_error (
                        "CONTROL POINT HAS NO hasTag attribute ");
                    continue;
                }

                if (!datapoint.HasMember ("checkBackId")
                    || !datapoint["checkBackId"].IsInt ())
                {
                    Tase2Utility::log_error (
                        "CONTROL POINT HAS NO checkBackId attribute ");
                    continue;
                }

                auto deviceClass = static_cast<Tase2_DeviceClass> (
                    datapoint["mode"].GetString () != "sbo");

                bool hasTag = datapoint["hasTag"].GetBool ();

                auto checkBackId = (int16_t)datapoint["checkBackId"].GetInt ();

                Tase2_ControlPointType contType
                    = TASE2Datapoint::toControlPointType (t2dp->getType ());

                t2dp->setControlPoint (Tase2_Domain_addControlPoint (
                    icc, t2dp->getLabel ().c_str (), contType, deviceClass,
                    hasTag, checkBackId));

                t2dp->setCheckBackId (checkBackId);
            }
            else
            {
                if (!datapoint.HasMember ("hasCOV")
                    || !datapoint["hasCOV"].IsBool ())
                {
                    Tase2Utility::log_error (
                        "INDICATION POINT HAS NO hasCOV attribute ");
                    return;
                }

                bool hasCOV = datapoint["hasCOV"].GetBool ();
                Tase2_QualityClass qClass
                    = TASE2Datapoint::getQualityClass (t2dp->getType ());
                Tase2_TimeStampClass tsClass
                    = TASE2Datapoint::getTimeStampClass (t2dp->getType ());
                Tase2_IndicationPointType indType
                    = TASE2Datapoint::toIndicationPointType (t2dp->getType ());
                t2dp->setIndicationPoint (Tase2_Domain_addIndicationPoint (
                    icc, t2dp->getLabel ().c_str (), indType, qClass, tsClass,
                    hasCOV, true));
            }

            m_modelEntries[iccValue["name"].GetString ()][t2dp->getLabel ()]
                = t2dp;

            Tase2Utility::log_debug (
                "Add datapoint %s to domain %s, %d datapoints present",
                t2dp->getLabel ().c_str (), iccValue["name"].GetString (),
                m_modelEntries.size ());
        }
    }

    if (!modelConf.HasMember ("bilateral_tables")
        || !modelConf["bilateral_tables"].IsArray ())
    {
        Tase2Utility::log_error (
            "Missing or invalid 'bilateral_tables' array in 'model_conf'");
        return;
    }

    const Value& bltArray = modelConf["bilateral_tables"];

    for (const Value& bltValue : bltArray.GetArray ())
    {
        if (!bltValue.HasMember ("name") || !bltValue["name"].IsString ())
        {
            Tase2Utility::log_error ("BILATERAL TABLE MISSING NAME");
            return;
        }

        if (!bltValue.HasMember ("icc") || !bltValue["icc"].IsString ())
        {
            Tase2Utility::log_error ("BILATERAL TABLE MISSING 'icc'");
            return;
        }

        if (!bltValue.HasMember ("apTitle")
            || !bltValue["apTitle"].IsString ())
        {
            Tase2Utility::log_error ("BILATERAL TABLE MISSING 'apTitle'");
            return;
        }

        if (!bltValue.HasMember ("aeQualifier")
            || !bltValue["aeQualifier"].IsInt ())
        {
            Tase2Utility::log_error ("BILATERAL TABLE MISSING 'aeQualifier'");
            return;
        }

        if (!bltValue.HasMember ("datapoints")
            || !bltValue["datapoints"].IsArray ())
        {
            Tase2Utility::log_error ("BILATERAL TABLE MISSING 'datapoints'");
            return;
        }

        auto it = m_domains.find (bltValue["icc"].GetString ());

        if (it == m_domains.end ())
        {
            Tase2Utility::log_error ("ICC not found for bilateral table");
            return;
        }

        Tase2_Domain icc = it->second;

        Tase2_BilateralTable blt
            = Tase2_BilateralTable_create (bltValue["name"].GetString (), icc,
                                           bltValue["apTitle"].GetString (),
                                           bltValue["aeQualifier"].GetInt ());

        const Value& bltDatapoints = bltValue["datapoints"];

        for (const Value& datapoint : bltDatapoints.GetArray ())
        {
            if (!datapoint.IsObject ())
            {
                Tase2Utility::log_error ("DATAPOINT NOT AN OBJECT");
                continue;
            }
            if (!datapoint.HasMember ("name")
                || !datapoint["name"].IsString ())
            {
                Tase2Utility::log_error ("DATAPOINT HAS NO NAME");
                continue;
            }

            auto it1 = m_modelEntries[bltValue["icc"].GetString ()].find (
                datapoint["name"].GetString ());

            if (it1 == m_modelEntries[bltValue["icc"].GetString ()].end ())
            {
                Tase2Utility::log_debug ("Data point '%s' not found in "
                                         "exchange definitions for ICC '%s'",
                                         datapoint["name"].GetString (),
                                         bltValue["icc"].GetString ());
                continue;
            }

            std::shared_ptr<TASE2Datapoint> t2dp = it1->second;

            if (TASE2Datapoint::isCommand (t2dp->getType ()))
            {
                Tase2_BilateralTable_addControlPoint (
                    blt, t2dp->getControlPoint (), t2dp->getCheckBackId (),
                    true, true, true, true);
                Tase2Utility::log_debug (
                    "Added Control Point '%s' to Bilateral Table '%s'",
                    t2dp->getLabel ().c_str (), bltValue["name"].GetString ());
            }
            else
            {
                Tase2_BilateralTable_addDataPoint (
                    blt, (Tase2_DataPoint)t2dp->getIndicationPoint (), true,
                    false);
                Tase2Utility::log_debug (
                    "Added Data Point '%s' to Bilateral Table '%s'",
                    t2dp->getLabel ().c_str (), bltValue["name"].GetString ());
            }
        }
        m_bilateral_tables.push_back (blt);
    }
}

void
TASE2Config::importProtocolConfig (const std::string& protocolConfig)
{
    m_protocolConfigComplete = false;

    Document document;

    if (document.Parse (const_cast<char*> (protocolConfig.c_str ()))
            .HasParseError ())
    {
        Tase2Utility::log_fatal ("Parsing error in protocol configuration");
        Tase2Utility::log_debug ("Parsing error in protocol configuration\n");
        return;
    }

    if (!document.IsObject ())
    {
        return;
    }

    if (!document.HasMember ("protocol_stack")
        || !document["protocol_stack"].IsObject ())
    {
        return;
    }

    const Value& protocolStack = document["protocol_stack"];

    if (!protocolStack.HasMember ("transport_layer")
        || !protocolStack["transport_layer"].IsObject ())
    {
        Tase2Utility::log_fatal ("transport layer configuration is missing");
        return;
    }

    const Value& transportLayer = protocolStack["transport_layer"];

    if (transportLayer.HasMember ("port"))
    {
        if (transportLayer["port"].IsInt ())
        {
            int tcpPort = transportLayer["port"].GetInt ();

            if (tcpPort > 0 && tcpPort < 65536)
            {
                m_tcpPort = tcpPort;
            }
            else
            {
                Tase2Utility::log_warn ("transport_layer.port value out of "
                                        "range-> using default port");
            }
        }
        else
        {
            Tase2Utility::log_debug ("transport_layer.port has invalid "
                                     "type -> using default port\n");
            Tase2Utility::log_warn (
                "transport_layer.port has invalid type -> using default port");
        }
    }

    if (transportLayer.HasMember ("srv_ip"))
    {
        if (transportLayer["srv_ip"].IsString ())
        {
            if (isValidIPAddress (transportLayer["srv_ip"].GetString ()))
            {
                m_ip = transportLayer["srv_ip"].GetString ();

                Tase2Utility::log_debug ("Using local IP address: %s\n",
                                         m_ip.c_str ());

                m_bindOnIp = true;
            }
            else
            {
                printf ("transport_layer.srv_ip is not a valid IP address -> "
                        "ignore\n");
                Tase2Utility::log_warn ("transport_layer.srv_ip has "
                                        "invalid type -> not using TLS");
            }
        }
    }

    if (transportLayer.HasMember ("tls"))
    {
        if (transportLayer["tls"].IsBool ())
        {
            m_useTLS = transportLayer["tls"].GetBool ();
        }
        else
        {
            Tase2Utility::log_warn ("tls has invalid type -> not using TLS");
        }
    }
}

void
TASE2Config::importExchangeConfig (const std::string& exchangeConfig,
                                   Tase2_DataModel model)
{
    m_exchangeConfigComplete = false;

    Document document;

    if (document.Parse (const_cast<char*> (exchangeConfig.c_str ()))
            .HasParseError ())
    {
        Logger::getLogger ()->fatal (
            "Parsing error in data exchange configuration");

        return;
    }

    if (!document.IsObject ())
        return;

    if (!document.HasMember (JSON_EXCHANGED_DATA)
        || !document[JSON_EXCHANGED_DATA].IsObject ())
    {
        return;
    }

    const Value& exchangeData = document[JSON_EXCHANGED_DATA];

    if (!exchangeData.HasMember (JSON_DATAPOINTS)
        || !exchangeData[JSON_DATAPOINTS].IsArray ())
    {
        return;
    }

    const Value& datapoints = exchangeData[JSON_DATAPOINTS];

    for (const Value& datapoint : datapoints.GetArray ())
    {

        if (!datapoint.IsObject ())
            return;

        if (!datapoint.HasMember (JSON_LABEL)
            || !datapoint[JSON_LABEL].IsString ())
            return;

        std::string label = datapoint[JSON_LABEL].GetString ();

        if (!datapoint.HasMember (JSON_PROTOCOLS)
            || !datapoint[JSON_PROTOCOLS].IsArray ())
            return;

        for (const Value& protocol : datapoint[JSON_PROTOCOLS].GetArray ())
        {

            if (!protocol.HasMember (JSON_PROT_NAME)
                || !protocol[JSON_PROT_NAME].IsString ())
                return;

            std::string protocolName = protocol[JSON_PROT_NAME].GetString ();

            if (!protocol.HasMember (JSON_PROT_REF)
                || !protocol[JSON_PROT_REF].IsString ())
                return;

            std::string protocolRef = protocol[JSON_PROT_REF].GetString ();

            size_t colonPos = protocolRef.find (':');

            if (colonPos != std::string::npos)
            {
                std::string domainRef = protocolRef.substr (0, colonPos);
                std::string dpRef = protocolRef.substr (colonPos + 1);

                auto itD = m_modelEntries.find (domainRef);
                if (itD == m_modelEntries.end ())
                {
                    Tase2Utility::log_warn ("Invalid Domain %s",
                                            domainRef.c_str ());
                    continue;
                }

                std::unordered_map<std::string,
                                   std::shared_ptr<TASE2Datapoint> >
                    domain = itD->second;

                auto itDP = domain.find (dpRef);
                if (itDP == domain.end ())
                {
                    Tase2Utility::log_warn ("Invalid Datapoint ref %s",
                                            dpRef.c_str ());
                    continue;
                }

                itDP->second->setInExchangedDefinitions (true);
            }
            else
            {
                Tase2Utility::log_warn ("Invalid ref %s",
                                        protocolRef.c_str ());
                continue;
            }
        }
    }
}

void
TASE2Config::importTlsConfig (const std::string& tlsConfig)
{
    return;
}

int
TASE2Config::TcpPort ()
{
    if (m_tcpPort == -1)
    {
        return 102;
    }
    else
    {
        return m_tcpPort;
    }
}

std::string
TASE2Config::ServerIp ()
{
    if (m_ip == "")
    {
        return "0.0.0.0";
    }
    else
    {
        return m_ip;
    }
}

std::shared_ptr<TASE2Datapoint>
TASE2Config::getDatapointByReference (const std::string& domainRef,
                                      const std::string& name)
{
    auto itDomain = m_modelEntries.find (domainRef);
    if (itDomain == m_modelEntries.end ())
    {
        return nullptr;
    }

    auto domain = itDomain->second;
    auto itDp = domain.find (name);

    if (itDp == domain.end ())
    {
        return nullptr;
    }

    return itDp->second;
}