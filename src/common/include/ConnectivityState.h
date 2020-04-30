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

#ifndef CONNECTIVITY_STATE_H
#define CONNECTIVITY_STATE_H

#include "LED.h"

/**
 * Shows the current connectivity state via LED.
 *
 * FIXME: Provide an overview of connectivity, or give a link to a doc that clearly explains it.
 *
 * The provisioning/connectivity state of the device is shown in one of the following ways:
 *
 *  Not provisioned at all: Short Flash On (50ms on/950ms off)
 *    The device is in an unprovisioned (unpaired) state and is waiting for a
 *    commissioning application to connect.
 *  BLE connection with commissioning application: Rapid Even Flashing (100ms on/100ms off)
 *    The device is in an unprovisioned state and a commissioning application is connected via BLE.
 *  Fully provisioned but no connectivity: Short Flash Off (950ms on/50ms off)
 *    The device is fully provisioned, but does not yet have full network (Thread)
 *    or service connectivity.
 *  Fullly provisioned and connected: Solid On
 *    The device is fully provisioned and has full network and service connectivity.
 */
class ConnectivityState
{
public:
    /** Sets the LED used to show the current Connectivity state. */
    void SetLED(LED * aLEDPtr);

    /**
     * Collects provisioning (connectivity and configuration) state from the Weave stack
     * and updates the LED if that state has changed.
     * This should be called at each cycle of the event loop of the application task.
     */
    void Update(bool areServiceSubscriptionsEstablished);

private:
    // The LED that shows the provisioning state.
    LED * connectivityStateLEDPtr;

    // State of the device with respect to the Thread network.
    bool isThreadProvisioned = false;
    bool isThreadEnabled     = false;
    bool isThreadAttached    = false;

    bool hasBLEConnections = false;

    bool isPairedToAccount                = false;
    bool isServiceSubscriptionEstablished = false;
    bool hasServiceConnectivity           = false;
};

#endif // CONNECTIVITY_STATE_H
