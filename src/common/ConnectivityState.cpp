/*
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

#include "ConnectivityState.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl::Weave::DeviceLayer;

void ConnectivityState::SetLED(LED * aLEDPtr)
{
    connectivityStateLEDPtr = aLEDPtr;
}

// FIXME: why is it "are" as opposed to "is"
void ConnectivityState::Update(bool areServiceSubscriptionsEstablished)
{
    // Because the Weave event loop is being run in a separate task, the stack must be locked
    // while these values are queried.  However we use a non-blocking lock request
    // (TryLockWeaveStack()) to avoid blocking other UI activities when the Weave
    // task is busy (e.g. with a long crypto operation).
    if (PlatformMgr().TryLockWeaveStack())
    {
        isThreadProvisioned              = ConnectivityMgr().IsThreadProvisioned();
        isThreadEnabled                  = ConnectivityMgr().IsThreadEnabled();
        isThreadAttached                 = ConnectivityMgr().IsThreadAttached();
        hasBLEConnections                = (ConnectivityMgr().NumBLEConnections() != 0);
        isPairedToAccount                = ConfigurationMgr().IsPairedToAccount();
        hasServiceConnectivity           = ConnectivityMgr().HaveServiceConnectivity();
        isServiceSubscriptionEstablished = areServiceSubscriptionsEstablished;
        PlatformMgr().UnlockWeaveStack();
    }

    // Consider the system to be "fully connected" if it has service
    // connectivity and it is able to interact with the service on a regular basis.
    bool isFullyConnected = (hasServiceConnectivity && isServiceSubscriptionEstablished);
    if (isFullyConnected)
    {
        // If system has "full connectivity", keep the LED On constantly.
        connectivityStateLEDPtr->Set(true);
    }
    else if (isThreadProvisioned && isThreadEnabled && isPairedToAccount && (!isThreadAttached || !isFullyConnected))
    {
        // Thread and service provisioned, but not attached to the thread network yet OR no
        // connectivity to the service OR subscriptions are not fully established
        // THEN blink the LED Off for a short period of time.
        connectivityStateLEDPtr->Blink(950, 50);
    }
    else if (hasBLEConnections)
    {
        // If the system has ble connection(s) uptill the stage above, THEN blink the LEDs at an even
        // rate of 100ms.
        connectivityStateLEDPtr->Blink(100, 100);
    }
    else
    {
        // No provisioing at all, blink the LED ON for a very short time.
        connectivityStateLEDPtr->Blink(50, 950);
    }
}
