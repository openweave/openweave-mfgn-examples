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
 *    @file
 *      APIs supported by all hardware platforms that run the example applications.
 */

#ifndef HARDWARE_PLATFORM_H
#define HARDWARE_PLATFORM_H

#include "bsp.h"
#include "gpiointerrupt.h"
#include "hal-config-board.h"

#include <FreeRTOS.h>
#include <timers.h>

#include "LED.h"
#include "Button.h"

#define PLATFORM_LEDS_COUNT                  BSP_LED_COUNT
#define PLATFORM_BUTTONS_COUNT               BSP_BUTTON_COUNT
#define PLATFORM_BUTTON_DEBOUNCE_PERIOD_MS   50


// EFR32 WSTK LEDs
#define BSP_LED_0 0
#define BSP_LED_1 1
#define BSP_LED_2 2
#define BSP_LED_3 3


class HardwarePlatform
{
public:
    /**
     * Initialization of the hardware platform.
     * Called from the application main().
     */
    void Init();

    /** Returns an array of the LEDS available on the devkit. */
    LED * GetLEDs();

    /** Returns an array of the Buttons available on the devkit. */
    Button * GetButtons();


private:
    LED mLEDs[PLATFORM_LEDS_COUNT];
    Button mButtons[PLATFORM_BUTTONS_COUNT];

    void InitLEDs(void);
    int InitButtons(void);
    void ButtonGpioInit(void);
    static void Button0Isr(uint8_t pin);
    static void Button1Isr(uint8_t pin);
    static void ButtonEventHelper(uint8_t btnIdx, bool isrContext);
    static void ButtonDebounceTimerCallback(TimerHandle_t xTimer);
    static void ButtonHwEventHandler(uint8_t buttonIndex, bool pressed);

    // Singleton.
    friend HardwarePlatform & GetHardwarePlatform(void);
    static HardwarePlatform sHardwarePlatform;
};

// Singleton.
inline HardwarePlatform & GetHardwarePlatform(void)
{
    return HardwarePlatform::sHardwarePlatform;
}

#endif // HARDWARE_PLATFORM_H
