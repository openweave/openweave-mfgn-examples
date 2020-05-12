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
 *   This file implements EFR32 hardware platform specific functionality.
 */

#include "HardwarePlatform.h"
#include "app.h"
#include "Button.h"
#include "AppTask.h"
#include "Efr32LED.h"

#include <efr32-weave-mbedtls-config.h>
#include <mbedtls/threading.h>
#include <openthread/heap.h>

#include <stdbool.h>
#include <stdint.h>

#include "efr32_log.h"

// Singleton.
HardwarePlatform HardwarePlatform::sHardwarePlatform;

#include <mbedtls/platform.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/EFR32/GroupKeyStoreImpl.h>
#include <Weave/DeviceLayer/internal/testing/ConfigUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/GroupKeyStoreUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/SystemClockUnitTest.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;

typedef struct ButtonArray
{
    GPIO_Port_TypeDef port;
    unsigned int      pin;
} ButtonArray_t;

static const ButtonArray_t sButtonArray[PLATFORM_BUTTONS_COUNT] = BSP_BUTTON_INIT; // GPIO info for the 2 WDTK buttons.
TimerHandle_t buttonTimers[PLATFORM_BUTTONS_COUNT]; // FreeRTOS timers used for debouncing buttons.

// ================================================================================
// App Error
//=================================================================================
void appError(int err)
{
    EFR32_LOG("!!!!!!!!!!!! App Critical Error: %d !!!!!!!!!!!", err);
    portDISABLE_INTERRUPTS();
    while (1)
        ;
}

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
#if EFR32_LOG_ENABLED
    if (efr32LogInit() != 0)
    {
        appError(WEAVE_ERROR_MAX);
    }
#endif

    EFR32_LOG("==================================================");
    EFR32_LOG(APP_NAME);
    EFR32_LOG("Hardware Platform: EFR32");
#if BUILD_RELEASE
    EFR32_LOG("*** PSEUDO-RELEASE BUILD ***");
#else
    EFR32_LOG("*** DEVELOPMENT BUILD ***");
#endif
    EFR32_LOG("==================================================");

    otHeapSetCAllocFree(calloc, free);

    // Initialize mbedtls threading support on EFR32.
    EFR32_LOG("setup mbedtls");
    THREADING_setup();

    InitLEDs();

    InitButtons();
}

void HardwarePlatform::InitLEDs(void)
{
    EFR32_LOG("InitLEDS()");

    // Sets gpio pin mode for ALL board Leds.
    BSP_LedsInit();

    for (uint8_t i = 0; i < PLATFORM_LEDS_COUNT; i++)
    {
         mLEDs[i].Init(new Efr32LED(i));
    }
}

LED * HardwarePlatform::GetLEDs(void)
{
    return mLEDs;
}

// -----------------------------------------------------------------------------
// Buttons

// Initialize buttons
int HardwarePlatform::InitButtons(void)
{
    EFR32_LOG("InitButtons()");

    ButtonGpioInit();

    for (uint8_t i = 0; i < PLATFORM_BUTTONS_COUNT; i++)
    {
        mButtons[0].Init();

        // Create FreeRTOS sw timer for debouncing the button.
        buttonTimers[i] =
            xTimerCreate("BtnTmr",                      // Just a text name, not used by the RTOS kernel
                         PLATFORM_BUTTON_DEBOUNCE_PERIOD_MS, // timer period
                         false,                         // no timer reload (==one-shot)
                         (void *)(int)i,                // init timer id = button index
                         ButtonDebounceTimerCallback    // timer callback handler (all buttons use the same timer cn function)
            );
    }

    return 0;
}

void HardwarePlatform::ButtonGpioInit(void)
{
    // Set up button GPIOs to input with pullups.
    for (uint8_t i = 0; i < PLATFORM_BUTTONS_COUNT; i++)
    {
        GPIO_PinModeSet(sButtonArray[i].port, sButtonArray[i].pin, gpioModeInputPull, 1);
    }
    // Set up interrupt based callback function - trigger on both edges.
    GPIOINT_Init();
    GPIOINT_CallbackRegister(sButtonArray[0].pin, Button0Isr);
    GPIOINT_CallbackRegister(sButtonArray[1].pin, Button1Isr);
    GPIO_IntConfig(sButtonArray[0].port, sButtonArray[0].pin, true, true, true);
    GPIO_IntConfig(sButtonArray[1].port, sButtonArray[1].pin, true, true, true);

    // Change GPIO interrupt priority (FreeRTOS asserts unless this is done here!)
    NVIC_SetPriority(GPIO_EVEN_IRQn, 5);
    NVIC_SetPriority(GPIO_ODD_IRQn, 5);
}

void HardwarePlatform::Button0Isr(uint8_t pin)
{
    // ISR for Button 0.
    uint8_t btnIdx = 0;

    if (pin == sButtonArray[btnIdx].pin)
    {
        ButtonEventHelper(btnIdx, true); // true== 'isr context'
    }
}

void HardwarePlatform::Button1Isr(uint8_t pin)
{
    // ISR for Button 1.
    uint8_t btnIdx = 1;

    if (pin == sButtonArray[btnIdx].pin)
    {
        ButtonEventHelper(btnIdx, true); // true== 'isr context'
    }
}

void HardwarePlatform::ButtonEventHelper(uint8_t btnIdx, bool isrContext)
{
    // May be called from Interrupt context so keep it short!

    if (btnIdx < PLATFORM_BUTTONS_COUNT)
    {
        if (isrContext)
        {
            portBASE_TYPE taskWoken = pdFALSE; // For FreeRTOS timer (below).

            // Start/restart the button debounce timer (Note ISR version of FreeRTOS api call here).
            xTimerStartFromISR(buttonTimers[btnIdx], &taskWoken);
            if (taskWoken != pdFALSE)
            {
                taskYIELD();
            }
        }
        else
        {
            // Called by debounce timer expiry (button gpio pin is now stable).

            // Get button gpio pin state.
            bool pressed = !GPIO_PinInGet(sButtonArray[btnIdx].port, sButtonArray[btnIdx].pin);

            // Notify App task of button state change.
            ButtonHwEventHandler(btnIdx, pressed);
        }
    }
}

void HardwarePlatform::ButtonDebounceTimerCallback(TimerHandle_t xTimer)
{
    // Get the timerId of the expired timer and call button event helper.

    uint32_t timerId;

    timerId = (uint32_t)pvTimerGetTimerID(xTimer);
    if (timerId < PLATFORM_BUTTONS_COUNT)
    {
        uint8_t btnIdx = timerId;
        ButtonEventHelper(btnIdx, false); // false== 'not from isr context'
    }
}

/**
 * Event handler called for every (debounced) hw platform button event.
 */
void HardwarePlatform::ButtonHwEventHandler(uint8_t buttonIndex, bool pressed)
{
    HardwarePlatform & _this = GetHardwarePlatform();
    Button::PhysicalButtonAction action;

    if (pressed)
    {
        action = Button::kPhysicalButtonAction_Press;
    }
    else
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

Button * HardwarePlatform::GetButtons(void)
{
    return mButtons;
}

