/*
 *
 *    Copyright (c) 2020 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @file
 *   This file implements nRF52840 devkit hardware platform specific functionality.
 */

#include "HardwarePlatform.h"
#include "app.h"
#include "Button.h"
#include "AppTask.h"
#include "Nrf5LED.h"

#include <stdbool.h>
#include <stdint.h>

//#include "boards.h"
//#include "app_button.h"

#include "nrf_log.h"
#include "nrf_delay.h"
#include "app_button.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#endif
#include "nrf_drv_clock.h"
#if NRF_CRYPTO_ENABLED
#include "nrf_crypto.h"
#endif
#include "mem_manager.h"
extern "C" {
#include "freertos_mbedtls_mutex.h"
}

// Singleton.
HardwarePlatform HardwarePlatform::sHardwarePlatform;

#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_uart.h"
#endif // NRF_LOG_ENABLED

#include <mbedtls/platform.h>

// FIXME: not sure I need this...
#include <openthread/instance.h>
#include <openthread/thread.h>
#include <openthread/tasklet.h>
#include <openthread/link.h>
#include <openthread/dataset.h>
#include <openthread/error.h>
#include <openthread/icmp6.h>
#include <openthread/platform/openthread-system.h>
extern "C" {
#include <openthread/platform/platform-softdevice.h>
}

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/nRF5/GroupKeyStoreImpl.h>
#include <Weave/DeviceLayer/internal/testing/ConfigUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/GroupKeyStoreUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/SystemClockUnitTest.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;

// FIXME: extern "C" size_t GetHeapTotalSize(void);

// ================================================================================
// Logging Support
// ================================================================================

#if NRF_LOG_ENABLED

#if NRF_LOG_USES_TIMESTAMP

uint32_t LogTimestamp(void)
{
    return static_cast<uint32_t>(nl::Weave::System::Platform::Layer::GetClock_MonotonicMS());
}

#define LOG_TIMESTAMP_FUNC LogTimestamp
#define LOG_TIMESTAMP_FREQ 1000

#else // NRF_LOG_USES_TIMESTAMP

#define LOG_TIMESTAMP_FUNC NULL
#define LOG_TIMESTAMP_FREQ 0

#endif // NRF_LOG_USES_TIMESTAMP

#endif // NRF_LOG_ENABLED

// ================================================================================
// SoftDevice Support
// ================================================================================

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

static void OnSoCEvent(uint32_t sys_evt, void * p_context)
{
    UNUSED_PARAMETER(p_context);

    otSysSoftdeviceSocEvtHandler(sys_evt);
}

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

// ================================================================================
// J-Link Monitor Mode Debugging Support
// ================================================================================

#if JLINK_MMD

extern "C" void JLINK_MONITOR_OnExit(void) {}

extern "C" void JLINK_MONITOR_OnEnter(void) {}

extern "C" void JLINK_MONITOR_OnPoll(void) {}

#endif // JLINK_MMD

void HardwarePlatform::Init(void)
{
    ret_code_t ret;

#if JLINK_MMD
    NVIC_SetPriority(DebugMonitor_IRQn, _PRIO_SD_LOWEST);
#endif

    // Initialize clock driver.
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);

    // Wait for the clock to be ready.
    while (!nrf_clock_lf_is_running())
    {
    }

#if NRF_LOG_ENABLED

    // Initialize logging component
    ret = NRF_LOG_INIT(LOG_TIMESTAMP_FUNC, LOG_TIMESTAMP_FREQ);
    APP_ERROR_CHECK(ret);

    // Initialize logging backends
    NRF_LOG_DEFAULT_BACKENDS_INIT();

#endif

    NRF_LOG_INFO("==================================================");
    NRF_LOG_INFO(APP_NAME);
    NRF_LOG_INFO("Hardware Platform: NRF5");
#if BUILD_RELEASE
    NRF_LOG_INFO("*** PSEUDO-RELEASE BUILD ***");
#else
    NRF_LOG_INFO("*** DEVELOPMENT BUILD ***");
#endif
    NRF_LOG_INFO("==================================================");

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    NRF_LOG_INFO("Enabling SoftDevice");

    ret = nrf_sdh_enable_request();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_sdh_enable_request() failed");
        APP_ERROR_HANDLER(ret);
    }

    NRF_LOG_INFO("Waiting for SoftDevice to be enabled");

    while (!nrf_sdh_is_enabled())
    {
    }

    // Register a handler for SOC events.
    NRF_SDH_SOC_OBSERVER(m_soc_observer, NRF_SDH_SOC_STACK_OBSERVER_PRIO, OnSoCEvent, NULL);

    NRF_LOG_INFO("SoftDevice enable complete");

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    NRF_LOG_INFO("nrf_mem_init()");
    ret = nrf_mem_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_mem_init() failed");
        APP_ERROR_HANDLER(ret);
    }

#if NRF_CRYPTO_ENABLED
    NRF_LOG_INFO("nrf_crypto_init()");
    ret = nrf_crypto_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_crypto_init() failed");
        APP_ERROR_HANDLER(ret);
    }
#endif

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    {
        NRF_LOG_INFO("enable BLE stack");
        uint32_t appRAMStart = 0;

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        ret = nrf_sdh_ble_default_cfg_set(WEAVE_DEVICE_LAYER_BLE_CONN_CFG_TAG, &appRAMStart);
        APP_ERROR_CHECK(ret);

        // Enable BLE stack.
        ret = nrf_sdh_ble_enable(&appRAMStart);
        APP_ERROR_CHECK(ret);
    }

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    // Configure mbedTLS to use FreeRTOS-based mutexes.  This ensures that mbedTLS can be used
    // simultaneously from multiple FreeRTOS tasks (e.g. OpenThread, OpenWeave and the application).
    NRF_LOG_INFO("setup mbedtls");
    freertos_mbedtls_mutex_init();

    // Reconfigure mbedTLS to use regular calloc and free.
    //
    // By default, OpenThread configures mbedTLS to use its private heap at initialization time.  However,
    // the OpenThread heap is not thread-safe, effectively preventing other threads from using mbedTLS
    // functions.
    //
    // Note that this presumes that the system heap is itself thread-safe.  On newlib-based systems
    // this requires a proper implementation of __malloc_lock()/__malloc_unlock() for the applicable
    // RTOS.  It also requires the heap to be provisioned with enough storage to accommodate OpenThread's
    // needs.
    //
    // FIXME: does this have to happen after the Weave and OT stacks are initialized?
    // If so, this would have to be called independently...
    mbedtls_platform_set_calloc_free(calloc, free);

    // Activate deep sleep mode
    // FIXME: Is this necessary? Can it be done before we start doing OT/OW iinitializations?
    //    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    NRF_LOG_INFO("Init the LEDs");
    InitLEDs();

    NRF_LOG_INFO("Init the Buttons");
    InitButtons();
}

LED * HardwarePlatform::GetLEDs(void)
{
    return mLEDs;
}

void HardwarePlatform::InitLEDs(void)
{
    // FIXME: put all leds in a list, and then have loop over that list. cleaner code, no?
    nrf_gpio_cfg_output(BSP_LED_0);
    nrf_gpio_cfg_output(BSP_LED_1);
    nrf_gpio_cfg_output(BSP_LED_2);
    nrf_gpio_cfg_output(BSP_LED_3);

    mLEDs[0].Init(new Nrf5LED(BSP_LED_0));
    mLEDs[1].Init(new Nrf5LED(BSP_LED_1));
    mLEDs[2].Init(new Nrf5LED(BSP_LED_2));
    mLEDs[3].Init(new Nrf5LED(BSP_LED_3));
}

// -----------------------------------------------------------------------------
// Buttons

// Initialize buttons
int HardwarePlatform::InitButtons(void)
{
    int ret;

    static app_button_cfg_t sButtons[] = {
        { BUTTON_1, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, HardwarePlatform::ButtonHwEventHandler },
        { BUTTON_2, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, HardwarePlatform::ButtonHwEventHandler },
        { BUTTON_3, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, HardwarePlatform::ButtonHwEventHandler },
        { BUTTON_4, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, HardwarePlatform::ButtonHwEventHandler },
    };

    ret = app_button_init(sButtons, ARRAY_SIZE(sButtons), pdMS_TO_TICKS(PLATFORM_BUTTON_DEBOUNCE_PERIOD_MS));
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_init() failed");
        VerifyOrExit(ret == NRF_SUCCESS, );
    }

    ret = app_button_enable();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("app_button_enable() failed");
        VerifyOrExit(ret == NRF_SUCCESS, );
    }

    for (int i = 0; i < PLATFORM_BUTTONS_COUNT; i++)
    {
        mButtons[0].Init();
    }

    mButtonPinNos[0] = BUTTON_1;
    mButtonPinNos[1] = BUTTON_2;
    mButtonPinNos[2] = BUTTON_3;
    mButtonPinNos[3] = BUTTON_4;

exit:
    return ret;
}

Button * HardwarePlatform::GetButtons(void)
{
    return mButtons;
}

int HardwarePlatform::GetButtonIndex(uint8_t pinNo)
{
    for (int i = 0; i < PLATFORM_BUTTONS_COUNT; i++)
    {
        if (mButtonPinNos[i] == pinNo)
        {
            return i;
        }
    }
    // FIXME: proper error handling
    NRF_LOG_ERROR("ERROR: no button associated with pinNo [%d]", pinNo);
    return -1;
}

/**
 * Event handler called for every hw platform button event.
 */
void HardwarePlatform::ButtonHwEventHandler(uint8_t pin_no, uint8_t button_action)
{
    HardwarePlatform & _this = GetHardwarePlatform();
    int buttonIndex          = _this.GetButtonIndex(pin_no);
    Button::PhysicalButtonAction action;

    if (button_action == APP_BUTTON_PUSH)
    {
        action = Button::kPhysicalButtonAction_Press;
    }
    else if (button_action == APP_BUTTON_RELEASE)
    {
        action = Button::kPhysicalButtonAction_Release;
    }
    Button::PhysicalButtonAppTaskEventData appTaskEventData;
    appTaskEventData.Action    = action;
    appTaskEventData.ButtonPtr = &_this.mButtons[buttonIndex];

    // We go through the AppTask so that the event is handled within that task.
    AppTask::AppTaskEvent appTaskEvent;
    appTaskEvent.Handler = Button::PhysicalButtonEventHandler;
    appTaskEvent.Data    = &appTaskEventData;
    GetAppTask().PostEvent(&appTaskEvent);
}
