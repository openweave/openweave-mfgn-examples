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

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

// Forward declaration for AppTaskEvent
struct AppTaskEvent;

// Two constants define the behavior of a "long press".
// A long press "activation" period starts when the button press has been held
// for a period of LONG_PRESS_ACTIVATION_START_MS. At that time, all LEDs start flashing
// to indicate that the "long press" activation period has started.
// If the button press is held until reaching LONG_PRESS_ACTIVATION_COMPLETE_MS, then
// a "long press" button event is triggered when the button is released.
// If the button press is released before reaching LONG_PRESS_ACTIVATION_COMPLETE_MS, then
// the "long press" button event is cancelled and no button events are triggered.
#define LONG_PRESS_ACTIVATION_START_MS 3000
#define LONG_PRESS_ACTIVATION_COMPLETE_MS 6000

/**
 * Interface for a Button.
 *
 * Button supports two types of button events: short press, and long press.
 *
 * A short press event is triggered if the button has been pressed and released within
 * LONG_PRESS_ACTIVATION_START_MS.
 *
 * A long press "activation" period starts when the button press has been held
 * for a period of LONG_PRESS_ACTIVATION_START_MS. At that time, all LEDs start flashing
 * to indicate that the "long press" activation period has started.
 * If the button press is held until reaching LONG_PRESS_ACTIVATION_COMPLETE_MS, then
 * a "long press" button event is triggered when the button is released.
 * If the button press is released before reaching LONG_PRESS_ACTIVATION_COMPLETE_MS, then
 * the "long press" button event is cancelled and no button event is triggered.
 */
class Button
{
public:
    // The different states a button press goes through.
    // If no button press active, the state is kButtonPressState_Inactive.
    // As soon as a button press is detected, state moves to kButtonPressState_Short.
    // If the button press is held, then after LONG_PRESS_ACTIVATION_START_MS,
    // state moves to kButtonPressState_Long_Started.
    // If the button press is held, then after LONG_PRESS_ACTIVATION_COMPLETE_MS,
    // state moves to kButtonPressState_Long_Completed.
    enum ButtonPressState
    {
        kButtonPressState_Inactive = 0,
        kButtonPressState_Short,
        kButtonPressState_Long_Started,
        kButtonPressState_Long_Completed,
    };

    // These button event handlers are static methods in class DeviceController.
    // They get invoked when a logical "short press" or "long press" button event is detected.
    typedef void (*ButtonEventHandler_t)(void);

    // Physical button actions are triggered by the hardware platform and dispatched
    // to the static method "ButtonHwEventHandler" via the Application Task.
    enum PhysicalButtonAction
    {
        kPhysicalButtonAction_Press = 0, // Button has been pressed
        kPhysicalButtonAction_Release,   // Button has been released
    };

    struct PhysicalButtonAppTaskEventData
    {
        PhysicalButtonAction Action;
        Button * ButtonPtr;
    };

    // Initializes the Button.
    void Init(void);

    // Sets the event handler for a "short" button press.
    void SetShortPressEventHandler(ButtonEventHandler_t handler);

    // Sets the event handler for a "long" button press.
    void SetLongPressEventHandler(ButtonEventHandler_t handler);

    // This is the "physical" button event handler. This handler is triggered
    // by HardwarePlatform (via the AppTask event queue) whenever it gets a "physical" button event.
    // We go through the AppTask so that all application code executes within the Application
    // Task (and not within an interrupt thread).
    static void PhysicalButtonEventHandler(void * data);

    // Must be called at every cycle of the AppTask event loop for proper processing
    // of button press logic.
    ButtonPressState UpdateButtonPressState(void);

private:
    ButtonPressState mButtonPressState;

    // Time when a button press started (state moved from kButtonPressState_Inactive to
    // kButtonPressState_Short).
    uint64_t mButtonPressStartedMs;

    // The short press event handler.
    ButtonEventHandler_t mShortPressEventHandler;

    // The long press event handler.
    ButtonEventHandler_t mLongPressEventHandler;

    // Button press processing.
    void StartButtonPress(void);
    void EndButtonPress(void);
};

#endif // BUTTON_H
