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

#ifndef LED_H
#define LED_H

#include <stdint.h>
#include "PlatformLED.h"

/**
 * Base class that encapsulates the functionality of a LED.
 */
class LED
{
public:
    /**
     * Initializes the LED. PlatformLED abstracts what is specific to the hardware platform.
     * FIXME: what's defined in PlatformLED should simply be virtual methods on LED.
     * However, first attempt on this broke 'RegisterService. No clue why. Anyway,
     * fix this once everthing else is fine and committed to openweave/openwave-mfgn-examples.
     */
    void Init(PlatformLED * platformLED);

    /** Sets the LED "on" (true) or "off" (false). */
    void Set(bool state);

    /** Inverts the state of the  LED. */
    void Invert(void);

    /** Blinks the LED at the specified rate. */
    void Blink(uint32_t changeRateMS);

    /** Blinks the LED for specified periods of time for "on" and "off". */
    void Blink(uint32_t onTimeMS, uint32_t offTimeMS);

    /**
     * Animates the LED. Must be called at regular intervals to properly handle
     * blinking (either via a changeRate or on/off periods).
     */
    void Animate();

private:
    // The delegate which implements platform-specific behavior.
    PlatformLED * mPlatformLEDPtr;

    // State of the LED. "on" is true, "off" is false.
    bool mState;

    // Manage the blinking for specific on/off periods of time.
    int64_t mLastChangeTimeUS;
    uint32_t mBlinkOnTimeMS;
    uint32_t mBlinkOffTimeMS;

    // Utility method to support higher level LED state changes.
    void DoSet(bool state);

    void On(uint32_t ledId);
    void Off(uint32_t ledId);
};

#endif // LED_H
