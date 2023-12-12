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
                    "name" : "tase2scheduler",
                    "version" : "1.0",
                    "transport_layer" : { "srv_ip" : "0.0.0.0", "port" : 103 }
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
                            "datapoints" :
                                [ { "name" : "datapoint2" } ]
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
                            "protocols" : [
                                { "name" : "tase2", "ref" : "icc1:datapoint1" }
                            ]
                        },
                        {
                            "pivot_id" : "TS2",
                            "label" : "TS2",
                            "protocols" : [
                                { "name" : "tase", "ref" : "icc1:datapoint2" }
                            ]
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
