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

#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#include "AppTask.h"
#include "LED.h"
#include "ConnectivityState.h"

#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Core/WeaveCore.h>
#include <InetLayer/InetLayer.h>

using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Inet;

// LEDs
#define CONNECTIVITY_STATE_LED_INDEX 0
#define LOCK_STATE_LED_INDEX 1

// Buttons
#define BUTTON_1_INDEX 0
#define BUTTON_2_INDEX 1

// How long it takes for the bolt to change position.
#define ACTUATOR_MOVEMENT_DURATION_MS 2000

/**
 * Controller for a lock device simulated via a hardware developer kit with the following
 * GPIO artifacts:
 *
 * LEDs
 *   LED 0: Shows connectivity state (see ConnectivityState.h)
 *   LED 1: Shows lock state. OFF:Unlocked  ON:Locked
 *
 * Buttons
 *   Button 1 short press: Triggers Software Update
 *   Button 1 long press: Triggers a Factory Reset
 *   Button 2: Toggles the lock state (lock/unlock)
 */
class DeviceController
{
public:
    // The various actions that can be taken on the lock.
    enum Action_t
    {
        LOCK_ACTION = 0,
        UNLOCK_ACTION,
    } Action;

    // The state of the lock goes from "initiated" to "completed", to simulate
    // the movement of the bolt (rather than just locked/unlocked).
    enum State_t
    {
        kState_LockingInitiated = 0,
        kState_LockingCompleted,
        kState_UnlockingInitiated,
        kState_UnlockingCompleted,
    } State;

    // The context associated with a device timer that has been started.
    enum TimerContext_t
    {
        AUTO_LOCK_CONTEXT = 0,
        ACTUATOR_MOVEMENT_CONTEXT,
    } TimerContext;

    // Data asociated with an event posted to the AppTask event queue to handle
    // a lock/unlock on-command request (e.g. from Penja).
    struct LockOnCommandRequestData
    {
        Action_t action;
        int32_t actor;
    };

    // Initializations.
    void Init(void);

    // Called on every cycle of the Application Task event loop.
    static void EventLoopCycle(void);

    // Accessor methods
    bool IsUnlocked();
    void EnableAutoLock(bool aOn);
    void SetAutoLockDuration(uint32_t aDurationInSeconds);
    bool IsLockingActionInProgress();

    // Handlers.
    static void LockOnCommandRequestEventHandler(void * data);

    // Utility Methods
    void PostLockOnCommandRequestEvent(int32_t aActor, Action_t aAction);

private:
    // Current state of the bolt lock.
    State_t mState;

    // Whether a "long press" button event is in flight.
    bool mLongPressButtonEventInFlight;

    // Auto-lock management.
    // Tells whether auto-lock is enabled or not.
    bool mAutoLockEnabled;
    // If auto-lock is enabled, then this is set to true as soon as the
    // lock is unlocked. This is then used to cancel auto-lock if the lock is
    // locked before the auto-lock timer goes off.
    bool mAutoLockTimerArmed;
    // If auto-lock is enabled, period of time in milliseconds before auto-lock
    // kicks in after the lock is unlocked.
    uint32_t mAutoLockDurationSeconds;

    // LEDs
    LED * mConnectivityStateLEDPtr;
    LED * mLockStateLEDPtr;

    // ConnectivityState displayed on a LED.
    ConnectivityState mConnectivityState;

    // DeviceDescription client.
    DeviceDescriptionClient mDeviceDescriptionClient;

    // Device Timer managemement.
    TimerContext_t mTimerContext;
    void StartTimer(uint32_t aTimeoutMs);
    void CancelTimer(void);

    // Button event handlers.
    static void LockButtonEventHandler(void);
    static void SoftwareUpdateButtonHandler(void);
    static void FactoryResetButtonHandler(void);
    static void SendIdentifyRequestButtonHandler(void);

    // Device Timer interrupt event handler. Posts the appropriate event to AppTask.
    static void DeviceTimerEventHandler(void * p_context);

    // Device Timer event handlers called from AppTask.
    static void AutoLockTimerEventHandler(void * data);
    static void ActuatorMovementTimerEventHandler(void * data);

    // DeviceDescription client: handler for OnIdentifyResponseReceived.
    static void OnIdentifyResponseReceivedHandler(void * appState, uint64_t nodeId, const IPAddress & nodeAddr,
                                                  const IdentifyResponseMessage & respMsg);

    // Lock/Unlock actions go through an "initiated" and then "completed"
    // sequence of events to simulate the movement of the bolt lock.
    // Helper methods to support this sequence of events.
    bool InitiateAction(int32_t aActor, Action_t aAction);
    void ActionCompleted(Action_t aAction);
    void ActionInitiated(Action_t aAction, int32_t aActor);

    // Expose singleton object.
    friend DeviceController & GetDeviceController(void);
    static DeviceController sDeviceController;
};

// Exposes singleton object.
inline DeviceController & GetDeviceController(void)
{
    return DeviceController::sDeviceController;
}

#endif // DEVICE_CONTROLLER_H
