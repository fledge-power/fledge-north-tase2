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


bool TASE2Config::isValidTase2Reference(const std::string& ref)
{
    std::regex pattern("[^A-Za-z$_0-9]");

    return !std::regex_search(ref, pattern);
}

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
        
        if(!isValidTase2Reference(datapoint["name"].GetString()))
        {
            Tase2Utility::log_error ("Invalid TASE2 reference %s, reference must only contain letters, numbers, $ or _",
                                     datapoint["name"].GetString ());
            continue;
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
        m_modelEntries["vcc"][t2dp->getLabel ()] = t2dp;

        Tase2Utility::log_debug (
            "Add datapoint %s to vcc, %d datapoints present",
            t2dp->getLabel ().c_str (), m_modelEntries.size ());
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

            if(!isValidTase2Reference(datapoint["name"].GetString()))
            {
                Tase2Utility::log_error ("Invalid TASE2 reference %s, reference must only contain letters, numbers, $ or _",
                                        datapoint["name"].GetString ());
                continue;
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
                    strcmp (datapoint["mode"].GetString (), "sbo") != 0);

                Tase2Utility::log_debug ("Device class is %d", deviceClass);

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

    if (modelConf.HasMember ("dataset_transfer_sets"))
    {
        if (!modelConf["dataset_transfer_sets"].IsArray ())
        {
            Tase2Utility::log_error ("invalid 'dataset_transfer_sets' "
                                     "array in 'model_conf'");
            return;
        }

        const Value& dtsArray = modelConf["dataset_transfer_sets"];

        for (const Value& dts : dtsArray.GetArray ())
        {
            if (!dts.IsObject ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET NOT AN OBJECT");
                return;
            }
            if (!dts.HasMember ("name") || !dts["name"].IsString ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET HAS NO NAME");
                return;
            }
            if (!dts.HasMember ("domain") || !dts["domain"].IsString ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET HAS NO DOMAIN");
                return;
            }

            std::string dtsDomainName = dts["domain"].GetString ();

            auto itD = m_domains.find (dtsDomainName);
            if (itD == m_domains.end ())
            {
                Tase2Utility::log_warn ("Invalid Domain %s",
                                        dtsDomainName.c_str ());
                continue;
            }

            Tase2_Domain dtsDomain = itD->second;

            Tase2_Domain_addDSTransferSet (dtsDomain,
                                           dts["name"].GetString ());
        }
    }

    if (modelConf.HasMember ("datasets"))
    {
        if (!modelConf["datasets"].IsArray ())
        {
            Tase2Utility::log_error ("invalid 'datasets' "
                                     "array in 'model_conf'");
            return;
        }

        const Value& dsArray = modelConf["datasets"];

        for (const Value& ds : dsArray.GetArray ())
        {
            if (!ds.IsObject ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET NOT AN OBJECT");
                continue;
            }
            if (!ds.HasMember ("name") || !ds["name"].IsString ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET HAS NO NAME");
                continue;
            }
            if (!ds.HasMember ("domain") || !ds["domain"].IsString ())
            {
                Tase2Utility::log_error ("DATASET TRANSFER SET HAS NO DOMAIN");
                continue;
            }

            std::string dsDomainName = ds["domain"].GetString ();

            auto itD = m_domains.find (dsDomainName);
            if (itD == m_domains.end ())
            {
                Tase2Utility::log_warn ("Invalid Domain %s",
                                        dsDomainName.c_str ());
                continue;
            }

            Tase2_Domain dsDomain = itD->second;

            Tase2_DataSet dataSet
                = Tase2_Domain_addDataSet (dsDomain, ds["name"].GetString ());

            Tase2Utility::log_debug ("Create dataset %s in domain %s",
                                     ds["name"].GetString (),
                                     ds["domain"].GetString ());

            if (!ds.HasMember ("datapoints") || !ds["datapoints"].IsArray ())
            {
                Tase2Utility::log_error ("invalid 'datapoints' "
                                         "array in dataset %s",
                                         ds["name"].GetString ());
                continue;
            }

            const Value& datasetDatapoints = ds["datapoints"];

            for (const Value& dp : datasetDatapoints.GetArray ())
            {
                if (!dp.IsString ())
                {
                    Tase2Utility::log_error (
                        "Invalid datapoint in dataset %s (not std::string)",
                        ds["name"].GetString ());
                    continue;
                }

                if (getDatapointByReference (ds["domain"].GetString (),
                                             dp.GetString ()))
                {
                    Tase2_DataSet_addEntry (dataSet, dsDomain,
                                            dp.GetString ());

                    Tase2Utility::log_debug ("Add entry %s to dataset %s",
                                             dp.GetString (),
                                             ds["name"].GetString ());
                }
                else
                {
                    Tase2Utility::log_error (
                        "datapoint %s not found in dataset %s",
                        dp.GetString (), ds["name"].GetString ());
                    continue;
                }
            }
        }
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

    if (transportLayer.HasMember ("passive"))
    {
        if (transportLayer["passive"].IsBool ())
        {
            m_passive = transportLayer["passive"].GetBool ();
        }
        else
        {
            Tase2Utility::log_warn (
                "passive has invalid type -> server is passive");
        }
    }

    if (transportLayer.HasMember ("localApTitle"))
    {
        if (transportLayer["localApTitle"].IsString ())
        {
            std::string localApString
                = transportLayer["localApTitle"].GetString ();

            size_t colonPos = localApString.find (':');

            if (colonPos == std::string::npos)
            {
                Tase2Utility::log_warn ("Invalid local AP Title %s",
                                        localApString.c_str ());
            }

            m_localAP = localApString.substr (0, colonPos);

            try
            {
                int localAe = std::stoi (localApString.substr (colonPos + 1));
                m_localAe = localAe;
                Tase2Utility::log_debug ("Using localApTitle address: %s\n",
                                         m_localAP.c_str ());
            }
            catch (...)
            {
                Tase2Utility::log_warn ("Invalid local AP Title %s",
                                        localApString.c_str ());
            }
        }
    }

    if (transportLayer.HasMember ("remoteApTitle"))
    {
        if (transportLayer["remoteApTitle"].IsString ())
        {

            std::string remoteApString
                = transportLayer["remoteApTitle"].GetString ();

            size_t colonPos = remoteApString.find (':');

            if (colonPos == std::string::npos)
            {
                Tase2Utility::log_warn ("Invalid remote AP Title %s",
                                        remoteApString.c_str ());
            }

            m_remoteAP = remoteApString.substr (0, colonPos);

            try
            {
                int remoteAe
                    = std::stoi (remoteApString.substr (colonPos + 1));
                m_remoteAe = remoteAe;
                Tase2Utility::log_debug ("Using remoteApTitle address: %s\n",
                                         m_remoteAP.c_str ());
            }
            catch (...)
            {
                Tase2Utility::log_warn ("Invalid remote AP Title %s",
                                        remoteApString.c_str ());
            }
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
        Tase2Utility::log_fatal (
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
                    Tase2Utility::log_warn ("Invalid Datapoint ref %s:%s",
                                            domainRef.c_str(), dpRef.c_str ());
                    continue;
                }

                Tase2Utility::log_debug ("Add dp to Exchange Def %s %s",
                                            domainRef.c_str (), dpRef.c_str());

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
    Document document;

    if (document.Parse (const_cast<char*> (tlsConfig.c_str ()))
            .HasParseError ())
    {
        Tase2Utility::log_fatal ("Parsing error in TLS configuration");

        return;
    }

    if (!document.IsObject ())
        return;

    if (!document.HasMember ("tls_conf") || !document["tls_conf"].IsObject ())
    {
        return;
    }

    const Value& tlsConf = document["tls_conf"];

    if (tlsConf.HasMember ("private_key")
        && tlsConf["private_key"].IsString ())
    {
        m_privateKey = tlsConf["private_key"].GetString ();
    }

    if (tlsConf.HasMember ("own_cert") && tlsConf["own_cert"].IsString ())
    {
        m_ownCertificate = tlsConf["own_cert"].GetString ();
    }

    if (tlsConf.HasMember ("ca_certs") && tlsConf["ca_certs"].IsArray ())
    {

        const Value& caCerts = tlsConf["ca_certs"];

        for (const Value& caCert : caCerts.GetArray ())
        {
            if (caCert.HasMember ("cert_file"))
            {
                if (caCert["cert_file"].IsString ())
                {
                    std::string certFileName
                        = caCert["cert_file"].GetString ();

                    m_caCertificates.push_back (certFileName);
                }
            }
        }
    }

    if (tlsConf.HasMember ("remote_certs")
        && tlsConf["remote_certs"].IsArray ())
    {

        const Value& remoteCerts = tlsConf["remote_certs"];

        for (const Value& remoteCert : remoteCerts.GetArray ())
        {
            if (remoteCert.HasMember ("cert_file"))
            {
                if (remoteCert["cert_file"].IsString ())
                {
                    std::string certFileName
                        = remoteCert["cert_file"].GetString ();

                    m_remoteCertificates.push_back (certFileName);
                }
            }
        }
    }
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
        return m_passive ? "0.0.0.0" : "127.0.0.1";
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