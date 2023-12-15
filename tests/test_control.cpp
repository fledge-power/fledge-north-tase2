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
                            "name" : "datapointReal",
                            "type" : "Real",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscrete",
                            "type" : "Discrete",
                            "hasCOV" : false
                        },
                        {
                            "name" : "command1",
                            "type" : "Command",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 123
                        },
                        {
                            "name" : "setpointReal1",
                            "type" : "SetPointReal",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 124
                        },
                        {
                            "name" : "setpointDiscrete1",
                            "type" : "SetpointDiscrete",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 125
                        },
                        {
                            "name" : "command2",
                            "type" : "Command",
                            "mode" : "sbo",
                            "hasTag" : false,
                            "checkBackId" : 126
                        },
                        {
                            "name" : "commandNotInExchange",
                            "type" : "Command",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 126
                        },
                        {
                            "name" : "commandTag",
                            "type" : "Command",
                            "mode" : "sbo",
                            "hasTag" : true,
                            "checkBackId" : 135
                        }
                    ]
                },
                "icc" : [ {
                    "name" : "icc1",
                    "datapoints" : [
                        {
                            "name" : "datapointReal",
                            "type" : "Real",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscrete",
                            "type" : "Discrete",
                            "hasCOV" : false
                        },
                        {
                            "name" : "command1",
                            "type" : "Command",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 123
                        },
                        {
                            "name" : "setpointReal1",
                            "type" : "SetPointReal",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 124
                        },
                        {
                            "name" : "setpointDiscrete1",
                            "type" : "SetPointDiscrete",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 125
                        },
                        {
                            "name" : "command2",
                            "type" : "Command",
                            "mode" : "sbo",
                            "hasTag" : false,
                            "checkBackId" : 126
                        },
                        {
                            "name" : "commandNotInExchange",
                            "type" : "Command",
                            "mode" : "direct",
                            "hasTag" : false,
                            "checkBackId" : 126
                        },
                        {
                            "name" : "commandTag",
                            "type" : "Command",
                            "mode" : "sbo",
                            "hasTag" : true,
                            "checkBackId" : 135
                        }
                    ]
                } ],
                "bilateral_tables" : [
                    {
                        "name" : "BLT_MZA_001_V1",
                        "icc" : "icc1",
                        "apTitle" : "1.1.1.998",
                        "aeQualifier" : 12,
                        "datapoints" : [
                            { "name" : "datapointReal" },
                            { "name" : "datapointDiscrete" },
                            { "name" : "command1" },
                            { "name" : "setpointDiscrete1" },
                            { "name" : "setpointReal1" },
                            { "name" : "command2" }
                        ]
                    },
                    {
                        "name" : "BLT_MZA_002_V1",
                        "icc" : "icc1",
                        "apTitle" : "1.1.1.999",
                        "aeQualifier" : 12,
                        "datapoints" : [
                            { "name" : "datapointReal" },
                            { "name" : "datapointDiscrete" },
                            { "name" : "command1" },
                            { "name" : "setpointDiscrete1" },
                            { "name" : "setpointReal1" },
                            { "name" : "command2" },
                            { "name" : "commandNotInExchange" },
                            { "name" : "commandTag" }
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
                    },
                    {
                        "pivot_id" : "TC1",
                        "label" : "TC1",
                        "protocols" :
                            [ { "name" : "tase2", "ref" : "icc1:command1" } ]
                    },
                    {
                        "pivot_id" : "TC2",
                        "label" : "TC2",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:setpointDiscrete1"
                        } ]
                    },
                    {
                        "pivot_id" : "TC3",
                        "label" : "TC3",
                        "protocols" : [
                            { "name" : "tase2", "ref" : "icc1:setpointReal1" }
                        ]
                    },
                    {
                        "pivot_id" : "TC4",
                        "label" : "TC4",
                        "protocols" :
                            [ { "name" : "tase2", "ref" : "icc1:command2" } ]
                    },
                    {
                        "pivot_id" : "TC5",
                        "label" : "TC5",
                        "protocols" :
                            [ { "name" : "tase2", "ref" : "icc1:commandTag" } ]
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

static int operateHandlerCalled = 0;

static char** receivedParameters = nullptr;

static int
operateHandler (char* operation, int paramCount, char* names[],
                char* parameters[], ControlDestination destination, ...)
{
    if (receivedParameters != nullptr)
    {
        for (int i = 0; receivedParameters[i] != nullptr; i++)
        {
            delete[] receivedParameters[i];
        }

        delete[] receivedParameters;

        receivedParameters = nullptr;
    }
    operateHandlerCalled++;

    receivedParameters = new char*[paramCount + 1];
    for (int i = 0; i < paramCount; i++)
    {
        receivedParameters[i] = new char[strlen (parameters[i]) + 1];
        strcpy (receivedParameters[i], parameters[i]);
    }
    receivedParameters[paramCount] = nullptr;

    for (int i = 0; i < paramCount; i++)
    {
        Tase2Utility::log_info ("Operate handler called %s",
                                receivedParameters[i]);
    }

    return 1;
}

// Class to be called in each test, contains fixture to be used in
class ControlTest : public testing::Test
{
  protected:
    PLUGIN_HANDLE handle;
    // Setup is ran for every tests, so each variable are reinitialised
    void
    SetUp () override
    {
        operateHandlerCalled = 0;
    }

    // TearDown is ran for every tests, so each variable are destroyed
    // again
    void
    TearDown () override
    {
        plugin_shutdown (handle);
        if (receivedParameters != nullptr)
        {
            for (int i = 0; receivedParameters[i] != nullptr; i++)
            {
                delete[] receivedParameters[i];
            }

            delete[] receivedParameters;

            receivedParameters = nullptr;
        }
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

    void
    setupTest (ConfigCategory& config, PLUGIN_HANDLE& handle,
               Tase2_Client& client) const
    {
        config = ConfigCategory ("tase2Config", default_config);
        handle = plugin_init (&config);
        plugin_register (handle, nullptr, operateHandler);
        plugin_start (handle, "");

        Thread_sleep (500);
        client = Tase2_Client_create (nullptr);
        Tase2_Client_setLocalApTitle (client, "1.1.1.998", 12);
        Tase2_Client_setRemoteApTitle (client, "1.1.1.999", 12);
        Tase2_Client_setTcpPort (client, TCP_TEST_PORT);
    }

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

        DPTYPE dpType = TASE2Datapoint::getDpTypeFromString (type);

        switch (dpType)
        {
        case DISCRETE:
        case DISCRETEQ:
        case DISCRETEQTIME:
        case DISCRETEQTIMEEXT:
        case STATESUP:
        case STATESUPQ:
        case STATESUPQTIME:
        case STATESUPQTIMEEXT:
        case STATE:
        case STATEQ:
        case STATEQTIME:
        case STATEQTIMEEXT: {
            datapoints->push_back (
                createDatapoint ("do_value", (int64_t)value));
            break;
        }
        case REAL:
        case REALQ:
        case REALQTIME:
        case REALQTIMEEXT: {
            datapoints->push_back (
                createDatapoint ("do_value", (double)value));
            break;
        }
        default: {
            break;
        }
        }

        datapoints->push_back (createDatapoint ("do_validity", iv));
        datapoints->push_back (createDatapoint ("do_cs", cs));
        datapoints->push_back (
            createDatapoint ("do_quality_normal_value", nv));
        if (ts)
        {
            datapoints->push_back (createDatapoint ("do_ts", (long)ts));
            datapoints->push_back (createDatapoint ("do_ts_validity", tsv));
        }

        DatapointValue dpv (datapoints, true);

        Datapoint* dp = new Datapoint ("data_object", dpv);

        return dp;
    }
};

TEST_F (ControlTest, SimpleCommand)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendCommand (client, &err, "icc1", "command1", 1);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 1);

    ASSERT_EQ (strcmp (receivedParameters[0], "Command"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "command1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[4], "1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SimpleCommandSBO)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    int checkbackId
        = Tase2_Client_selectDevice (client, &err, "icc1", "command2");

    ASSERT_EQ (126, checkbackId);

    Thread_sleep (1000);
    Tase2_Client_sendCommand (client, &err, "icc1", "command2", 1);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 2);

    ASSERT_EQ (strcmp (receivedParameters[0], "Command"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "command2"), 0);
    ASSERT_EQ (strcmp (receivedParameters[4], "1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SimpleCommandSBONoSelect)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendCommand (client, &err, "icc1", "command2", 1);

    ASSERT_NE (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SimpleCommandSBOTag)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_setTag (client, &err, "icc1", "commandTag",
                         TASE2_TAG_OPEN_AND_CLOSE_INHIBIT, "Yes");

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    bool isArmed = false;

    Tase2_TagValue tagValue = Tase2_Client_getTag (
        client, &err, "icc1", "commandTag", &isArmed, NULL, 0);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);
    ASSERT_EQ (TASE2_TAG_OPEN_AND_CLOSE_INHIBIT, tagValue);

    int checkbackId
        = Tase2_Client_selectDevice (client, &err, "icc1", "commandTag");

    ASSERT_TRUE (err != TASE2_CLIENT_ERROR_OK);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SetpointDiscrete)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendDiscreteSetPoint (client, &err, "icc1",
                                       "setpointDiscrete1", 1);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 1);

    ASSERT_EQ (strcmp (receivedParameters[0], "SetPointDiscrete"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "setpointDiscrete1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[4], "1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SetpointReal)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendRealSetPoint (client, &err, "icc1", "setpointReal1",
                                   1.38f);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 1);

    ASSERT_EQ (strcmp (receivedParameters[0], "SetPointReal"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "setpointReal1"), 0);
    ASSERT_NEAR (stof (receivedParameters[4]), 1.38, 0.0001);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, OutstandingCommandSuccess)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendRealSetPoint (client, &err, "icc1", "setpointReal1",
                                   1.38f);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 1);

    ASSERT_EQ (((TASE2Server*)handle)->m_outstandingCommands.size (), 1);

    ASSERT_EQ (strcmp (receivedParameters[0], "SetPointReal"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "setpointReal1"), 0);
    ASSERT_NEAR (stof (receivedParameters[4]), 1.38, 0.0001);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    auto* dataobjects = new vector<Datapoint*>;
    dataobjects->push_back (createDataObject (
        "SetPointReal", "icc1", "setpointReal1", (float)1.38, "valid",
        "telemetered", "normal", (uint64_t)123456, "valid"));
    auto* reading = new Reading (std::string ("TC3"), *dataobjects);
    vector<Reading*> readings;
    readings.push_back (reading);
    plugin_send (handle, readings);
    delete dataobjects;
    delete reading;
    readings.clear ();
    Thread_sleep (50);

    ASSERT_EQ (((TASE2Server*)handle)->m_outstandingCommands.size (), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, OutstandingCommandFailure)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendRealSetPoint (client, &err, "icc1", "setpointReal1",
                                   1.38f);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 1);
    ASSERT_EQ (((TASE2Server*)handle)->m_outstandingCommands.size (), 1);

    ASSERT_EQ (strcmp (receivedParameters[0], "SetPointReal"), 0);
    ASSERT_EQ (strcmp (receivedParameters[1], "domain"), 0);
    ASSERT_EQ (strcmp (receivedParameters[2], "icc1"), 0);
    ASSERT_EQ (strcmp (receivedParameters[3], "setpointReal1"), 0);
    ASSERT_NEAR (stof (receivedParameters[4]), 1.38, 0.0001);
    ASSERT_EQ (strcmp (receivedParameters[5], "0"), 0);
    ASSERT_NE (strcmp (receivedParameters[6], ""), 0);

    Thread_sleep (50);

    ASSERT_EQ (((TASE2Server*)handle)->m_outstandingCommands.size (), 1);

    Thread_sleep (6000);

    ASSERT_EQ (((TASE2Server*)handle)->m_outstandingCommands.size (), 0);

    Tase2_Client_destroy (client);
}

TEST_F (ControlTest, SimpleCommandNotInExchange)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    Tase2_Client_sendCommand (client, &err, "icc1", "commandNotInExchange", 1);

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (operateHandlerCalled, 0);

    Tase2_Client_destroy (client);
}