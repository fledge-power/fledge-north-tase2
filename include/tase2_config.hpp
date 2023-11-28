#ifndef TASE2_CONFIG_H
#define TASE2_CONFIG_H

#include "logger.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "tase2_datapoint.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <libtase2/tase2_server.h>

class TASE2Config
{
  public:
    TASE2Config ();
    TASE2Config (const std::string& protocolConfig,
                 const std::string& exchangeConfig);
    ~TASE2Config ();

    void importProtocolConfig (const std::string& protocolConfig);
    void importExchangeConfig (const std::string& exchangeConfig,
                               Tase2_DataModel model);
    void importModelConfig (const std::string& modelConfig,
                            Tase2_DataModel model);
    void importTlsConfig (const std::string& tlsConfig);

    int TcpPort ();
    bool
    bindOnIp ()
    {
        return m_bindOnIp;
    };
    std::string ServerIp ();

    bool
    TLSEnabled ()
    {
        return m_useTLS;
    };

    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> > >&
    getExchangeDefinitions ()
    {
        return m_exchangeDefinitions;
    };

    std::shared_ptr<TASE2Datapoint>
    getDatapointByObjectReference (const std::string& objRef);

  private:
    static bool isValidIPAddress (const std::string& addrStr);

    std::string m_ip = "";
    int m_tcpPort = -1;

    bool m_useTLS = false;

    bool m_bindOnIp;
    bool m_protocolConfigComplete = false;
    bool m_exchangeConfigComplete = false;

    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> > >
        m_exchangeDefinitions;

    std::unordered_map<std::string, Tase2_Domain> m_domains;
    std::string m_privateKey;
    std::string m_ownCertificate;
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;
};

#endif
