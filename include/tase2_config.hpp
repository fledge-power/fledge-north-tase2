#ifndef TASE2_CONFIG_H
#define TASE2_CONFIG_H

#include "logger.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "tase2_datapoint.hpp"

#include <gtest/gtest.h>
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

    std::string
    localAP ()
    {
        return m_localAP;
    }

    int
    localAE ()
    {
        return m_localAe;
    }

    std::string
    remoteAP ()
    {
        return m_remoteAP;
    }

    int
    remoteAE ()
    {
        return m_remoteAe;
    }

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
        std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> > >
    getModelEntries ()
    {
        return m_modelEntries;
    };

    std::vector<Tase2_BilateralTable>&
    getBilateralTables ()
    {
        return m_bilateral_tables;
    };

    std::shared_ptr<TASE2Datapoint>
    getDatapointByReference (const std::string& ref, const std::string& name);

    int
    CmdExecTimeout ()
    {
        return m_cmdExecTimeout;
    };

    std::string&
    GetPrivateKey ()
    {
        return m_privateKey;
    };
    std::string&
    GetOwnCertificate ()
    {
        return m_ownCertificate;
    };
    std::vector<std::string>&
    GetRemoteCertificates ()
    {
        return m_remoteCertificates;
    };
    std::vector<std::string>&
    GetCaCertificates ()
    {
        return m_caCertificates;
    };

    bool
    Passive ()
    {
        return m_passive;
    }

  private:
    static bool isValidTase2Reference (const std::string& ref);
    static bool isValidIPAddress (const std::string& addrStr);

    std::string m_remoteAP = "";
    std::string m_localAP = "";

    int m_localAe;
    int m_remoteAe;

    std::string m_ip = "";
    int m_tcpPort = -1;
    int m_cmdExecTimeout = 5;

    bool m_useTLS = false;

    bool m_bindOnIp;
    bool m_protocolConfigComplete = false;
    bool m_exchangeConfigComplete = false;

    bool m_passive = true;

    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::shared_ptr<TASE2Datapoint> > >
        m_modelEntries;

    std::vector<Tase2_BilateralTable> m_bilateral_tables;
    std::unordered_map<std::string, Tase2_Domain> m_domains;
    std::string m_privateKey;
    std::string m_ownCertificate;
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;
    FRIEND_TEST (DatasetTest, ValidReference);
};

#endif
