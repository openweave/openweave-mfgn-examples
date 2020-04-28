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

#include "AppTask.h"
#include "Button.h"
#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

void Button::Init()
{
    mButtonPressState     = kButtonPressState_Inactive;
    mButtonPressStartedMs = 0;

    mShortPressEventHandler = nullptr;
    mLongPressEventHandler  = nullptr;
}

void Button::SetShortPressEventHandler(ButtonEventHandler_t handler)
{
    mShortPressEventHandler = handler;
}

void Button::SetLongPressEventHandler(ButtonEventHandler_t handler)
{
    mLongPressEventHandler = handler;
}

// -----------------------------------------------------------------------------
// PhysicalButtonEventHandler

void Button::PhysicalButtonEventHandler(void * eventData)
{
    PhysicalButtonAppTaskEventData * data     = static_cast<PhysicalButtonAppTaskEventData *>(eventData);
    Button * _this                            = static_cast<Button *>(data->ButtonPtr);
    PhysicalButtonAction physicalButtonAction = data->Action;
    WeaveLogProgress(Support, "Button::PhysicalButtonEventHandler: Action [%d].", physicalButtonAction);

    if (physicalButtonAction == kPhysicalButtonAction_Press)
    {
        _this->StartButtonPress();
    }
    else if (physicalButtonAction == kPhysicalButtonAction_Release)
    {
        ButtonPressState currentButtonPressState = _this->mButtonPressState;
        _this->EndButtonPress();
        switch (currentButtonPressState)
        {
        case kButtonPressState_Inactive:
            WeaveLogError(Support,
                          "ERROR: Should never be in state kButtonPressState_Inactive when a button release is triggered.");
            break;
        case kButtonPressState_Short:
            WeaveLogProgress(Support, "kButtonPressState_Short", physicalButtonAction);
            if (_this->mShortPressEventHandler != nullptr)
            {
                _this->mShortPressEventHandler();
            }
            break;
        case kButtonPressState_Long_Started:
            // Nothing to do. Long press canceled before it kicked in.
            break;
        case kButtonPressState_Long_Completed:
            WeaveLogProgress(Support, "kButtonPressState_Long_Completed", physicalButtonAction);
            if (_this->mLongPressEventHandler != nullptr)
            {
                _this->mLongPressEventHandler();
            }
            break;
        }
    }
    else
    {
        WeaveLogError(Support, "ERROR: This physical button action is not supported: [%d].", physicalButtonAction);
    }
}

// -----------------------------------------------------------------------------
// Long Press Processing

void Button::StartButtonPress()
{
    WeaveLogProgress(Support, "Button::StartButtonPress()");
    mButtonPressState     = kButtonPressState_Short;
    mButtonPressStartedMs = ::nl::Weave::System::Platform::Layer::GetClock_MonotonicMS();
}

Button::ButtonPressState Button::UpdateButtonPressState(void)
{
    if (mButtonPressState == kButtonPressState_Inactive)
        return mButtonPressState;

    uint32_t now = ::nl::Weave::System::Platform::Layer::GetClock_MonotonicMS();
    if (now - mButtonPressStartedMs >= LONG_PRESS_ACTIVATION_COMPLETE_MS)
    {
        WeaveLogProgress(Support, "Moving to kButtonPressState_Long_Completed");
        mButtonPressState = kButtonPressState_Long_Completed;
    }
    else if (now - mButtonPressStartedMs >= LONG_PRESS_ACTIVATION_START_MS)
    {
        WeaveLogProgress(Support, "Moving to kButtonPressState_Long_Started");
        mButtonPressState = kButtonPressState_Long_Started;
    }
    return mButtonPressState;
}

void Button::EndButtonPress()
{
    WeaveLogProgress(Support, "Button::EndButtonPress()");
    mButtonPressState     = kButtonPressState_Inactive;
    mButtonPressStartedMs = 0;
}
