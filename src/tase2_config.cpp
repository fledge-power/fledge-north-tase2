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
#define JSON_PROT_OBJ_REF "objref"
#define JSON_PROT_CDC "cdc"

TASE2Config::TASE2Config ()
{
    m_exchangeConfigComplete = false;
    m_protocolConfigComplete = false;
}

void
TASE2Config::deleteExchangeDefinitions ()
{
    if (m_exchangeDefinitions == nullptr)
        return;

    delete m_exchangeDefinitions;

    m_exchangeDefinitions = nullptr;

    if (m_exchangeDefinitionsObjRef == nullptr)
        return;

    delete m_exchangeDefinitionsObjRef;

    m_exchangeDefinitionsObjRef = nullptr;
}

TASE2Config::~TASE2Config () { deleteExchangeDefinitions (); }

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
        return;
    }

    if (!document.HasMember ("model_conf")
        || !document["model_conf"].IsObject ())
    {
        return;
    }

    const Value& modelConf = document["model_conf"];

    if (!modelConf.HasMember ("vcc") || !modelConf["vcc"].IsObject ())
    {
        return;
    }

    const Value& vcc = modelConf["vcc"];

    if (!vcc.HasMember ("datapoints") || !vcc["datapoints"].IsArray ())
    {
        return;
    }

    const Value& vccDatapoints = vcc["datapoints"];
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
    return;
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