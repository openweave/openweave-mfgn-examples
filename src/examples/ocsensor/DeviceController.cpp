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

#include "DeviceController.h"
#include "HardwarePlatform.h"
#include "AppTask.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/DeviceLayer/SoftwareUpdateManager.h>
#include <Weave/DeviceLayer/ConfigurationManager.h>

#include <nest/trait/detector/OpenCloseTrait.h>
#include <nest/trait/security/SecurityOpenCloseTrait.h>

#include "app_timer.h"
#include "FreeRTOS.h"

//#include "BoltLockTrait.h"
#include "AppSoftwareUpdateManager.h"
#include "ConnectivityState.h"
#include "WDMFeature.h"
#include "AppTask.h"

using namespace ::nl::Weave::DeviceLayer;

using namespace Schema::Nest::Trait::Detector::OpenCloseTrait;
using namespace Schema::Nest::Trait::Security::SecurityOpenCloseTrait;

extern void SuccessOrAbort(WEAVE_ERROR ret, const char * msg);

// -----------------------------------------------------------------------------
// Initializations

// Singleton object.
DeviceController DeviceController::sDeviceController;

// -----------------------------------------------------------------------------
// DeviceManager API (see common/include/DeviceManager.h)

void DeviceController::Init()
{
    WEAVE_ERROR ret;

    // Initial state of the OC Sensor.
    mState                        = kState_Closed;
    mLongPressButtonEventInFlight = false;

    WeaveLogProgress(Support, "Initializing the Buttons");
    Button * buttons = GetHardwarePlatform().GetButtons();
    (buttons + BUTTON_1_INDEX)->SetShortPressEventHandler(SoftwareUpdateButtonHandler);
    (buttons + BUTTON_1_INDEX)->SetLongPressEventHandler(FactoryResetButtonHandler);
    (buttons + BUTTON_2_INDEX)->SetShortPressEventHandler(OCSensorButtonEventHandler);
    (buttons + BUTTON_2_INDEX)->SetLongPressEventHandler(EnableUserSelectedModeButtonHandler);

    WeaveLogProgress(Support, "Initializing the LEDs");
    LED * leds               = GetHardwarePlatform().GetLEDs();
    mConnectivityStateLEDPtr = leds + CONNECTIVITY_STATE_LED_INDEX;
    mOCSensorStateLEDPtr     = leds + OCSENSOR_STATE_LED_INDEX;
    mOCSensorStateLEDPtr->Set(!IsOpen());

    WeaveLogProgress(Support, "Initializing WDMFeature");
    ret = GetWDMFeature().Init();
    SuccessOrAbort(ret, "GetWDMFeature().Init() failed.");

    // Initialize the Manager for Software Updates (SWU aka OTA).
    GetAppSoftwareUpdateManager().Init();

    // Setup the ConnectivityState object that reflects the provisioning state on a LED.
    mConnectivityState.SetLED(mConnectivityStateLEDPtr);

    // Print the current software version.
    char currentFirmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength + 1] = { 0 };
    size_t currentFirmwareRevLen;
    ret = ConfigurationMgr().GetFirmwareRevision(currentFirmwareRev, sizeof(currentFirmwareRev), currentFirmwareRevLen);
    SuccessOrAbort(ret, "ConfigurationMgr().GetFirmwareRevision() failed.");
    WeaveLogProgress(Support, "Current Firmware Version: %s", currentFirmwareRev);
}

void DeviceController::EventLoopCycle()
{
    DeviceController & _this = GetDeviceController();
    LED * leds               = GetHardwarePlatform().GetLEDs();
    Button * buttons         = GetHardwarePlatform().GetButtons();

    // Update the button press state.
    // If LongPressInFlight state changes, then update LEDs state accordingly
    // (All LEDs blink when in "LongPressInFlight".)
    bool startLongPressInFlight = false;
    bool endLongPressInFlight   = false;
    bool allButtonsReleased     = true;    // Provides check for button released before Long_Completed state.
    for (int idx = 0; idx < PLATFORM_BUTTONS_COUNT; idx++)
    {
        Button::ButtonPressState buttonPressState = (buttons + idx)->UpdateButtonPressState();
        if (buttonPressState != Button::kButtonPressState_Inactive)
        {
            allButtonsReleased = false;
        }
        if (buttonPressState == Button::kButtonPressState_Long_Started)
        {
            if (!_this.mLongPressButtonEventInFlight)
            {
                _this.mLongPressButtonEventInFlight = true;
                startLongPressInFlight              = true;
            }
        }
        else if ((buttonPressState == Button::kButtonPressState_Long_Completed) ||
                 ((idx == PLATFORM_BUTTONS_COUNT-1) && allButtonsReleased))
        {
            if (_this.mLongPressButtonEventInFlight)
            {
                _this.mLongPressButtonEventInFlight = false;
                endLongPressInFlight                = true;
            }
        }
    }

    if (startLongPressInFlight)
    {
        // Indicate that long press is in flight on the LEDs by blinking all of them.
        // Turn off all LEDs before starting blink to make sure blink is co-ordinated.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Set(false);
        }
        // Blink all LEDs.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Blink(500);
        }
    }
    else if (endLongPressInFlight)
    {
        // Reset all the LEDs.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Set(false);
        }

        // Set the OC Sensor status LED back to show state of lock.
        _this.mOCSensorStateLEDPtr->Set(!_this.IsOpen());

        // Update the provisioning state shown on LED.
        _this.mConnectivityState.Update(GetWDMFeature().AreServiceSubscriptionsEstablished());
    }
    else if (!_this.mLongPressButtonEventInFlight)
    {
        // Update the provisioning state shown on LED.
        static uint16_t loopCount = 0;
        ++loopCount;
        if ((loopCount % 100) == 0)
        {
            _this.mConnectivityState.Update(GetWDMFeature().AreServiceSubscriptionsEstablished());
        }
    }

    // Animate the LEDs.
    for (int idx = 0; idx < PLATFORM_LEDS_COUNT; idx++)
    {
        (leds + idx)->Animate();
    }
}

// -----------------------------------------------------------------------------
// Accessors

bool DeviceController::IsOpen()
{
    return (mState == kState_Open);
}

// -----------------------------------------------------------------------------
// Event Handlers

void DeviceController::OCSensorButtonEventHandler()
{
    WeaveLogProgress(Support, "DeviceController::OCSensorButtonEventHandler");
    DeviceController & _this    = GetDeviceController();
    _this.mState                = (_this.IsOpen()) ? kState_Closed : kState_Open;
    int32_t openCloseTraitState = (!_this.IsOpen()) ? OPEN_CLOSE_STATE_CLOSED : OPEN_CLOSE_STATE_OPEN;
    _this.mOCSensorStateLEDPtr->Set(!_this.IsOpen());
    GetWDMFeature().GetSecurityOpenCloseTraitDataSource().HandleStateChange(openCloseTraitState);
}

void DeviceController::SoftwareUpdateButtonHandler()
{
    WeaveLogProgress(Support, "Manual Software Update Triggered");
    GetAppSoftwareUpdateManager().CheckNow();
}

void DeviceController::FactoryResetButtonHandler()
{
    WeaveLogProgress(Support, "Factory Reset Triggered.");
    nl::Weave::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

void DeviceController::EnableUserSelectedModeButtonHandler()
{
    WeaveLogProgress(Support, "Enable User Selected Mode Triggered.");
    ConnectivityMgr().SetUserSelectedModeTimeout(USER_SELECTED_MODE_TIMEOUT_MS);
    ConnectivityMgr().SetUserSelectedMode(true);
}
