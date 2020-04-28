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

#ifndef APP_SOFTWARE_UPATE_MANAGER_H
#define APP_SOFTWARE_UPATE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/DeviceLayer/SoftwareUpdateManager.h>

#define SWU_INTERVAl_WINDOW_MIN_MS (23 * 60 * 60 * 1000) // 23 hours
#define SWU_INTERVAl_WINDOW_MAX_MS (24 * 60 * 60 * 1000) // 24 hours

/**
 * Manages all Software Update functionality.
 */
class AppSoftwareUpdateManager
{
    typedef ::nl::Weave::DeviceLayer::SoftwareUpdateManager SoftwareUpdateManager;

public:
    static void Init(void);
    static bool IsInProgress(void);
    static void Abort(void);
    static void CheckNow(void);

private:
    static void InstallEventHandler(void * data);
    static void HandleSoftwareUpdateEvent(void * apAppState, SoftwareUpdateManager::EventType aEvent,
                                          const SoftwareUpdateManager::InEventParam & aInParam,
                                          SoftwareUpdateManager::OutEventParam & aOutParam);

    // Singleton.
    friend AppSoftwareUpdateManager & GetAppSoftwareUpdateManager(void);
    static AppSoftwareUpdateManager sAppSoftwareUpdateManager;
};

// Singleton.
inline AppSoftwareUpdateManager & GetAppSoftwareUpdateManager(void)
{
    return AppSoftwareUpdateManager::sAppSoftwareUpdateManager;
}

#endif // APP_SOFTWARE_UPATE_MANAGER_H