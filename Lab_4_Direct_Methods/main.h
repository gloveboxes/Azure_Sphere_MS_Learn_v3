/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_deferred_update.h"
#include "dx_gpio.h"
#include "dx_intercore.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_version.h"

#include "hw/azure_sphere_learning_path.h" // Hardware definition
#include "app_exit_codes.h"                // application specific exit codes
#include "hvac_sensors.h"
#include "hvac_status.h"

#include <applibs/applications.h>
#include <applibs/log.h>
#include <applibs/powermanagement.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"
#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.01"

// Forward declarations
static DX_DIRECT_METHOD_RESPONSE_CODE hvac_off_handler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static DX_DIRECT_METHOD_RESPONSE_CODE hvac_on_handler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static DX_DIRECT_METHOD_RESPONSE_CODE hvac_restart_handler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static void hvac_delay_restart_handler(EventLoopTimer *eventLoopTimer);
static void publish_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void read_telemetry_handler(EventLoopTimer *eventLoopTimer);
void azure_status_led_off_handler(EventLoopTimer *eventLoopTimer);
void azure_status_led_on_handler(EventLoopTimer *eventLoopTimer);

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};
DX_USER_CONFIG dx_config;
bool azure_connected = false;
ENVIRONMENT telemetry;

#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
static char Log_Debug_Time_buffer[128];

/// <summary>
/// Publish sensor telemetry using the following properties for efficient IoT Hub routing
/// https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-d2c
/// </summary>
static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

/// <summary>
/// Common content properties for publish messages to IoT Hub/Central
/// </summary>
static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// declare device twin bindings
static DX_DEVICE_TWIN_BINDING dt_hvac_sw_version = {.propertyName = "HvacSoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_hvac_start_utc = {.propertyName = "HvacStartUtc", .twinType = DX_DEVICE_TWIN_STRING};

// declare gpio bindings
static DX_GPIO_BINDING gpio_operating_led = {.pin = LED2, .name = "hvac_operating_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};
DX_GPIO_BINDING gpio_network_led = {.pin = NETWORK_CONNECTED_LED, .name = "network_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};

// Create an RGB LED gpio binding set
static DX_GPIO_BINDING *gpio_ledRgb[] = {
    &(DX_GPIO_BINDING){.pin = LED_RED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "red led"},
    &(DX_GPIO_BINDING){.pin = LED_GREEN, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "green led"},
    &(DX_GPIO_BINDING){.pin = LED_BLUE, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "blue led"}};

// declare timer bindings
DX_TIMER_BINDING tmr_azure_status_led_off = {.name = "tmr_azure_status_led_off", .handler = azure_status_led_off_handler};
DX_TIMER_BINDING tmr_azure_status_led_on = {.period = {0, 500 * ONE_MS}, .name = "tmr_azure_status_led_on", .handler = azure_status_led_on_handler};
static DX_TIMER_BINDING tmr_hvac_restart_oneshot_timer = {.name = "tmr_hvac_restart_oneshot_timer", .handler = hvac_delay_restart_handler};
static DX_TIMER_BINDING tmr_publish_telemetry = {.period = {5, 0}, .name = "tmr_publish_telemetry", .handler = publish_telemetry_handler};
static DX_TIMER_BINDING tmr_read_telemetry = {.period = {4, 0}, .name = "tmr_read_telemetry", .handler = read_telemetry_handler};

// Declare direct method bindings
static DX_DIRECT_METHOD_BINDING dm_hvac_off = {.methodName = "HvacOff", .handler = hvac_off_handler, .context = &gpio_operating_led};
static DX_DIRECT_METHOD_BINDING dm_hvac_on = {.methodName = "HvacOn", .handler = hvac_on_handler, .context = &gpio_operating_led};
static DX_DIRECT_METHOD_BINDING dm_hvac_restart = {.methodName = "HvacRestart", .handler = hvac_restart_handler};

// All bindings referenced in the following binding sets are initialised in the
// InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_hvac_sw_version, &dt_hvac_start_utc};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_hvac_restart, &dm_hvac_on, &dm_hvac_off};
DX_GPIO_BINDING *gpio_bindings[] = {&gpio_network_led, &gpio_operating_led};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_publish_telemetry, &tmr_read_telemetry, &tmr_hvac_restart_oneshot_timer, &tmr_azure_status_led_off,
                                      &tmr_azure_status_led_on};
