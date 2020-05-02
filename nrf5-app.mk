#
#   Copyright (c) 2020 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   @file
#         Makefile for building the sample apps on the NRF5 platform.
#

PROJECT_ROOT ?= $(realpath $(dir $(firstword $(MAKEFILE_LIST))))

OPENWEAVE_ROOT ?= $(PROJECT_ROOT)/third_party/openweave-core

BUILD_SUPPORT_DIR = $(OPENWEAVE_ROOT)/build/nrf5

include $(BUILD_SUPPORT_DIR)/nrf5-app.mk
include $(BUILD_SUPPORT_DIR)/nrf5-openweave.mk
include $(BUILD_SUPPORT_DIR)/nrf5-openthread.mk

LOCK_SRCS = \
    $(PROJECT_ROOT)/src/examples/lock/main.cpp \
    $(PROJECT_ROOT)/src/examples/lock/DeviceController.cpp \
    $(PROJECT_ROOT)/src/examples/lock/WDMFeature.cpp \
    $(PROJECT_ROOT)/src/examples/lock/traits/BoltLockSettingsTraitDataSink.cpp \
    $(PROJECT_ROOT)/src/examples/lock/traits/BoltLockTraitDataSource.cpp \
    $(PROJECT_ROOT)/src/examples/lock/traits/DeviceIdentityTraitDataSource.cpp \
    $(PROJECT_ROOT)/src/examples/lock/schema/BoltLockSettingsTrait.cpp \
    $(PROJECT_ROOT)/src/examples/lock/schema/BoltLockTrait.cpp \
    $(PROJECT_ROOT)/src/examples/lock/schema/DeviceIdentityTrait.cpp \
    $(PROJECT_ROOT)/src/common/AppTask.cpp \
    $(PROJECT_ROOT)/src/common/LED.cpp \
    $(PROJECT_ROOT)/src/common/Button.cpp \
    $(PROJECT_ROOT)/src/common/ConnectivityState.cpp \
    $(PROJECT_ROOT)/src/common/AppSoftwareUpdateManager.cpp \
    $(PROJECT_ROOT)/src/common/AltPrintf.c \
    $(PROJECT_ROOT)/src/common/CXXExceptionStubs.cpp \
    $(PROJECT_ROOT)/src/common/FreeRTOSNewlibLockSupport.c \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/HardwarePlatform.cpp \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/Nrf5LED.cpp \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/nRF5Sbrk.c \
    $(PROJECT_ROOT)/third_party/printf/printf.c

OCSENSOR_SRCS = \
    $(PROJECT_ROOT)/src/examples/ocsensor/main.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/DeviceController.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/WDMFeature.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/ApplicationKeysTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/DeviceIdentityTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/DeviceLocatedSettingsTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/LocatedTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/OpenCloseTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/schema/SecurityOpenCloseTrait.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/traits/DeviceIdentityTraitDataSource.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/traits/DeviceLocatedSettingsTraitDataSink.cpp \
    $(PROJECT_ROOT)/src/examples/ocsensor/traits/SecurityOpenCloseTraitDataSource.cpp \
    $(PROJECT_ROOT)/src/common/AppTask.cpp \
    $(PROJECT_ROOT)/src/common/LED.cpp \
    $(PROJECT_ROOT)/src/common/Button.cpp \
    $(PROJECT_ROOT)/src/common/ConnectivityState.cpp \
    $(PROJECT_ROOT)/src/common/AppSoftwareUpdateManager.cpp \
    $(PROJECT_ROOT)/src/common/AltPrintf.c \
    $(PROJECT_ROOT)/src/common/CXXExceptionStubs.cpp \
    $(PROJECT_ROOT)/src/common/FreeRTOSNewlibLockSupport.c \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/HardwarePlatform.cpp \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/Nrf5LED.cpp \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/nRF5Sbrk.c \
    $(PROJECT_ROOT)/third_party/printf/printf.c

# To build an app (e.g. ocsensor)
#   $ make APP=ocsensor PLATFORM=nrf5
ifeq ($(APP),lock)
   APP_DIR = lock
   override APP = openweave-lock-$(PLATFORM)-example
   APP_SRCS = $(LOCK_SRCS)
   $(warning building lock application for $(PLATFORM))
else ifeq ($(APP),ocsensor)
   APP_DIR = ocsensor
   override APP = openweave-ocsensor-$(PLATFORM)-example
   APP_SRCS = $(OCSENSOR_SRCS)
   $(warning building ocsensor application for $(PLATFORM))
else
   $(Error must specify APP as "lock" or "ocsensor". For example,"make APP=lock".)
endif

OPENTHREAD_PROJECT_CONFIG = $(PROJECT_ROOT)/src/common/include/OpenThreadConfig.h
OPENWEAVE_PROJECT_CONFIG = $(PROJECT_ROOT)/src/examples/$(APP_DIR)/include/WeaveProjectConfig.h

SRCS = \
    $(APP_SRCS) \
    $(NRF5_SDK_ROOT)/components/ble/common/ble_advdata.c \
    $(NRF5_SDK_ROOT)/components/ble/common/ble_srv_common.c \
    $(NRF5_SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
    $(NRF5_SDK_ROOT)/components/boards/boards.c \
    $(NRF5_SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
    $(NRF5_SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
    $(NRF5_SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
    $(NRF5_SDK_ROOT)/components/libraries/button/app_button.c \
    $(NRF5_SDK_ROOT)/components/libraries/crc16/crc16.c \
    $(NRF5_SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
    $(NRF5_SDK_ROOT)/components/libraries/fds/fds.c \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_backend_rtt.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
    $(NRF5_SDK_ROOT)/components/libraries/mem_manager/mem_manager.c \
    $(NRF5_SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
    $(NRF5_SDK_ROOT)/components/libraries/queue/nrf_queue.c \
    $(NRF5_SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
    $(NRF5_SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
    $(NRF5_SDK_ROOT)/components/libraries/timer/app_timer_freertos.c \
    $(NRF5_SDK_ROOT)/components/libraries/uart/retarget.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error_weak.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_util_platform.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/nrf_assert.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
    $(NRF5_SDK_ROOT)/components/thread/freertos_mbedtls_mutex/freertos_mbedtls_mutex.c \
    $(NRF5_SDK_ROOT)/external/fprintf/nrf_fprintf.c \
    $(NRF5_SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52/port_cmsis.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52/port_cmsis_systick.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/GCC/nrf52/port.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/croutine.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/event_groups.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/list.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/portable/MemMang/heap_3.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/queue.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/stream_buffer.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/tasks.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/timers.c \
    $(NRF5_SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
    $(NRF5_SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
    $(NRF5_SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
    $(NRF5_SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
    $(NRF5_SDK_ROOT)/integration/nrfx/legacy/nrf_drv_rng.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uart.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uarte.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c

INC_DIRS = \
    $(PROJECT_ROOT) \
    $(PROJECT_ROOT)/src/common/include \
    $(PROJECT_ROOT)/src/common/platforms/nrf5/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/traits/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/schema/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/platforms/nrf5/include \
    $(PROJECT_ROOT)/third_party/printf \
    $(NRF5_SDK_ROOT)/components \
    $(NRF5_SDK_ROOT)/components/boards \
    $(NRF5_SDK_ROOT)/components/ble/ble_advertising \
    $(NRF5_SDK_ROOT)/components/ble/common \
    $(NRF5_SDK_ROOT)/components/ble/nrf_ble_gatt \
    $(NRF5_SDK_ROOT)/components/libraries/atomic \
    $(NRF5_SDK_ROOT)/components/libraries/atomic_fifo \
    $(NRF5_SDK_ROOT)/components/libraries/balloc \
    $(NRF5_SDK_ROOT)/components/libraries/bsp \
    $(NRF5_SDK_ROOT)/components/libraries/button \
    $(NRF5_SDK_ROOT)/components/libraries/crc16 \
    $(NRF5_SDK_ROOT)/components/libraries/delay \
    $(NRF5_SDK_ROOT)/components/libraries/experimental_section_vars \
    $(NRF5_SDK_ROOT)/components/libraries/fds \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage \
    $(NRF5_SDK_ROOT)/components/libraries/log \
    $(NRF5_SDK_ROOT)/components/libraries/log/src \
    $(NRF5_SDK_ROOT)/components/libraries/memobj \
    $(NRF5_SDK_ROOT)/components/libraries/mem_manager \
    $(NRF5_SDK_ROOT)/components/libraries/mutex \
    $(NRF5_SDK_ROOT)/components/libraries/queue \
    $(NRF5_SDK_ROOT)/components/libraries/ringbuf \
    $(NRF5_SDK_ROOT)/components/libraries/stack_info \
    $(NRF5_SDK_ROOT)/components/libraries/strerror \
    $(NRF5_SDK_ROOT)/components/libraries/timer \
    $(NRF5_SDK_ROOT)/components/libraries/util \
    $(NRF5_SDK_ROOT)/components/softdevice/common \
    $(NRF5_SDK_ROOT)/components/softdevice/s140/headers \
    $(NRF5_SDK_ROOT)/components/softdevice/mbr/nrf52840/headers \
    $(NRF5_SDK_ROOT)/components/thread/freertos_mbedtls_mutex \
    $(NRF5_SDK_ROOT)/components/toolchain/cmsis/include \
    $(NRF5_SDK_ROOT)/config/nrf52840/config \
    $(NRF5_SDK_ROOT)/external/fprintf \
    $(NRF5_SDK_ROOT)/external/freertos/config \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52 \
    $(NRF5_SDK_ROOT)/external/freertos/portable/GCC/nrf52 \
    $(NRF5_SDK_ROOT)/external/freertos/source/include \
    $(NRF5_SDK_ROOT)/external/segger_rtt \
    $(NRF5_SDK_ROOT)/integration/nrfx \
    $(NRF5_SDK_ROOT)/integration/nrfx/legacy \
    $(NRF5_SDK_ROOT)/modules/nrfx \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/include \
    $(NRF5_SDK_ROOT)/modules/nrfx/hal \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk

DEFINES = \
    NRF52840_XXAA \
    BOARD_PCA10056 \
    BSP_DEFINES_ONLY \
    CONFIG_GPIO_AS_PINRESET \
    FLOAT_ABI_HARD \
    USE_APP_CONFIG \
    __HEAP_SIZE=40960 \
    __STACK_SIZE=8192 \
    SOFTDEVICE_PRESENT \
    PRINTF_DISABLE_SUPPORT_EXPONENTIAL

CFLAGS = \
    --specs=nano.specs

LDFLAGS = \
    --specs=nano.specs

ifdef DEVICE_FIRMWARE_REVISION
DEFINES += \
    WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION=\"$(DEVICE_FIRMWARE_REVISION)\"
endif

LINKER_SCRIPT = $(PROJECT_ROOT)/src/examples/$(APP_DIR)/platforms/nrf5/ldscripts/openweave-nrf52840-example.ld

$(call GenerateBuildRules)
