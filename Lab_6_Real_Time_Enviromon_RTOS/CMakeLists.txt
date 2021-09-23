#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)
#set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project (demo_threadx C ASM)

SET(CMAKE_ASM_FLAGS "-mcpu=cortex-m4")

if (EXISTS "${PARENT_DIR}/azsphere_board.txt")
    include("${PARENT_DIR}/azsphere_board.txt")
else()
    include(cmake/azsphere_board.cmake)
endif()

include(cmake/azsphere_config.cmake)

add_compile_definitions(OSAI_AZURE_RTOS)

add_link_options(-specs=nano.specs -specs=nosys.specs)

# Create executable
add_executable (${PROJECT_NAME}
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_adc.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_dma.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_eint.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpio.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpioif.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpt.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2c.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2s.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_pwm.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_spim.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_uart.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_wdt.c
    ./demo_threadx/demo_threadx.c 
    ./demo_threadx/rtcoremain.c
    ./demo_threadx/tx_initialize_low_level.S

    ./demo_threadx/mt3620-intercore.c                             
    ./demo_threadx/mt3620-uart-poll.c 
    
    IMU_lib/imu_temp_pressure.c
    IMU_lib/lps22hh_reg.c
    IMU_lib/lsm6dso_reg.c
    
    )

target_include_directories(${PROJECT_NAME} PUBLIC mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/inc)
target_include_directories(${PROJECT_NAME} PUBLIC
                            mt3620_m4_software/MT3620_M4_Driver/MHAL/inc
                            mt3620_m4_software/MT3620_M4_Driver/HDL/inc
                            mt3620_m4_software/MT3620_M4_BSP/CMSIS/include
                            mt3620_m4_software/MT3620_M4_BSP/mt3620/inc
                            mt3620_m4_software/MT3620_M4_BSP/printf)
target_include_directories(${PROJECT_NAME} PUBLIC
                           ../IntercoreContract
                           ./imu_temp_pressure)

add_subdirectory(mt3620_m4_software/MT3620_M4_Driver)

set(THREADX_ARCH "cortex_m4")
set(THREADX_TOOLCHAIN "gnu")
add_subdirectory(threadx)

target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver azrtos::threadx)
target_link_libraries(${PROJECT_NAME} m)

set_target_properties (${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${PROJECT_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
# include ("${AZURE_SPHERE_MAKE_IMAGE_FILE}")

if(AVNET)
    add_definitions( -DOEM_AVNET=TRUE )
endif(AVNET)

if(AVNET_REV_2)
    add_definitions( -DOEM_AVNET=TRUE )
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)
    add_definitions( -DOEM_SEEED_STUDIO=TRUE )
endif()

if (SEEED_STUDIO_MINI)
    add_definitions( -DOEM_SEEED_STUDIO_MINI=TRUE )    
endif(SEEED_STUDIO_MINI)

azsphere_target_add_image_package(${PROJECT_NAME})

