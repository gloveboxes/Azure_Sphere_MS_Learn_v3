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
 *	 1. Open azsphere_board.txt
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
 **********************************************************************************************************/

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

    if (telemetry.valid && azure_connected)
    {
        // clang-format off
        // Serialize telemetry as JSON
        if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 6,
            DX_JSON_INT, "msgId", msgId++,
            DX_JSON_INT, "temperature", telemetry.latest.temperature,
            DX_JSON_INT, "pressure", telemetry.latest.pressure,
            DX_JSON_INT, "humidity", telemetry.latest.humidity,
            DX_JSON_INT, "peakUserMemoryKiB", (int)Applications_GetPeakUserModeMemoryUsageInKB(),
            DX_JSON_INT, "totalMemoryKiB", (int)Applications_GetTotalMemoryUsageInKB()))
        // clang-format on
        {
            dx_Log_Debug("%s\n", msgBuffer);

            // Publish telemetry message to IoT Hub/Central
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
        }
        else
        {
            dx_Log_Debug("JSON Serialization failed: Buffer too small\n");
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
    hvac_sensors_read(&telemetry.latest);

    telemetry.updated = true;

    // clang-format off
    telemetry.valid =
        IN_RANGE(telemetry.latest.temperature, -20, 50) &&
        IN_RANGE(telemetry.latest.pressure, 800, 1200) &&
        IN_RANGE(telemetry.latest.humidity, 0, 100);
    // clang-format on
}

/***********************************************************************************************************
 * APPLICATION BASICS
 *
 * Set Azure connection state
 * Initialize resources
 * Close resources
 * Run the main event loop
 **********************************************************************************************************/

/// <summary>
/// Update local azure_connected with new connection status
/// </summary>
/// <param name="connected"></param>
void azure_connection_state(bool connected)
{
    azure_connected = connected;
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    hvac_sensors_init();
    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_i2cSetOpen(i2c_bindings, NELEMS(i2c_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_azureRegisterConnectionChangedNotification(azure_connection_state);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
    dx_i2cSetClose(i2c_bindings, NELEMS(i2c_bindings));
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
    dx_Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}