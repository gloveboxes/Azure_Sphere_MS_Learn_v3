/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_deferred_update.h"
#include "dx_intercore.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_version.h"

#include "hw/azure_sphere_learning_path.h" // Hardware definition
#include "app_exit_codes.h"                // application specific exit codes
#include "onboard_sensors.h"

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
static DX_DIRECT_METHOD_RESPONSE_CODE RestartDeviceHandler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static void DelayRestartDeviceTimerHandler(EventLoopTimer *eventLoopTimer);
static void dt_set_panel_message_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_set_target_temperature_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void publish_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void read_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void update_device_twins(EventLoopTimer *eventLoopTimer);

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};
static char display_panel_message[64];
DX_USER_CONFIG dx_config;
static char *hvac_state[] = {"Unknown", "Heating", "Green", "Cooling", "On", "Off"};

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
static DX_DEVICE_TWIN_BINDING dt_env_humidity = {.propertyName = "Humidity", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_env_pressure = {.propertyName = "Pressure", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_env_temperature = {.propertyName = "Temperature", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_hvac_operating_mode = {.propertyName = "OperatingMode", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_hvac_panel_message = {
    .propertyName = "PanelMessage", .twinType = DX_DEVICE_TWIN_STRING, .handler = dt_set_panel_message_handler};
static DX_DEVICE_TWIN_BINDING dt_hvac_sw_version = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_hvac_target_temperature = {
    .propertyName = "TargetTemperature", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_set_target_temperature_handler};
static DX_DEVICE_TWIN_BINDING dt_utc_startup = {.propertyName = "StartupUtc", .twinType = DX_DEVICE_TWIN_STRING};

// declare gpio bindings
static DX_GPIO_BINDING gpio_operating_led = {
    .pin = LED2, .name = "gpio_operating_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING gpio_network_led = {
    .pin = NETWORK_CONNECTED_LED, .name = "network_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};

// Create an RGB LED gpio binding set
static DX_GPIO_BINDING *gpio_ledRgb[] = {
    &(DX_GPIO_BINDING){.pin = LED_RED, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "red led"},
    &(DX_GPIO_BINDING){.pin = LED_GREEN, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "green led"},
    &(DX_GPIO_BINDING){.pin = LED_BLUE, .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true, .name = "blue led"}};

// declare timer bindings
static DX_TIMER_BINDING tmr_read_telemetry = {.period = {4, 0}, .name = "tmr_read_telemetry", .handler = read_telemetry_handler};
static DX_TIMER_BINDING tmr_publish_telemetry = {.period = {5, 0}, .name = "tmr_publish_telemetry", .handler = publish_telemetry_handler};
static DX_TIMER_BINDING tmr_update_device_twins = {.period = {15, 0}, .name = "tmr_update_device_twins", .handler = update_device_twins};
static DX_TIMER_BINDING restart_device_oneshot_timer = {.name = "restart_device_oneshot_timer", .handler = DelayRestartDeviceTimerHandler};

// Declare direct method bindings
static DX_DIRECT_METHOD_BINDING dm_hvac_off = {.methodName = "HvacOff", .handler = hvac_off_handler};
static DX_DIRECT_METHOD_BINDING dm_hvac_on = {.methodName = "HvacOn", .handler = hvac_on_handler};
static DX_DIRECT_METHOD_BINDING dm_restart_device = {.methodName = "RestartDevice", .handler = RestartDeviceHandler};

// All bindings referenced in the following binding sets are initialised in the
// InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_utc_startup,  &dt_hvac_sw_version,    &dt_env_temperature,     &dt_env_pressure,
                                                  &dt_env_humidity, &dt_hvac_panel_message, &dt_hvac_operating_mode, &dt_hvac_target_temperature};

DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_restart_device, &dm_hvac_on, &dm_hvac_off};
DX_GPIO_BINDING *gpio_bindings[] = {&gpio_network_led, &gpio_operating_led};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_publish_telemetry, &tmr_read_telemetry, &tmr_update_device_twins, &restart_device_oneshot_timer};
