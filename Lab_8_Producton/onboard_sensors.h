/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

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
    HVAC_OPERATING_MODE latest_operating_mode;
    HVAC_OPERATING_MODE previous_operating_mode;
} ENVIRONMENT;
