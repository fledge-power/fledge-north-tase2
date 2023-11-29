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
