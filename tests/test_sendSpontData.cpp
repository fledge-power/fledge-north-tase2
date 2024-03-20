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
                            "name" : "datapoint1",
                            "type" : "StateQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapoint2",
                            "type" : "DiscreteQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointReal",
                            "type" : "Real",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQ",
                            "type" : "RealQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQTime",
                            "type" : "RealQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQTimeExt",
                            "type" : "RealQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointState",
                            "type" : "State",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQ",
                            "type" : "StateQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQTime",
                            "type" : "StateQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQTimeExt",
                            "type" : "StateQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscrete",
                            "type" : "Discrete",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQ",
                            "type" : "DiscreteQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQTime",
                            "type" : "DiscreteQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQTimeExt",
                            "type" : "DiscreteQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSup",
                            "type" : "StateSup",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQ",
                            "type" : "StateSupQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQTime",
                            "type" : "StateSupQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQTimeExt",
                            "type" : "StateSupQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointNotInExchange",
                            "type" : "StateSupQTimeExt",
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
                        },
                        {
                            "name" : "datapointReal",
                            "type" : "Real",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQ",
                            "type" : "RealQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQTime",
                            "type" : "RealQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointRealQTimeExt",
                            "type" : "RealQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointState",
                            "type" : "State",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQ",
                            "type" : "StateQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQTime",
                            "type" : "StateQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateQTimeExt",
                            "type" : "StateQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscrete",
                            "type" : "Discrete",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQ",
                            "type" : "DiscreteQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQTime",
                            "type" : "DiscreteQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointDiscreteQTimeExt",
                            "type" : "DiscreteQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSup",
                            "type" : "StateSup",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQ",
                            "type" : "StateSupQ",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQTime",
                            "type" : "StateSupQTime",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointStateSupQTimeExt",
                            "type" : "StateSupQTimeExt",
                            "hasCOV" : false
                        },
                        {
                            "name" : "datapointNotInExchange",
                            "type" : "StateSupQTimeExt",
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
                        "datapoints" : [
                            { "name" : "datapoint1" },
                            { "name" : "datapointReal" }
                        ]
                    },
                    {
                        "name" : "BLT_MZA_002_V1",
                        "icc" : "icc1",
                        "apTitle" : "1.1.1.999",
                        "aeQualifier" : 12,
                        "datapoints" : [
                            { "name" : "datapoint1" },
                            { "name" : "datapoint2" },
                            { "name" : "datapointReal" },
                            { "name" : "datapointRealQ" },
                            { "name" : "datapointRealQTime" },
                            { "name" : "datapointRealQTimeExt" },
                            { "name" : "datapointState" },
                            { "name" : "datapointStateQ" },
                            { "name" : "datapointStateQTime" },
                            { "name" : "datapointStateQTimeExt" },
                            { "name" : "datapointDiscrete" },
                            { "name" : "datapointDiscreteQ" },
                            { "name" : "datapointDiscreteQTime" },
                            { "name" : "datapointDiscreteQTimeExt" },
                            { "name" : "datapointStateSup" },
                            { "name" : "datapointStateSupQ" },
                            { "name" : "datapointStateSupQTime" },
                            { "name" : "datapointStateSupQTimeExt" },
                            { "name" : "datapointNotInExchange" }
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
                            [ { "name" : "tase2", "ref" : "icc1:datapoint2" } ]
                    },
                    {
                        "pivot_id" : "TS3",
                        "label" : "TS3",
                        "protocols" : [
                            { "name" : "tase2", "ref" : "icc1:datapointReal" }
                        ]
                    },
                    {
                        "pivot_id" : "TS4",
                        "label" : "TS4",
                        "protocols" : [
                            { "name" : "tase2", "ref" : "icc1:datapointRealQ" }
                        ]
                    },
                    {
                        "pivot_id" : "TS5",
                        "label" : "TS5",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointRealQTime"
                        } ]
                    },
                    {
                        "pivot_id" : "TS6",
                        "label" : "TS6",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointRealQTimeExt"
                        } ]
                    },
                    {
                        "pivot_id" : "TS7",
                        "label" : "TS7",
                        "protocols" : [
                            { "name" : "tase2", "ref" : "icc1:datapointState" }
                        ]
                    },
                    {
                        "pivot_id" : "TS8",
                        "label" : "TS8",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateQ"
                        } ]
                    },
                    {
                        "pivot_id" : "TS9",
                        "label" : "TS9",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateQTime"
                        } ]
                    },
                    {
                        "pivot_id" : "TS10",
                        "label" : "TS10",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateQTimeExt"
                        } ]
                    },
                    {
                        "pivot_id" : "TS11",
                        "label" : "TS11",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointDiscrete"
                        } ]
                    },
                    {
                        "pivot_id" : "TS12",
                        "label" : "TS12",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointDiscreteQ"
                        } ]
                    },
                    {
                        "pivot_id" : "TS13",
                        "label" : "TS13",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointDiscreteQTime"
                        } ]
                    },
                    {
                        "pivot_id" : "TS14",
                        "label" : "TS14",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointDiscreteQTimeExt"
                        } ]
                    },
                    {
                        "pivot_id" : "TS15",
                        "label" : "TS15",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateSup"
                        } ]
                    },
                    {
                        "pivot_id" : "TS16",
                        "label" : "TS16",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateSupQ"
                        } ]
                    },
                    {
                        "pivot_id" : "TS17",
                        "label" : "TS17",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateSupQTime"
                        } ]
                    },
                    {
                        "pivot_id" : "TS18",
                        "label" : "TS18",
                        "protocols" : [ {
                            "name" : "tase2",
                            "ref" : "icc1:datapointStateSupQTimeExt"
                        } ]
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
    PLUGIN_HANDLE handle;
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
        plugin_shutdown (handle);
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

    template <class T>
    static Datapoint*
    createDatapoint (const std::string& dataname, const T value)
    {
        DatapointValue dp_value = DatapointValue (value);
        return new Datapoint (dataname, dp_value);
    }

    static Datapoint*
    createFakeDataObject (const char* type, const char* domain,
                          const char* name, const char* iv, const char* cs,
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
            datapoints->push_back (createDatapoint ("do_value", "wrong"));
            break;
        }
        case REAL:
        case REALQ:
        case REALQTIME:
        case REALQTIMEEXT: {
            datapoints->push_back (createDatapoint ("do_value", "wrong"));
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

    void
    setupTest (ConfigCategory& config, PLUGIN_HANDLE& handle,
               Tase2_Client& client)
    {
        config = ConfigCategory ("tase2Config", default_config);
        handle = plugin_init (&config);
        plugin_start (handle, "");
        Thread_sleep (500);
        client = Tase2_Client_create (nullptr);
        Tase2_Client_setLocalApTitle (client, "1.1.1.998", 12);
        Tase2_Client_setRemoteApTitle (client, "1.1.1.999", 12);
        Tase2_Client_setTcpPort (client, TCP_TEST_PORT);
    }

    void
    executeTestWrongDataType (Tase2_Client& client, PLUGIN_HANDLE& handle,
                              const char* type, const char* name,
                              const char* label, std::string expectedValue)
    {
        auto* dataobjects = new vector<Datapoint*>;
        dataobjects->push_back (
            createFakeDataObject (type, "icc1", name, "valid", "telemetered",
                                  "normal", (uint64_t)123456, "valid"));
        auto* reading = new Reading (std::string (label), *dataobjects);
        vector<Reading*> readings;
        readings.push_back (reading);
        plugin_send (handle, readings);
        delete dataobjects;
        delete reading;
        readings.clear ();
    }

    template <class T>
    void
    executeTest (Tase2_Client& client, PLUGIN_HANDLE& handle, const char* type,
                 const char* name, const char* label, const char* validity,
                 const char* cs, const char* nv, T expectedValue,
                 bool expectedSuccess)
    {
        Tase2_ClientError err
            = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
        ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

        auto* dataobjects = new vector<Datapoint*>;
        dataobjects->push_back (
            createDataObject (type, "icc1", name, expectedValue, validity, cs,
                              nv, (uint64_t)123456, "valid"));
        auto* reading = new Reading (std::string (label), *dataobjects);
        vector<Reading*> readings;
        readings.push_back (reading);
        plugin_send (handle, readings);
        delete dataobjects;
        delete reading;
        readings.clear ();
        Thread_sleep (50);

        Tase2_PointValue pv
            = Tase2_Client_readPointValue (client, &err, "icc1", name);
        ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

        Tase2_DataFlags flags = TASE2_DATA_FLAGS_VALIDITY_VALID
                                | TASE2_DATA_FLAGS_CURRENT_SOURCE_TELEMETERED
                                | TASE2_DATA_FLAGS_NORMAL_VALUE;

        DPTYPE dpType = TASE2Datapoint::getDpTypeFromString (type);

        switch (dpType)
        {
        case STATE:
        case STATEQ:
        case STATEQTIME:
        case STATEQTIMEEXT: {
            Tase2_DataState state = Tase2_PointValue_getValueState (pv);
            ASSERT_EQ (state,
                       (int)expectedValue | (dpType == STATE ? 0 : flags));
            break;
        }
        case DISCRETE:
        case DISCRETEQ:
        case DISCRETEQTIME:
        case DISCRETEQTIMEEXT: {
            int value = Tase2_PointValue_getValueDiscrete (pv);
            ASSERT_EQ (value, (int)expectedValue);
            break;
        }
        case REAL:
        case REALQ:
        case REALQTIME:
        case REALQTIMEEXT: {
            float value = Tase2_PointValue_getValueReal (pv);
            ASSERT_EQ (value, (float)expectedValue);
            break;
        }
        case STATESUP:
        case STATESUPQ:
        case STATESUPQTIME:
        case STATESUPQTIMEEXT: {
            Tase2_DataStateSupplemental stateSup
                = Tase2_PointValue_getValueStateSupplemental (pv);
            if (expectedSuccess)
            {
                ASSERT_EQ (stateSup, (int)expectedValue);
            }
            else
            {
                ASSERT_NE (stateSup, (int)expectedValue);
            }
            break;
        }
        default: {
            break;
        }
        }

        if (expectedSuccess)
        {
            Tase2_TimeStampClass tsClass
                = TASE2Datapoint::getTimeStampClass (dpType);

            if (tsClass == TASE2_TIMESTAMP_EXTENDED)
            {
                ASSERT_EQ (Tase2_PointValue_getTimeStamp (pv), 123456);
            }
            else if (tsClass == TASE2_TIMESTAMP)
            {
                ASSERT_EQ (Tase2_PointValue_getTimeStamp (pv), 123000);
            }
        }
        if (pv)
        {
            Tase2_PointValue_destroy (pv);
        }
        Tase2_Client_destroy (client);
    }
};

TEST_F (SendSpontDataTest, SendRealData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<float> (client, handle, "Real", "datapointReal",
                        "RealTestLabel", "valid", "telemetered", "normal",
                        123.45f, true);
}

TEST_F (SendSpontDataTest, SendRealQData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<float> (client, handle, "RealQ", "datapointRealQ",
                        "RealQTestLabel", "held", "entered", "normal", 234.56f,
                        true);
}

TEST_F (SendSpontDataTest, SendRealQTimeData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<float> (client, handle, "RealQTime", "datapointRealQTime",
                        "RealQTimeTestLabel", "suspect", "calculated",
                        "normal", 345.67f, true);
}

TEST_F (SendSpontDataTest, SendRealQTimeExtData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<float> (client, handle, "RealQTimeExt",
                        "datapointRealQTimeExt", "RealQTimeExtTestLabel",
                        "invalid", "estimated", "normal", 456.78f, true);
}

TEST_F (SendSpontDataTest, SendStateData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "State", "datapointState",
                      "StateTestLabel", "valid", "telemetered", "normal", 1,
                      true);
}

TEST_F (SendSpontDataTest, SendStateQData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateQ", "datapointStateQ",
                      "StateQTestLabel", "valid", "telemetered", "normal", 2,
                      true);
}

TEST_F (SendSpontDataTest, SendStateQTimeData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateQTime", "datapointStateQTime",
                      "StateQTimeTestLabel", "valid", "telemetered", "normal",
                      3, true);
}

TEST_F (SendSpontDataTest, SendStateQTimeExtData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateQTimeExt",
                      "datapointStateQTimeExt", "StateQTimeExtTestLabel",
                      "valid", "telemetered", "normal", 4, true);
}

TEST_F (SendSpontDataTest, SendDiscreteData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "Discrete", "datapointDiscrete",
                      "DiscreteTestLabel", "valid", "telemetered", "normal", 5,
                      true);
}

TEST_F (SendSpontDataTest, SendDiscreteQData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "DiscreteQ", "datapointDiscreteQ",
                      "DiscreteQTestLabel", "valid", "telemetered", "normal",
                      6, true);
}

TEST_F (SendSpontDataTest, SendDiscreteQTimeData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "DiscreteQTime",
                      "datapointDiscreteQTime", "DiscreteQTimeTestLabel",
                      "valid", "telemetered", "normal", 7, true);
}

TEST_F (SendSpontDataTest, SendDiscreteQTimeExtData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "DiscreteQTimeExt",
                      "datapointDiscreteQTimeExt", "DiscreteQTimeExtTestLabel",
                      "valid", "telemetered", "normal", 8, true);
}

TEST_F (SendSpontDataTest, SendStateSupData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateSup", "datapointStateSup",
                      "StateSupTestLabel", "valid", "telemetered", "normal", 9,
                      true);
}

TEST_F (SendSpontDataTest, SendStateSupQData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateSupQ", "datapointStateSupQ",
                      "StateSupQTestLabel", "valid", "telemetered", "normal",
                      10, true);
}

TEST_F (SendSpontDataTest, SendStateSupQTimeData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateSupQTime",
                      "datapointStateSupQTime", "StateSupQTimeTestLabel",
                      "valid", "telemetered", "normal", 11, true);
}

TEST_F (SendSpontDataTest, SendStateSupQTimeExtData)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateSupQTimeExt",
                      "datapointStateSupQTimeExt", "StateSupQTimeExtTestLabel",
                      "valid", "telemetered", "normal", 12, true);
}

TEST_F (SendSpontDataTest, InvalidDataTypes)
{
    ConfigCategory config;
    Tase2_Client client;

    std::vector<std::pair<std::string, std::string> > nameTypePairs
        = { { "datapointReal", "Real" },
            { "datapointRealQ", "RealQ" },
            { "datapointRealQTime", "RealQTime" },
            { "datapointRealQTimeExt", "RealQTimeExt" },
            { "datapointState", "State" },
            { "datapointStateQ", "StateQ" },
            { "datapointStateQTime", "StateQTime" },
            { "datapointStateQTimeExt", "StateQTimeExt" },
            { "datapointDiscrete", "Discrete" },
            { "datapointDiscreteQ", "DiscreteQ" },
            { "datapointDiscreteQTime", "DiscreteQTime" },
            { "datapointDiscreteQTimeExt", "DiscreteQTimeExt" },
            { "datapointStateSup", "StateSup" },
            { "datapointStateSupQ", "StateSupQ" },
            { "datapointStateSupQTime", "StateSupQTime" },
            { "datapointStateSupQTimeExt", "StateSupQTimeExt" } };

    setupTest (config, handle, client);

    Tase2_ClientError err
        = Tase2_Client_connect (client, "127.0.0.1", "1.1.1.999", 12);
    ASSERT_TRUE (err == TASE2_CLIENT_ERROR_OK);

    for (auto pair : nameTypePairs)
    {
        executeTestWrongDataType (client, handle, pair.second.c_str (),
                                  pair.first.c_str (), "",
                                  (std::string) "wrong");
    }

    Tase2_Client_destroy (client);
}

TEST_F (SendSpontDataTest, SendDataNotInExchange)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);
    executeTest<int> (client, handle, "StateSupQTimeExt",
                      "datapointNotInExchange", "Ex", "valid", "telemetered",
                      "normal", 12, false);
}