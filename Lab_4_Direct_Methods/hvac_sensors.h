/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "dx_i2c.h"

#include "hw/azure_sphere_learning_path.h"

#ifdef OEM_AVNET
#include "AzureSphereDrivers/AVNET/HL/imu_temp_pressure.h"
#endif

extern DX_I2C_BINDING i2c_onboard_sensors;

typedef struct {
    int temperature;
    int pressure;
    int humidity;
} SENSOR;

typedef enum { HVAC_MODE_UNKNOWN, HVAC_MODE_HEATING, HVAC_MODE_GREEN, HVAC_MODE_COOLING } HVAC_OPERATING_MODE;

typedef struct {
    SENSOR latest;
    SENSOR previous;
    bool updated;
    bool valid;
    HVAC_OPERATING_MODE latest_operating_mode;
    HVAC_OPERATING_MODE previous_operating_mode;
} ENVIRONMENT;

bool hvac_sensors_read(SENSOR *telemetry);
bool hvac_sensors_init(void);
bool hvac_sensors_close(void);