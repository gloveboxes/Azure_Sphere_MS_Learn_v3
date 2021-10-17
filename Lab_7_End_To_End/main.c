/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This example is built on the Azure Sphere DevX library.
 *   1. DevX is an Open Source community-maintained implementation of the Azure Sphere SDK samples.
 *   2. DevX is a modular library that simplifies common development scenarios.
 *        - You can focus on your solution, not the plumbing.
 *   3. DevX documentation is maintained at https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki
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
  * Update HVAC Operating mode and current environment data
  **********************************************************************************************************/

  /// <summary>
  /// Determine if telemetry value changed. If so, update it's device twin
  /// </summary>
  /// <param name="new_value"></param>
  /// <param name="previous_value"></param>
  /// <param name="device_twin"></param>
static void device_twin_update(int* latest_value, int* previous_value, DX_DEVICE_TWIN_BINDING* device_twin)
{
	if (*latest_value != *previous_value)
	{
		*previous_value = *latest_value;
		dx_deviceTwinReportValue(device_twin, latest_value);
	}
}

/// <summary>
/// Only update device twins if data changed to minimize network and cloud costs
/// </summary>
/// <param name="temperature"></param>
/// <param name="pressure"></param>
static void update_device_twins(EventLoopTimer* eventLoopTimer)
{
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
	{
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	if (telemetry.valid && azure_connected)
	{
		device_twin_update(&telemetry.latest.temperature, &telemetry.previous.temperature, &dt_hvac_temperature);
		device_twin_update(&telemetry.latest.pressure, &telemetry.previous.pressure, &dt_hvac_pressure);
		device_twin_update(&telemetry.latest.humidity, &telemetry.previous.humidity, &dt_hvac_humidity);

		if (telemetry.latest_operating_mode != HVAC_MODE_UNKNOWN && telemetry.latest_operating_mode != telemetry.previous_operating_mode)
		{
			telemetry.previous_operating_mode = telemetry.latest_operating_mode;
			// Update operating mode device twin
			dx_deviceTwinReportValue(&dt_hvac_operating_mode, hvac_state[telemetry.latest_operating_mode]);
		}
	}
}

/// <summary>
/// Publish HVAC telemetry
/// </summary>
/// <param name="eventLoopTimer"></param>
static void publish_telemetry_handler(EventLoopTimer* eventLoopTimer)
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

/***********************************************************************************************************
 * Integrate real-time core sensor
 *
 * Request latest environment readings from the real-time core app
 * Process environment readings intercore message from the real-time core app
 **********************************************************************************************************/

 /// <summary>
 /// read_telemetry_handler callback handler called every 4 seconds
 /// Environment sensors read and HVAC operating mode LED updated
 /// </summary>
 /// <param name="eventLoopTimer"></param>
static void read_telemetry_handler(EventLoopTimer* eventLoopTimer)
{
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
	{
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	// Set command for real-time core application
	intercore_block.cmd = IC_READ_SENSOR;
	dx_intercorePublish(&intercore_environment_ctx, &intercore_block, sizeof(intercore_block));
}

/// <summary>
/// Callback handler for Inter-Core Messaging
/// </summary>
static void intercore_environment_receive_msg_handler(void* data_block, ssize_t message_length)
{
	INTERCORE_BLOCK* ic_data = (INTERCORE_BLOCK*)data_block;

	switch (ic_data->cmd)
	{
	case IC_READ_SENSOR:
		telemetry.latest.temperature = ic_data->temperature;
		telemetry.latest.pressure = ic_data->pressure;
		telemetry.latest.humidity = ic_data->humidity;
		telemetry.latest_operating_mode = ic_data->operating_mode;

		telemetry.updated = true;

		// clang-format off
		telemetry.valid =
			IN_RANGE(telemetry.latest.temperature, -20, 50) &&
			IN_RANGE(telemetry.latest.pressure, 800, 1200) &&
			IN_RANGE(telemetry.latest.humidity, 0, 100);
		// clang-format on

		if (telemetry.previous_operating_mode != telemetry.latest_operating_mode)
		{
			telemetry.previous_operating_mode = telemetry.latest_operating_mode;
			// Update HVAC operating mode device twin
			dx_deviceTwinReportValue(&dt_hvac_operating_mode, hvac_state[telemetry.latest_operating_mode]);
		}

		break;
	default:
		break;
	}
}

/***********************************************************************************************************
 * REMOTE OPERATIONS: DEVICE TWINS
 *
 * Set target HVAC temperature
 **********************************************************************************************************/

 /// <summary>
 /// dt_set_target_temperature_handler callback handler is called when TargetTemperature device twin message received
 /// HVAC operating mode LED updated and IoT Plug and Play device twin acknowledged
 /// </summary>
 /// <param name="deviceTwinBinding"></param>
static void dt_set_target_temperature_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding)
{
	if (IN_RANGE(*(int*)deviceTwinBinding->propertyValue, 0, 50))
	{
		intercore_block.cmd = IC_TARGET_TEMPERATURE;
		intercore_block.temperature = *(int*)deviceTwinBinding->propertyValue;
		dx_intercorePublish(&intercore_environment_ctx, &intercore_block, sizeof(intercore_block));

		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
	}
	else
	{
		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
	}
}


/***********************************************************************************************************
 * REMOTE OPERATIONS: DIRECT METHODS
 *
 * Set HVAC panel message
 * Turn HVAC on and off
 **********************************************************************************************************/

 // Direct method name = HvacOn
static DX_DIRECT_METHOD_RESPONSE_CODE gpio_on_handler(JSON_Value* json, DX_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg)
{
	dx_gpioOn((DX_GPIO_BINDING*)directMethodBinding->context);
	return DX_METHOD_SUCCEEDED;
}

// Direct method name = HvacOff
static DX_DIRECT_METHOD_RESPONSE_CODE gpio_off_handler(JSON_Value* json, DX_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg)
{
	dx_gpioOff((DX_GPIO_BINDING*)directMethodBinding->context);
	return DX_METHOD_SUCCEEDED;
}

/***********************************************************************************************************
 * PRODUCTION
 *
 * Enable remote HVAC restart
 * Update software version and Azure connect UTC time device twins on first connection
 * Enable and extend application level watchdog
 **********************************************************************************************************/

 /************************************************************************************************************
  * Add remote HVAC restart
  ***********************************************************************************************************/

  /// <summary>
  /// Restart the Device
  /// </summary>
static void hvac_delay_restart_handler(EventLoopTimer* eventLoopTimer)
{
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
	{
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	PowerManagement_ForceSystemReboot();
}

/// <summary>
/// Start Device Power Restart Direct Method 'ResetMethod' integer seconds eg 5
/// </summary>
static DX_DIRECT_METHOD_RESPONSE_CODE hvac_restart_handler(JSON_Value* json, DX_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg)
{
	// Allocate and initialize a response message buffer. The
	// calling function is responsible for the freeing memory
	const size_t responseLen = 100;
	static struct timespec period;

	*responseMsg = (char*)malloc(responseLen);
	memset(*responseMsg, 0, responseLen);

	if (json_value_get_type(json) != JSONNumber)
	{
		return DX_METHOD_FAILED;
	}

	int seconds = (int)json_value_get_number(json);

	// leave enough time for the device twin dt_reportedRestartUtc
	// to update before restarting the device
	if (IN_RANGE(seconds, 3, 10))
	{
		// Create Direct Method Response
		snprintf(*responseMsg, responseLen, "%s called. Restart in %d seconds", directMethodBinding->methodName, seconds);

		// Set One Shot DX_TIMER_BINDING
		period = (struct timespec){ .tv_sec = seconds, .tv_nsec = 0 };
		dx_timerOneShotSet(&tmr_hvac_restart_oneshot_timer, &period);

		return DX_METHOD_SUCCEEDED;
	}
	else
	{
		snprintf(*responseMsg, responseLen, "%s called. Restart Failed. Seconds out of range: %d", directMethodBinding->methodName, seconds);

		return DX_METHOD_FAILED;
	}
}

/************************************************************************************************************
 * Update software version and Azure connect UTC time device twins on first connection
 ***********************************************************************************************************/

 /// <summary>
 /// Called when the Azure connection status changes then unregisters this callback
 /// </summary>
 /// <param name="connected"></param>
static void hvac_startup_report(bool connected)
{
	snprintf(msgBuffer, sizeof(msgBuffer), "HVAC firmware: %s, DevX version: %s", HVAC_FIRMWARE_VERSION, AZURE_SPHERE_DEVX_VERSION);
	dx_deviceTwinReportValue(&dt_hvac_sw_version, msgBuffer);                                     // DX_TYPE_STRING
	dx_deviceTwinReportValue(&dt_hvac_start_utc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

	dx_azureUnregisterConnectionChangedNotification(hvac_startup_report);
}

/***********************************************************************************************************
 * Add deferred update support
 **********************************************************************************************************/

 /// <summary>
 /// Algorithm to determine if a deferred update can proceed
 /// </summary>
 /// <param name="max_deferral_time_in_minutes">The maximum number of minutes you can defer</param>
 /// <returns>Return 0 to start update, return greater than zero to defer</returns>
static uint32_t DeferredUpdateCalculate(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type, SysEvent_Status status, const char* typeDescription,
	const char* statusDescription)
{
	// UTC +11 is for Australia/Sydney AEDT
	// Set the time_zone_offset to your time zone offset.
	const int time_zone_offset = 11;

	char utc[40];

	//  Get UTC time
	time_t now = time(NULL);
	struct tm* t = gmtime(&now);

	// Calculate UTC plus offset and normalize.
	t->tm_hour += time_zone_offset;
	t->tm_hour = t->tm_hour % 24;

	// If local time is between 1 am and 5 am defer for zero minutes else defer for 15 minutes
	uint32_t requested_minutes = IN_RANGE(t->tm_hour, 1, 5) ? 0 : 15;

	// Update defer requested device twin
	snprintf(msgBuffer, sizeof(msgBuffer), "Utc: %s, Type: %s, Status: %s, Max defer minutes: %i, Requested minutes: %i", dx_getCurrentUtc(utc, sizeof(utc)),
		typeDescription, statusDescription, max_deferral_time_in_minutes, requested_minutes);

	dx_deviceTwinReportValue(&dt_defer_requested, msgBuffer);

	return requested_minutes;
}

/************************************************************************************************************
 * Add application level watchdog
 ***********************************************************************************************************/

 /// <summary>
 /// This timer extends the app level lease watchdog
 /// </summary>
 /// <param name="eventLoopTimer"></param>
static void watchdog_handler(EventLoopTimer* eventLoopTimer)
{
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
	{
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}

/// <summary>
/// Set up watchdog timer - the lease is extended via the Watchdog_handler function
/// </summary>
/// <param name=""></param>
void start_watchdog(void)
{
	struct sigevent alarmEvent;
	alarmEvent.sigev_notify = SIGEV_SIGNAL;
	alarmEvent.sigev_signo = SIGALRM;
	alarmEvent.sigev_value.sival_ptr = &watchdogTimer;

	if (timer_create(CLOCK_MONOTONIC, &alarmEvent, &watchdogTimer) == 0)
	{
		if (timer_settime(watchdogTimer, 0, &watchdogInterval, NULL) == -1)
		{
			dx_Log_Debug("Issue setting watchdog timer. %s %d\n", strerror(errno), errno);
		}
	}
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
	dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
	dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
	dx_intercoreConnect(&intercore_environment_ctx);

	dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
	dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
	dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
	dx_directMethodSubscribe(direct_method_binding_sets, NELEMS(direct_method_binding_sets));

	dx_azureRegisterConnectionChangedNotification(azure_connection_state);
	dx_azureRegisterConnectionChangedNotification(hvac_startup_report);

	dx_deferredUpdateRegistration(DeferredUpdateCalculate, NULL);

	// Uncomment for production
	// start_watchdog();

	// initialize previous environment sensor variables
	telemetry.previous.temperature = telemetry.previous.pressure = telemetry.previous.humidity = INT32_MAX;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
	dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
	dx_deviceTwinUnsubscribe();
	dx_directMethodUnsubscribe();
	dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
	dx_timerEventLoopStop();
}

int main(int argc, char* argv[])
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