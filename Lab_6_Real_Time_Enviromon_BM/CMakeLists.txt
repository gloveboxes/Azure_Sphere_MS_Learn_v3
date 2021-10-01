# This code is based on a sample from Microsoft (see license below),
# with modifications made by MediaTek.
# Modified version of CMakeLists.txt from Microsoft Azure Sphere sample code:
# https://github.com/Azure/azure-sphere-samples/blob/master/Samples/HelloWorld/HelloWorld_RTApp_MT3620_BareMetal/CMakeLists.txt

#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

# Generate project id from the folder name
get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
string(TOLOWER ${ProjectId} ProjectId)

project(${ProjectId} C)

# Check if there is a global azsphere_board.cmake file
get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

if (EXISTS "${PARENT_DIR}/azsphere_board.txt")
    include("${PARENT_DIR}/azsphere_board.txt")
else()
    include(azsphere_board.txt)
endif()

if (EXISTS "${PARENT_DIR}/Tools/cmake/azsphere_config.cmake")
    include(../Tools/cmake/azsphere_config.cmake)
    auto_generate_tools_revision()
else()

    # For information on setting tools revision and target api set see 
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.07")

endif()

add_compile_definitions(OSAI_BARE_METAL)
add_compile_definitions(OSAI_ENABLE_DMA)

# When place CODE_REGION in FLASH instead of TCM, please enable this definition:
# add_compile_definitions(M4_ENABLE_XIP_FLASH)
add_link_options(-specs=nano.specs -specs=nosys.specs)

# Executable
add_executable(${PROJECT_NAME}
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_dma.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpio.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpt.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2c.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox_shared_mem.c
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_uart.c              
                intercore.c                 
                main.c
                utils.c
                ./IMU_lib/imu_temp_pressure.c
                ./IMU_lib/lps22hh_reg.c
                ./IMU_lib/lsm6dso_reg.c
               )

target_link_libraries(${PROJECT_NAME} m)

# Include Folders
target_include_directories(${PROJECT_NAME} PUBLIC
                mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/inc
                mt3620_m4_software/MT3620_M4_BSP/mt3620/inc
                mt3620_m4_software/MT3620_M4_Driver/MHAL/inc
                ../IntercoreContract
                ./)     

# Libraries
add_subdirectory(mt3620_m4_software/MT3620_M4_Driver ./lib/MT3620_M4_Driver)
target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver)

set_source_files_properties(./IMU_lib/lsm6dso_reg.c PROPERTIES COMPILE_FLAGS -Wno-maybe-uninitialized)

# Linker, Image
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

set(BOARD_COUNTER 0)

if(AVNET)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_learning_path.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 1")
endif(AVNET)

if(AVNET_REV_2)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "azure_sphere_learning_path.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 2")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO RDB")
endif()

if (SEEED_STUDIO_MINI)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO_MINI=TRUE )    
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/seeed_mt3620_mdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO MINI")
endif(SEEED_STUDIO_MINI)

if(BOARD_COUNTER EQUAL 0)
    message(FATAL_ERROR "No Azure Sphere boards selected. Ensure one board set")
endif()

if(BOARD_COUNTER GREATER 1)
    message(FATAL_ERROR "Multiple (${BOARD_COUNTER}) Azure Sphere boards selected. Ensure only one board set")
endif()

azsphere_target_add_image_package(${PROJECT_NAME})
