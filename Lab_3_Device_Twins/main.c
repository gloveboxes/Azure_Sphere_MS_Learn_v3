/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This example is built on the Azure Sphere DevX library.
 *   1. DevX is an Open Source community-maintained implementation of the Azure Sphere SDK samples.
 *   2. DevX is a modular library that simplifies common development scenarios.
 *        - You can focus on your solution, not the plumbing.
 *   3. DevX documentation is maintained at
 *https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki
 *	 4. The DevX library is not a substitute for understanding the Azure Sphere SDK Samples.
 *          - https://github.com/Azure/azure-sphere-samples
 *
 * DEVELOPER BOARD SELECTION
 *
 * The following developer boards are supported.
 *
 *	 1. AVNET Azure Sphere Starter Kit.
 *   2. AVNET Azure Sphere Starter Kit Revision 2.
 *	 3. Seeed Studio Azure Sphere MT3620 Development Kit aka Reference Design Board (RDB).
 *	 4. Seeed Studio Seeed Studio MT3620 Mini Dev Board.
 *
 * ENABLE YOUR DEVELOPER BOARD
 *
 * Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *    configuration that matches your board.
 *
 * Follow these steps:
 *
 *	 1. Open cmake/azsphere_board.cmake
 *	 2. Uncomment the set command that matches your developer board.
 *	 3. Click File, then Save to auto-generate the CMake Cache.
 *
 ************************************************************************************************/

#include "main.h"

/***********************************************************************************************************
 * HVAC sensor data
 *
 * Read HVAC environment sensor
 * Publish HVAC environment data
 * Update HVAC Operating mode and current environment data
 **********************************************************************************************************/

/// <summary>
/// Update temperature and pressure device twins
/// Only update if data changes to minimise costs
/// </summary>
/// <param name="temperature"></param>
/// <param name="pressure"></param>
static void update_device_twins(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (telemetry.valid && dx_isAzureConnected())
    {
        if (telemetry.previous.temperature != telemetry.latest.temperature)
        {
            telemetry.previous.temperature = telemetry.latest.temperature;
            // Update temperature device twin
            dx_deviceTwinReportValue(&dt_env_temperature, &telemetry.latest.temperature);
        }

        if (telemetry.previous.pressure != telemetry.latest.pressure)
        {
            telemetry.previous.pressure = telemetry.latest.pressure;
            // Update pressure device twin
            dx_deviceTwinReportValue(&dt_env_pressure, &telemetry.latest.pressure);
        }

        if (telemetry.previous.humidity != telemetry.latest.humidity)
        {
            telemetry.previous.humidity = telemetry.latest.humidity;
            // Update humidity device twin
            dx_deviceTwinReportValue(&dt_env_humidity, &telemetry.latest.humidity);
        }

        if (telemetry.latest_operating_mode != HVAC_MODE_UNKNOWN && telemetry.latest_operating_mode != telemetry.previous_operating_mode)
        {
            telemetry.previous_operating_mode = telemetry.latest_operating_mode;
            // Update operating mode device twin
            dx_deviceTwinReportValue(&dt_hvac_operating_mode, hvac_state[telemetry.latest_operating_mode]);
        }
    }
}

/// <summary>
/// Set the temperature status led.
/// Red to turn on heater to reach desired temperature.
/// Blue to turn on cooler to reach desired temperature
/// Green equals just right, no action required.
/// </summary>
void set_hvac_operating_mode(void)
{
    if (dt_hvac_target_temperature.propertyUpdated && telemetry.updated)
    {
        int target_temperature = *(int *)dt_hvac_target_temperature.propertyValue;

        telemetry.latest_operating_mode = telemetry.latest.temperature == target_temperature  ? HVAC_MODE_GREEN
                                          : telemetry.latest.temperature > target_temperature ? HVAC_MODE_COOLING
                                                                                              : HVAC_MODE_HEATING;

        if (telemetry.previous_operating_mode != telemetry.latest_operating_mode)
        {
            // minus one as first item is HVAC_MODE_UNKNOWN
            if (telemetry.previous_operating_mode != HVAC_MODE_UNKNOWN)
            {
                dx_gpioOff(gpio_ledRgb[telemetry.previous_operating_mode - 1]);
            }
            telemetry.previous_operating_mode = telemetry.latest_operating_mode;
        }

        // minus one as first item is HVAC_MODE_UNKNOWN
        dx_gpioOn(gpio_ledRgb[telemetry.latest_operating_mode - 1]);
    }
}

/// <summary>
/// Publish HVAC telemetry
/// </summary>
/// <param name="eventLoopTimer"></param>
static void publish_telemetry_handler(EventLoopTimer *eventLoopTimer)
{
    static int msgId = 0;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (telemetry.valid && dx_isAzureConnected())
    {
        // clang-format off
		// Serialize telemetry as JSON
		if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 6,
			DX_JSON_INT, "MsgId", msgId++,
			DX_JSON_INT, "Temperature", telemetry.latest.temperature,
			DX_JSON_INT, "Pressure", telemetry.latest.pressure,
			DX_JSON_INT, "Humidity", telemetry.latest.humidity,
			DX_JSON_INT, "PeakUserMemoryKiB", (int)Applications_GetPeakUserModeMemoryUsageInKB(),
			DX_JSON_INT, "TotalMemoryKiB", (int)Applications_GetTotalMemoryUsageInKB()))
        // clang-format on
        {
            Log_Debug("%s\n", msgBuffer);

            // Publish telemetry message to IoT Hub/Central
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
        }
        else
        {
            Log_Debug("JSON Serialization failed: Buffer too small\n");
            dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
        }
    }
}

/// <summary>
/// read_telemetry_handler callback handler called every 4 seconds
/// Environment sensors read and HVAC operating mode LED updated
/// </summary>
/// <param name="eventLoopTimer"></param>
static void read_telemetry_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    onboard_sensors_read(&telemetry.latest);

    telemetry.updated = true;

    // clang-format off
	telemetry.valid = IN_RANGE(telemetry.latest.temperature, -20, 50) &&
		IN_RANGE(telemetry.latest.pressure, 800, 1200) &&
		IN_RANGE(telemetry.latest.humidity, 0, 100);
    // clang-format on

    // Set the HVAC Operating mode color
    set_hvac_operating_mode();
}

/***********************************************************************************************************
 * REMOTE OPERATIONS: DEVICE TWINS
 *
 * Set target HVAC temperature
 * Set HVAC panel message
 **********************************************************************************************************/

/// <summary>
/// dt_set_target_temperature_handler callback handler is called when TargetTemperature device twin
/// message received HVAC operating mode LED updated and IoT Plug and Play device twin acknowledged
/// </summary>
/// <param name="deviceTwinBinding"></param>
static void dt_set_target_temperature_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 50))
    {
        // Set the HVAC Operating mode color
        set_hvac_operating_mode();
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
    else
    {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

// This device twin callback demonstrates how to manage device twins of type string.
// A reference to the string is passed that is available only for the lifetime of the callback.
// You must copy to a global char array to preserve the string outside of the callback.
// As strings are arbitrary length on a constrained device this gives you, the developer, control of
// memory allocation.
static void dt_set_panel_message_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    char *panel_message = (char *)deviceTwinBinding->propertyValue;

    // Is the message size less than the destination buffer size and printable characters
    if (strlen(panel_message) < sizeof(display_panel_message) && dx_isStringPrintable(panel_message))
    {
        strncpy(display_panel_message, panel_message, sizeof(display_panel_message));
        Log_Debug("Virtual HVAC Display Panel Message: %s\n", display_panel_message);
        // IoT Plug and Play acknowledge completed
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
    else
    {
        Log_Debug("Local copy failed. String too long or invalid data\n");
        // IoT Plug and Play acknowledge error
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

/***********************************************************************************************************
 * PRODUCTION
 *
 * Set Azure connection state LED
 **********************************************************************************************************/

/// <summary>
/// ConnectionStatus callback handler is called when the connection status changes
/// </summary>
/// <param name="connected"></param>
static void connection_status(bool connected)
{
    dx_gpioStateSet(&gpio_network_led, connected);
}

/***********************************************************************************************************
 * APPLICATION BASICS
 *
 * Initialize resources
 * Close resources
 * Run the main event loop
 **********************************************************************************************************/

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_binding_sets.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_gpioSetOpen(gpio_binding_sets, NELEMS(gpio_binding_sets));
    dx_gpioSetOpen(gpio_ledRgb, NELEMS(gpio_ledRgb));
    dx_timerSetStart(timer_binding_sets, NELEMS(timer_binding_sets));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_binding_sets, NELEMS(direct_method_binding_sets));

    dx_azureRegisterConnectionChangedNotification(connection_status);

    // initialize previous environment sensor variables
    telemetry.previous.temperature = telemetry.previous.pressure = telemetry.previous.humidity = INT32_MAX;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_binding_sets, NELEMS(timer_binding_sets));
    dx_deviceTwinUnsubscribe();
    dx_directMethodUnsubscribe();
    dx_gpioSetClose(gpio_binding_sets, NELEMS(gpio_binding_sets));
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();

    if (!dx_configParseCmdLineArguments(argc, argv, &dx_config))
    {
        return dx_getTerminationExitCode();
    }

    InitPeripheralsAndHandlers();

    // Main loop
    while (!dx_isTerminationRequired())
    {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR)
        {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}