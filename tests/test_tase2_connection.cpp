#include "tase2.hpp"
#include <gtest/gtest.h>
#include <libtase2/tase2_client.h>

using namespace std;

#define TCP_TEST_PORT 10002
#define LOCAL_HOST "127.0.0.1"

static string protocol_stack = QUOTE ({
    "protocol_stack" : {
        "name" : "tase2north",
        "version" : "1.0",
        "transport_layer" : {
            "srv_ip" : "0.0.0.0",
            "port" : 10002,
            "passive" : true,
            "localApTitle" : "1.1.1.999:12",
            "remoteApTitle" : "1.1.1.998:12"
        }
    }
});

static string protocol_stack_tls = QUOTE ({
    "protocol_stack" : {
        "name" : "tase2north",
        "version" : "1.0",
        "transport_layer" : {
            "srv_ip" : "0.0.0.0",
            "port" : 10002,
            "passive" : true,
            "localApTitle" : "1.1.1.999:12",
            "remoteApTitle" : "1.1.1.998:12",
            "tls" : true
        }
    }
});

static string protocol_stack_active = QUOTE ({
    "protocol_stack" : {
        "name" : "tase2north",
        "version" : "1.0",
        "transport_layer" : {
            "srv_ip" : "0.0.0.0",
            "port" : 10002,
            "passive" : false,
            "localApTitle" : "1.1.1.999:12",
            "remoteApTitle" : "1.1.1.998:12",
            "remote_ip" : "127.0.0.1"
        }
    }
});

static string exchanged_data
    = QUOTE ({ "exchanged_data" : { "datapoints" : [] } });

static string tls = QUOTE ({
    "tls_conf" : {
        "private_key" : "tase2_server.key",
        "own_cert" : "tase2_server.cer",
        "ca_certs" : [ { "cert_file" : "tase2_ca.cer" } ],
        "remote_certs" : [ { "cert_file" : "tase2_client.cer" } ]
    }
});

static string model_config = QUOTE ({
    "model_conf" : {
        "vcc" : {
            "datapoints" : [
                {
                    "name" : "datapoint1",
                    "type" : "StateQTime",
                    "hasCOV" : false
                },
                {
                    "name" : "datapoint2",
                    "type" : "StateQTimeExt",
                    "hasCOV" : false
                }
            ]
        },
        "icc" : [
            {
                "name" : "ICC1",
                "datapoints" : [ {
                    "name" : "datapoint1",
                    "type" : "StateQTime",
                    "hasCOV" : false
                } ]
            },
            {
                "name" : "ICC2",
                "datapoints" : [ {
                    "name" : "datapoint2",
                    "type" : "StateQTimeExt",
                    "hasCOV" : false
                } ]
            }
        ],
        "bilateral_tables" : [
            {
                "name" : "BLT_MZA_001_V1",
                "icc" : "ICC1",
                "apTitle" : "1.1.1.998",
                "aeQualifier" : 12,
                "datapoints" : [ { "name" : "datapoint1" } ]
            },
            {
                "name" : "BLT_MZA_002_V1",
                "icc" : "ICC2",
                "apTitle" : "1.1.1.999",
                "aeQualifier" : 12,
                "datapoints" : [ { "name" : "datapoint2" } ]
            }
        ]
    }
});

// Class to be called in each test, contains fixture to be used in
class ConnectionHandlerTest : public testing::Test
{
  protected:
    TASE2Server* tase2Server; // Object on which we call for tests
    Tase2_Endpoint endpoint;
    // Setup is ran for every tests, so each variable are reinitialised
    void
    SetUp () override
    {
        // Init tase2server object
        tase2Server = new TASE2Server ();
    }

    // TearDown is ran for every tests, so each variable are destroyed again
    void
    TearDown () override
    {
        delete tase2Server;
    }

    /* callback handler that is called twice for each received transfer set
     * report */
    static void
    dsTransferSetReportHandler (void* parameter, bool finished, uint32_t seq,
                                Tase2_ClientDSTransferSet transferSet)
    {
        if (finished)
        {
            printf ("--> (%i) report processing finished\n", seq);
        }
        else
        {
            printf ("New report received with seq no: %u\n", seq);
        }
    }

    /* callback handler that is called for each data point of a received
     * transfer set report */
    static void
    dsTransferSetValueHandler (void* parameter,
                               Tase2_ClientDSTransferSet transferSet,
                               const char* domainName, const char* pointName,
                               Tase2_PointValue pointValue)
    {
        printf ("  Received value for %s:%s\n", domainName, pointName);
    }

    static void
    connectionClosedHandler (void* parameter, Tase2_Client connection)
    {
        printf ("  Connection to server closed server!\n");
    }

    /**
     * callback handler for library log messages
     */
    static void
    logHandler (Tase2_LogLevel logLevel, Tase2_LogSource source,
                Tase2_Endpoint endpoint, Tase2_Endpoint_Connection peer,
                const char* message)
    {
        const char* levelStr = "UNKNOWN";

        switch (logLevel)
        {
        case TASE2_LOG_DEBUG:
            levelStr = "DEBUG";
            break;
        case TASE2_LOG_INFO:
            levelStr = "INFO";
            break;
        case TASE2_LOG_WARNING:
            levelStr = "WARNING";
            break;
        case TASE2_LOG_ERROR:
            levelStr = "ERROR";
            break;
        default:
            break;
        }

        const char* sourceStr = "UNKNOWN";

        switch (source)
        {
        case TASE2_LOG_SOURCE_ICCP:
            sourceStr = "ICCP";
            break;
        case TASE2_LOG_SOURCE_MMS:
            sourceStr = "MMS";
            break;
        case TASE2_LOG_SOURCE_TLS:
            sourceStr = "TLS";
            break;
        case TASE2_LOG_SOURCE_TCP:
            sourceStr = "TCP";
            break;
        case TASE2_LOG_SOURCE_ISO_LAYERS:
            sourceStr = "ISO";
            break;
        default:
            sourceStr = "UNKOWN";
            break;
        }

        printf ("-- LOG (%s|%s) [%s][%s]: %s\n", levelStr, sourceStr,
                endpoint ? Tase2_Endpoint_getId (endpoint) : "-",
                peer ? Tase2_Endpoint_Connection_getPeerIpAddress (peer) : "-",
                message);
    }
};

TEST_F (ConnectionHandlerTest, NormalConnectionPassive)
{
    tase2Server->setJsonConfig (protocol_stack, exchanged_data, "",
                                model_config);

    tase2Server->start ();

    Thread_sleep (500); /* wait for the server to start */

    Tase2_Client client = Tase2_Client_create (nullptr);

    Tase2_Client_setLocalApTitle (client, "1.1.1.998", 12);
    Tase2_Client_setRemoteApTitle (client, "1.1.1.999", 12);

    Tase2_Client_setTcpPort (client, TCP_TEST_PORT);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);

    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_destroy (client);
}

TEST_F (ConnectionHandlerTest, NormalConnectionActive)
{
    tase2Server->setJsonConfig (protocol_stack_active, exchanged_data, "",
                                model_config);

    Tase2_Endpoint endpoint = Tase2_Endpoint_create (nullptr, true);

    Tase2_Endpoint_setLocalIpAddress (endpoint, "0.0.0.0");
    Tase2_Endpoint_setLocalTcpPort (endpoint, 10002);
    Tase2_Endpoint_setLocalApTitle (endpoint, "1.1.1.998", 12);
    Tase2_Endpoint_setRemoteApTitle (endpoint, "1.1.1.999", 12);

    Tase2_Client client = Tase2_Client_createEx (endpoint);
    Thread_sleep (100);

    Tase2_Endpoint_connect (endpoint);

    ASSERT_TRUE (Tase2_Endpoint_getState (endpoint)
                 == TASE2_ENDPOINT_STATE_LISTENING);

    tase2Server->start ();

    Thread_sleep (1500);

    ASSERT_TRUE (Tase2_Endpoint_getState (tase2Server->m_endpoint)
                 == TASE2_ENDPOINT_STATE_CONNECTED);

    Tase2_Client_destroy (client);
    Tase2_Endpoint_destroy (endpoint);
}

TEST_F (ConnectionHandlerTest, NormalConnectionTLS)
{
    setenv ("FLEDGE_DATA", "../tests/data", 1);

    tase2Server->setJsonConfig (protocol_stack_tls, exchanged_data, tls,
                                model_config);

    tase2Server->start ();

    Thread_sleep (500); /* wait for the server to start */

    TLSConfiguration tlsConfig = TLSConfiguration_create ();

    TLSConfiguration_addCACertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/tase2_ca.cer");
    TLSConfiguration_setOwnCertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/tase2_client.cer");
    TLSConfiguration_setOwnKeyFromFile (
        tlsConfig, "../tests/data/etc/certs/tase2_client.key", NULL);
    TLSConfiguration_addAllowedCertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/tase2_server.cer");
    TLSConfiguration_setChainValidation (tlsConfig, true);
    TLSConfiguration_setAllowOnlyKnownCertificates (tlsConfig, true);

    Tase2_Client client = Tase2_Client_create (tlsConfig);

    Tase2_Client_setLocalApTitle (client, "1.1.1.998", 12);
    Tase2_Client_setRemoteApTitle (client, "1.1.1.999", 12);

    Tase2_Client_setTcpPort (client, TCP_TEST_PORT);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);

    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    TLSConfiguration_destroy (tlsConfig);
    Tase2_Client_destroy (client);
}
