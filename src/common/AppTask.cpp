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

#include "AppTask.h"

#include "app_config.h"
#include "app_timer.h"

#include "FreeRTOS.h"

#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/DeviceLayer/SoftwareUpdateManager.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Profiles/data-management/Current/DataManagement.h>

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::DeviceLayer;

#define APP_TASK_STACK_SIZE (4096)
#define APP_TASK_PRIORITY 2
#define APP_EVENT_QUEUE_SIZE 10

static TaskHandle_t sAppTaskHandle;
static QueueHandle_t sAppEventQueue;

static SemaphoreHandle_t sWeaveEventLock;

namespace nl {
namespace Weave {
namespace Profiles {
namespace DataManagement_Current {
namespace Platform {

// FIXME: Document the need for this.
void CriticalSectionEnter(void)
{
    xSemaphoreTake(sWeaveEventLock, 0);
}

void CriticalSectionExit(void)
{
    xSemaphoreGive(sWeaveEventLock);
}

} // namespace Platform
} // namespace DataManagement_Current
} // namespace Profiles
} // namespace Weave
} // namespace nl

extern void SuccessOrAbort(WEAVE_ERROR ret, const char * msg);

// Singleton.
AppTask AppTask::sAppTask;

// Static data members
AppTask::EventLoopCycleCallback_t AppTask::sEventLoopCycleCallback;

// -----------------------------------------------------------------------------
// AppTask Lifecycle

int AppTask::StartAppTask(EventLoopCycleCallback_t eventLoopCycleCallback)
{
    sEventLoopCycleCallback = eventLoopCycleCallback;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppTaskEvent));
    if (sAppEventQueue == NULL)
    {
        WeaveLogError(Support, "Failed to allocate app event queue");
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    // Start App task.
    if (xTaskCreate(AppTaskMain, "APP", APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, APP_TASK_PRIORITY, &sAppTaskHandle) !=
        pdPASS)
    {
        WeaveLogError(Support, "Failed to create the task.");
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    return WEAVE_NO_ERROR;
}

int AppTask::Init()
{
    WEAVE_ERROR ret;

    // Initialize Timer for Function Selection
    ret = app_timer_init();
    if (ret != WEAVE_NO_ERROR)
    {
        WeaveLogError(Support, "app_timer_init() failed.");
        return ret;
    }

    sWeaveEventLock = xSemaphoreCreateMutex();
    if (sWeaveEventLock == NULL)
    {
        WeaveLogError(Support, "xSemaphoreCreateMutex() failed.");
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    return WEAVE_NO_ERROR;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    WEAVE_ERROR ret;
    AppTaskEvent event;

    ret = sAppTask.Init();
    SuccessOrAbort(ret, "AppTask.Init() failed.");

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
        // Invoke the callback.
        sEventLoopCycleCallback();
    }
}

// -----------------------------------------------------------------------------
// Events Management

void AppTask::PostEvent(const AppTaskEvent * event)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, event, 1))
        {
            WeaveLogError(Support, "Failed to post event to app task event queue");
        }
    }
}

void AppTask::DispatchEvent(const AppTaskEvent * event)
{
    if (event->Handler)
    {
        event->Handler(event->Data);
    }
    else
    {
        WeaveLogError(Support, "Event received with no handler. Dropping event.");
    }
}
