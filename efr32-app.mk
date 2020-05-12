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
#         Makefile for building the sample apps on the EFR32 platform.
#


PROJECT_ROOT := $(realpath .)

EFR32_SDK_ROOT         := $(EFR32_SDK_ROOT)
EFR32_TOOLS_ROOT       := $(EFR32_SDK_ROOT)

OPENWEAVE_ROOT         := $(PROJECT_ROOT)/third_party/openweave-core
BUILD_SUPPORT_DIR      := $(OPENWEAVE_ROOT)/build/efr32

OPENTHREAD_ROOT        := $(PROJECT_ROOT)/third_party/openthread
OPENTHREAD_SDK_SYMLINK := $(PROJECT_ROOT)/third_party/openthread/third_party/silabs/gecko_sdk_suite/v2.7
GECKO_SDK_SUITE_DIR    := $(EFR32_SDK_ROOT)

FREERTOS_ROOT          := $(PROJECT_ROOT)/third_party/FreeRTOS/FreeRTOS
FREERTOSCONFIG_DIR     := $(PROJECT_ROOT)/src/common/platforms/efr32/include

all : | $(OPENTHREAD_SDK_SYMLINK)

#TODO Fix
# $(OPENTHREAD_SDK_SYMLINK) :  \
#    ln -s $(GECKO_SDK_SUITE_DIR) $(OPENTHREAD_SDK_SYMLINK)

#TODO Fix - OpenThread changes required for the MFGN example to build without error
# cd $(OPENTHREAD_ROOT)  \
# git apply ../../src/common/platforms/efr32/fixup-openthread-compile-err.patch

include $(BUILD_SUPPORT_DIR)/efr32-app.mk
include $(BUILD_SUPPORT_DIR)/efr32-openweave.mk
include $(BUILD_SUPPORT_DIR)/efr32-openthread.mk
include $(BUILD_SUPPORT_DIR)/efr32-freertos.mk

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
    $(PROJECT_ROOT)/src/common/platforms/efr32/HardwarePlatform.cpp \
    $(PROJECT_ROOT)/src/common/platforms/efr32/efr32LED.cpp \
    $(PROJECT_ROOT)/src/common/platforms/efr32/app_timer.cpp \
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
    $(PROJECT_ROOT)/src/common/platforms/efr32/HardwarePlatform.cpp \
    $(PROJECT_ROOT)/src/common/platforms/efr32/efr32LED.cpp \
    $(PROJECT_ROOT)/src/common/platforms/efr32/app_timer.cpp \
    $(PROJECT_ROOT)/third_party/printf/printf.c

# To build an app (e.g. ocsensor)
#   $ make APP=ocsensor PLATFORM=efr32
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

SRCS = \
    $(APP_SRCS)

INC_DIRS = \
    $(PROJECT_ROOT) \
    $(PROJECT_ROOT)/src/common/include \
    $(PROJECT_ROOT)/src/common/platforms/efr32/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/traits/include \
    $(PROJECT_ROOT)/src/examples/$(APP_DIR)/schema/include \
    $(PROJECT_ROOT)/third_party/printf \
    $(OPENTHREAD_ROOT)/include

ifeq ($(EFR32FAMILY), efr32mg12)
    LINKER_SCRIPT = $(PROJECT_ROOT)/src/examples/$(APP_DIR)/platforms/efr32/ldscripts/openweave-efr32mg12-example.ld
else ifeq ($(EFR32FAMILY), efr32mg21)
    LINKER_SCRIPT = $(PROJECT_ROOT)/src/examples/$(APP_DIR)/platforms/efr32/ldscripts/openweave-efr32mg21-example.ld
else
    $(Error must specify EFR32FAMILY as "efr32mg12" or "efr32mg21".)
endif

DEFINES = \
  HARD_FAULT_LOG_ENABLE \
  RETARGET_VCOM \
  RETARGET_USART0 \
  PLATFORM_HEADER='<hal/micro/cortexm3/compiler/gcc.h>' \
  CORTEXM3_EFM32_MICRO \
  WEAVE_ERROR_LOGGING=1 \
  WEAVE_PROGRESS_LOGGING=1 \
  WEAVE_DETAIL_LOGGING=1  \
  THREAD_FULL_LOGS=1 \
  EFR32_LOG_ENABLED=1 \
  NVM3_DEFAULT_NVM_SIZE=40960

CFLAGS = \
    -specs=nano.specs

LDFLAGS = \
    -specs=nano.specs

ifdef DEVICE_FIRMWARE_REVISION
DEFINES += \
    WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION=\"$(DEVICE_FIRMWARE_REVISION)\"
endif

OPENTHREAD_PROJECT_CONFIG = $(PROJECT_ROOT)/src/common/include/OpenThreadConfig.h
OPENWEAVE_PROJECT_CONFIG = $(PROJECT_ROOT)/src/examples/$(APP_DIR)/include/WeaveProjectConfig.h

$(call GenerateBuildRules)
