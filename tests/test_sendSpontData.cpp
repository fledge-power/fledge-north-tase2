#include "tase2.hpp"
#include <filter.h>
#include <gtest/gtest.h>
#include <libtase2/tase2_client.h>
#include <reading.h>
#include <reading_set.h>

using namespace std;

#define TCP_TEST_PORT 10002
#define LOCAL_HOST "127.0.0.1"

extern "C"
{
    PLUGIN_INFORMATION* plugin_info ();

    PLUGIN_HANDLE plugin_init (ConfigCategory* config);

    void plugin_shutdown (PLUGIN_HANDLE handle);

    void plugin_register (PLUGIN_HANDLE handle,
                          bool (*write) (const char* name, const char* value,
                                         ControlDestination destination, ...),
                          int (*operation) (char* operation, int paramCount,
                                            char* names[], char* parameters[],
                                            ControlDestination destination,
                                            ...));
    void plugin_start (const PLUGIN_HANDLE handle, const string& storedData);

    uint32_t plugin_send (const PLUGIN_HANDLE handle,
                          const vector<Reading*>& readings);
};

static const char* default_config = QUOTE ({
    "plugin" : {
        "description" : "TASE2 Server Plugin",
        "type" : "string",
        "default" : PLUGIN_NAME,
        "readonly" : "true"
    },
    "name" : {
        "description" : "The TASE2 Server name to advertise",
        "type" : "string",
        "default" : "Fledge TASE2 North Plugin",
        "order" : "1",
        "displayName" : "Server Name"
    },
    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "2",
        "default" : QUOTE ({
            "protocol_stack" : {
                "name" : "tase2",
                "version" : "1.0",
                "transport_layer" : { "srv_ip" : "0.0.0.0", "port" : 10002 }
            }
        })
    },
    "model_conf" : {
        "description" : "Tase2 Data Model configuration",
        "type" : "JSON",
        "displayName" : "Data Model",
        "order" : "3",
        "default" : QUOTE ({
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
                            "type" : "DiscreteQTimeExt",
                            "hasCOV" : false
                        }
                    ]
                },
                "icc" : [ {
                    "name" : "icc1",
                    "datapoints" : [
                        {
                            "name" : "datapoint1",
                            "type" : "StateQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapoint2",
                            "type" : "DiscreteQTimeExt",
                            "hasCOV" : false
                        }
                    ]
                } ],
                "bilateral_tables" : [
                    {
                        "name" : "BLT_MZA_001_V1",
                        "icc" : "icc1",
                        "apTitle" : "1.1.1.998",
                        "aeQualifier" : 12,
                        "datapoints" : [ { "name" : "datapoint1" } ]
                    },
                    {
                        "name" : "BLT_MZA_002_V1",
                        "icc" : "icc1",
                        "apTitle" : "1.1.1.999",
                        "aeQualifier" : 12,
                        "datapoints" : [
                            { "name" : "datapoint1" },
                            { "name" : "datapoint2" }
                        ]
                    }
                ]
            }
        })
    },

    "exchanged_data" : {
        "description" : "Exchanged datapoints configuration",
        "type" : "JSON",
        "displayName" : "Exchanged datapoints",
        "order" : "4",
        "default" : QUOTE ({
            "exchanged_data" : {
                "datapoints" : [
                    {
                        "pivot_id" : "TS1",
                        "label" : "TS1",
                        "protocols" :
                            [ { "name" : "tase2", "ref" : "icc1:datapoint1" } ]
                    },
                    {
                        "pivot_id" : "TS2",
                        "label" : "TS2",
                        "protocols" :
                            [ { "name" : "tase", "ref" : "icc2:datapoint2" } ]
                    }
                ]
            }
        })
    },
    "tls_conf" : {
        "description" : "TLS configuration",
        "type" : "JSON",
        "displayName" : "TLS Configuration",
        "order" : "5",
        "default" : QUOTE ({
            "tls_conf" : {
                "private_key" : "tase2_server.key",
                "own_cert" : "tase2_server.cer",
                "ca_certs" : [
                    { "cert_file" : "tase2_ca.cer" },
                    { "cert_file" : "tase2_ca2.cer" }
                ],
                "remote_certs" :
                    [ { "cert_file" : "tase2_client.cer" } ]
            }
        })
    }
});

// Class to be called in each test, contains fixture to be used in
class SendSpontDataTest : public testing::Test
{
  protected:
    // Setup is ran for every tests, so each variable are reinitialised
    void
    SetUp () override
    {
        // Init tase2server object
    }

    // TearDown is ran for every tests, so each variable are destroyed
    // again
    void
    TearDown () override
    {
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

template <class T>
static Datapoint*
createDatapoint (const std::string& dataname, const T value)
{
    DatapointValue dp_value = DatapointValue (value);
    return new Datapoint (dataname, dp_value);
}

template <class T>
static Datapoint*
createDataObject (const char* type, const char* domain, const char* name,
                  const T value, const char* iv, const char* cs,
                  const char* nv, uint64_t ts, const char* tsv)
{
    auto* datapoints = new vector<Datapoint*>;

    datapoints->push_back (createDatapoint ("do_type", type));
    datapoints->push_back (createDatapoint ("do_domain", domain));
    datapoints->push_back (createDatapoint ("do_name", name));
    datapoints->push_back (createDatapoint ("do_value", value));
    datapoints->push_back (createDatapoint ("do_validity", iv));
    datapoints->push_back (createDatapoint ("do_cs", cs));
    datapoints->push_back (createDatapoint ("do_quality_normal_value", nv));
    if (ts)
    {
        datapoints->push_back (createDatapoint ("do_ts", (long)ts));
        datapoints->push_back (createDatapoint ("do_ts_validity", tsv));
    }

    DatapointValue dpv (datapoints, true);

    Datapoint* dp = new Datapoint ("data_object", dpv);

    return dp;
}

TEST_F (SendSpontDataTest, SendStateQTimeExt)
{
    ConfigCategory config ("tase2Config", default_config);
    PLUGIN_HANDLE handle = plugin_init (&config);

    plugin_start (handle, "");

    Thread_sleep (500); /* wait for the server to start */

    Tase2_Client client = Tase2_Client_create (nullptr);

    Tase2_Client_setLocalApTitle (client, "1.1.1.998", 12);
    Tase2_Client_setRemoteApTitle (client, "1.1.1.999", 12);

    Tase2_Client_setTcpPort (client, TCP_TEST_PORT);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);

    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    auto* dataobjects = new vector<Datapoint*>;

    dataobjects->push_back (createDataObject (
        "StateQTime", "icc1", "datapoint1", (int64_t)1, "valid", "telemetered",
        "normal", (uint64_t)100000, "valid"));

    Reading* reading = new Reading (std::string ("TS1"), *dataobjects);

    vector<Reading*> readings;

    readings.push_back (reading);

    plugin_send (handle, readings);

    Thread_sleep (1000);

    const Tase2_PointValue& pv
        = Tase2_Client_readPointValue (client, &err, "icc1", "datapoint1");

    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_DataState state = Tase2_PointValue_getValueState (pv);

    ASSERT_EQ (state, 1);

    Tase2_Client_destroy (client);
    plugin_shutdown (handle);
}
