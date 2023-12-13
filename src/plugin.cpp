/*
 * Copyright 2023 MZ Automation GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <config_category.h>
#include <iostream>
#include <libtase2/hal_thread.h>
#include <logger.h>
#include <plugin_api.h>
#include <plugin_exception.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <tase2.hpp>
#include <version.h>

using namespace std;
using namespace rapidjson;

extern "C"
{

#define PLUGIN_NAME "tase2"
    /**
     * Plugin specific default configuration
     */
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
                        "port" : 102,
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
                            "protocols" : [
                                { "name" : "tase2", "ref" : "icc1:datapoint1" }
                            ]
                        },
                        {
                            "pivot_id" : "TS2",
                            "label" : "TS2",
                            "protocols" : [
                                { "name" : "tase2", "ref" : "icc1:datapoint2" }
                            ]
                        },
                        {
                            "pivot_id" : "TS3",
                            "label" : "TS3",
                            "protocols" : [ {
                                "name" : "tase2",
                                "ref" : "icc1:datapointReal"
                            } ]
                        },
                        {
                            "pivot_id" : "TS4",
                            "label" : "TS4",
                            "protocols" : [ {
                                "name" : "tase2",
                                "ref" : "icc1:datapointRealQ"
                            } ]
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
                            "protocols" : [ {
                                "name" : "tase2",
                                "ref" : "icc1:datapointState"
                            } ]
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
    /**
     * The TASE2 North Plugin plugin interface
     */

    /**
     * The C API plugin information structure
     */
    static PLUGIN_INFORMATION info = {
        PLUGIN_NAME,       // Name
        VERSION,           // Version
        SP_CONTROL,        // Flags
        PLUGIN_TYPE_NORTH, // Type
        "0.0.1",           // Interface version
        default_config     // Configuration
    };

    /**
     * Return the information about this plugin
     */
    PLUGIN_INFORMATION*
    plugin_info ()
    {
        return &info;
    }

    /**
     * Initialise the plugin with configuration.
     *
     * This function is called to get the plugin handle.
     */
    PLUGIN_HANDLE
    plugin_init (ConfigCategory* configData)
    {
        Tase2Utility::log_info ("Initializing the plugin");

        TASE2Server* tase2 = new TASE2Server ();

        if (tase2)
        {
            tase2->configure (configData);
        }

        return (PLUGIN_HANDLE)tase2;
    }

    /**
     * Send Readings data to historian server
     */
    uint32_t
    plugin_send (const PLUGIN_HANDLE handle, const vector<Reading*>& readings)
    {
        TASE2Server* tase2 = (TASE2Server*)handle;

        return tase2->send (readings);
    }

    void
    plugin_register (PLUGIN_HANDLE handle,
                     bool (*write) (const char* name, const char* value,
                                    ControlDestination destination, ...),
                     int (*operation) (char* operation, int paramCount,
                                       char* names[], char* parameters[],
                                       ControlDestination destination, ...))
    {
        Tase2Utility::log_info ("plugin_register");

        TASE2Server* tase2 = (TASE2Server*)handle;

        tase2->registerControl (operation);
    }

    /**
     * Plugin start with stored plugin_data
     *
     * @param handle	The plugin handle
     * @param storedData	The stored plugin_data
     */
    void
    plugin_start (const PLUGIN_HANDLE handle, const string& storedData)
    {
        Tase2Utility::log_warn ("Plugin start called");
        TASE2Server* tase2 = (TASE2Server*)handle;
        if (tase2)
        {
            tase2->start ();
        }
    }

    /**
     * Shutdown the plugin
     *
     * Delete allocated data
     *
     * @param handle    The plugin handle
     */
    void
    plugin_shutdown (PLUGIN_HANDLE handle)
    {
        TASE2Server* tase2 = (TASE2Server*)handle;

        delete tase2;
    }
}
