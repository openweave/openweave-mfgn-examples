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

#ifndef OCSENSOR_DEVICE_CONTROLLER_H
#define OCSENSOR_DEVICE_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#include "AppTask.h"
#include "LED.h"
#include "ConnectivityState.h"

// LEDs
#define CONNECTIVITY_STATE_LED_INDEX 0
#define OCSENSOR_STATE_LED_INDEX 1

// Buttons
#define BUTTON_1_INDEX 0
#define BUTTON_2_INDEX 1

// The time period for which "User Selected Mode" is enabled when it is activated.
// See doc/DeviceAssociationInLocalNetwor.md.
#define USER_SELECTED_MODE_TIMEOUT_MS 60000

/**
 * Controller for an Open/Close Sensor device simulated via a hardware developer kit with the
 * following GPIO artifacts:
 *
 * LEDs
 *   LED 0: Shows connectivity state (see ConnectivityState.h)
 *   LED 1: Shows the Open/Close Sensor state. OFF:Open  ON:Closed
 *
 * Buttons
 *   Button 1 short press: Triggers Software Update
 *   Button 1 long press: Triggers a Factory Reset
 *   Button 2: Toggles the sensor state (open/close)
 */
class DeviceController
{
public:
    enum State_t
    {
        kState_Open = 0,
        kState_Closed,
    } State;

    // Initializations.
    void Init(void);

    // Called on every cycle of the Application Task event loop.
    static void EventLoopCycle(void);

    // Accessor methods
    bool IsOpen();

private:
    // Current state of the OC Sensor.
    State_t mState;

    // Whether a "long press" button event is in flight.
    bool mLongPressButtonEventInFlight;

    // LEDs
    LED * mConnectivityStateLEDPtr;
    LED * mOCSensorStateLEDPtr;

    // ConnectivityState displayed on a LED.
    ConnectivityState mConnectivityState;

    // Button event handlers.
    static void OCSensorButtonEventHandler(void);
    static void SoftwareUpdateButtonHandler(void);
    static void FactoryResetButtonHandler(void);
    static void EnableUserSelectedModeButtonHandler(void);

    // Expose singleton object.
    friend DeviceController & GetDeviceController(void);
    static DeviceController sDeviceController;
};

// Exposes singleton object.
inline DeviceController & GetDeviceController(void)
{
    return DeviceController::sDeviceController;
}

#endif // OCSENSOR_DEVICE_CONTROLLER_H
