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
                            { "name" : "datapointStateSupQTimeExt" }
                        ]
                    }
                ],
                "dataset_transfer_sets" :
                    [ { "domain" : "icc1", "name" : "DSTrans1" } ],
                "datasets" : [ {
                    "name" : "ds1",
                    "domain" : "icc1",
                    "datapoints" :
                        [ "datapointReal", "datapointState" ]
                } ]
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

static int dataSetCreated = 0;
static int dsValueHandlerCalled = 0;
static Tase2_PointValue lastPointValue = nullptr;

// Class to be called in each test, contains fixture to be used in
class DatasetTest : public testing::Test
{
  protected:
    PLUGIN_HANDLE handle;
    // Setup is ran for every tests, so each variable are reinitialised
    void
    SetUp () override
    {
        dataSetCreated = 0;
        dsValueHandlerCalled = 0;
        lastPointValue = nullptr;
    }

    // TearDown is ran for every tests, so each variable are destroyed
    // again
    void
    TearDown () override
    {
        if (lastPointValue)
        {
            Tase2_PointValue_destroy (lastPointValue);
        }
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
        dsValueHandlerCalled++;
        lastPointValue = Tase2_PointValue_getCopy (pointValue);

        printf ("  Received value for %s:%s \n", domainName, pointName);
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

    static void
    dataSetEventHandler (void* parameter, bool create,
                         Tase2_Endpoint_Connection peer,
                         Tase2_BilateralTable clientBlt,
                         Tase2_Domain dataSetDomain, char* dataSetName,
                         LinkedList dataPoints)
    {
        if (create)
        {
            printf ("Created new data set %s:%s with data points\n",
                    dataSetDomain ? dataSetDomain->name : "(VCC)",
                    dataSetName);

            dataSetCreated++;

            if (peer)
            {
                printf ("  client: IP: %s ap-title: %s ae-qualifier: %i\n",
                        Tase2_Endpoint_Connection_getPeerIpAddress (peer),
                        Tase2_Endpoint_Connection_getPeerApTitle (peer),
                        Tase2_Endpoint_Connection_getPeerAeQualifier (peer));
            }

            if (clientBlt)
            {
                printf ("  BLT: %s\n", Tase2_BilateralTable_getID (clientBlt));
            }

            LinkedList dataPointElem = LinkedList_getNext (dataPoints);

            while (dataPointElem)
            {
                Tase2_DataPoint dataPoint
                    = (Tase2_DataPoint)LinkedList_getData (dataPointElem);

                printf ("  data point: %s:%s\n",
                        Tase2_DataPoint_getDomain (dataPoint)->name
                            ? Tase2_DataPoint_getDomain (dataPoint)->name
                            : "[VCC]",
                        Tase2_DataPoint_getName (dataPoint));

                dataPointElem = LinkedList_getNext (dataPointElem);
            }
        }
    }

    static void
    dsTransferSetUpdateHandler (void* parameter,
                                Tase2_Endpoint_Connection peer,
                                Tase2_BilateralTable clientBlt,
                                Tase2_DSTransferSet transferSet,
                                bool isEnabled)
    {
        printf ("DS transfer set %s is updated\n",
                Tase2_TransferSet_getName ((Tase2_TransferSet)transferSet));

        if (peer)
        {
            printf ("  --> client: IP: %s ap-title: %s ae-qualifier: %i\n",
                    Tase2_Endpoint_Connection_getPeerIpAddress (peer),
                    Tase2_Endpoint_Connection_getPeerApTitle (peer),
                    Tase2_Endpoint_Connection_getPeerAeQualifier (peer));
        }

        if (clientBlt)
        {
            printf ("  --> BLT: %s\n", Tase2_BilateralTable_getID (clientBlt));
        }

        printf ("  Status: %s\n", Tase2_DSTransferSet_getStatus (transferSet)
                                      ? "enabled"
                                      : "disabled");
    }

    static void
    dsTransferSetSentHandler (void* parameter, Tase2_Endpoint_Connection peer,
                              Tase2_BilateralTable clientBlt,
                              Tase2_DSTransferSet ts, LinkedList sentValues,
                              Tase2_ReportReason reason)
    {
        printf ("DS Transfer set %s (reason: %i) was sent with the following "
                "values:\n",
                Tase2_TransferSet_getName ((Tase2_TransferSet)ts), reason);

        LinkedList sentValueElem = LinkedList_getNext (sentValues);

        while (sentValueElem)
        {
            Tase2_SentPointValue sentValue
                = (Tase2_SentPointValue)LinkedList_getData (sentValueElem);

            Tase2_DataPoint sentDataPoint
                = Tase2_SentPointValue_getDataPoint (sentValue);
            Tase2_PointValue pointValue
                = Tase2_SentPointValue_getPointValue (sentValue);

            printf ("  data point: %s \n",
                    Tase2_DataPoint_getName (sentDataPoint));

            sentValueElem = LinkedList_getNext (sentValueElem);
        }
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
        Tase2_Client_installDSTransferSetValueHandler (
            client, dsTransferSetValueHandler, NULL);
        Tase2_Client_installDSTransferSetReportHandler (
            client, dsTransferSetReportHandler, NULL);
        Tase2_Client_connect (client, "localhost", "1.1.1.999", 12);
    }
};

TEST_F (DatasetTest, CreateDatasetAndUpdate)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err;

    Tase2_ClientDataSet dataSet
        = Tase2_Client_getDataSet (client, &err, "icc1", "ds1");

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (Tase2_ClientDataSet_getSize (dataSet), 2);
    ASSERT_EQ (
        strcmp (Tase2_ClientDataSet_getPointName (dataSet, 0)->pointName,
                "datapointReal"),
        0);
    ASSERT_EQ (
        strcmp (Tase2_ClientDataSet_getPointName (dataSet, 1)->pointName,
                "datapointState"),
        0);

    Tase2_ClientDataSet_destroy (dataSet);

    Tase2_Client_destroy (client);
}

TEST_F (DatasetTest, ActivateDataTransferSet)
{
    ConfigCategory config;
    Tase2_Client client;

    setupTest (config, handle, client);

    Tase2_ClientError err;

    Tase2_ClientDataSet dataSet
        = Tase2_Client_getDataSet (client, &err, "icc1", "ds1");

    ASSERT_EQ (err, TASE2_CLIENT_ERROR_OK);

    Tase2_ClientDSTransferSet dsts
        = Tase2_Client_getNextDSTransferSet (client, "icc1", &err);

    ASSERT_EQ (Tase2_ClientDataSet_read (dataSet, client),
               TASE2_CLIENT_ERROR_OK);

    Tase2_ClientDSTransferSet_setDataSet (dsts, dataSet);
    Tase2_ClientDSTransferSet_setDataSetName (dsts, "icc1", "ds1");
    Tase2_ClientDSTransferSet_setInterval (dsts, 5);
    Tase2_ClientDSTransferSet_setTLE (dsts, 60);
    Tase2_ClientDSTransferSet_setBufferTime (dsts, 2);
    Tase2_ClientDSTransferSet_setIntegrityCheck (dsts, 30);
    Tase2_ClientDSTransferSet_setRBE (dsts, true);
    Tase2_ClientDSTransferSet_setCritical (dsts, false);
    Tase2_ClientDSTransferSet_setDSConditionsRequested (
        dsts, TASE2_DS_CONDITION_INTERVAL | TASE2_DS_CONDITION_CHANGE);
    Tase2_ClientDSTransferSet_setStatus (dsts, true);

    printf ("Start DSTransferSet\n");
    ASSERT_EQ (Tase2_ClientDSTransferSet_writeValues (dsts, client),
               TASE2_CLIENT_ERROR_OK);

    ASSERT_EQ (Tase2_ClientDSTransferSet_readValues (dsts, client),
               TASE2_CLIENT_ERROR_OK);

    auto* dataobjects = new vector<Datapoint*>;
    dataobjects->push_back (
        createDataObject ("Real", "icc1", "datapointReal", 1.2f, "valid",
                          "telemetered", "normal", (uint64_t)123456, "valid"));
    auto* reading = new Reading (std::string ("TS3"), *dataobjects);
    vector<Reading*> readings;
    readings.push_back (reading);
    plugin_send (handle, readings);
    delete dataobjects;
    delete reading;

    auto start = std::chrono::high_resolution_clock::now ();
    auto timeout = std::chrono::seconds (10);
    while (dsValueHandlerCalled <= 0)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            FAIL () << "Connection not established within timeout";
            Tase2_ClientDataSet_destroy (dataSet);
            Tase2_ClientDSTransferSet_destroy (dsts);
            Tase2_Client_destroy (client);

            break;
        }
        Thread_sleep (10);
    }

    ASSERT_NEAR (Tase2_PointValue_getValueReal (lastPointValue), 1.2, 0.00001);

    Tase2_ClientDataSet_destroy (dataSet);

    Tase2_ClientDSTransferSet_destroy (dsts);

    Tase2_Client_destroy (client);
}
