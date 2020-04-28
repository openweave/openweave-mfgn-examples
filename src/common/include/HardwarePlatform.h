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

#include "LED.h"
#include "Button.h"
#include "boards.h"

#define PLATFORM_LEDS_COUNT 4
#define PLATFORM_BUTTONS_COUNT 4
#define PLATFORM_BUTTON_DEBOUNCE_PERIOD_MS 50

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

    //    void On(uint32_t ledId);
    //    void Off(uint32_t ledId);
    //
    //    /** Button accessors */
    //    uint8_t GetButtonIdFromPinNo(uint8_t pinNo);
    //    uint8_t GetPinNoFromButtonId(uint8_t buttonId);

private:
    LED mLEDs[BUTTONS_NUMBER];
    Button mButtons[BUTTONS_NUMBER];
    uint8_t mButtonPinNos[BUTTONS_NUMBER];

    // Initialize GPIO artifacts.
    void InitLEDs(void);
    int InitButtons(void);

    static void ButtonHwEventHandler(uint8_t pin_no, uint8_t button_action);
    int GetButtonIndex(uint8_t pinNo);

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