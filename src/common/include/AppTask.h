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

#ifndef APP_TASK_H
#define APP_TASK_H

/**
 * The FreeRTOS Application Task.
 */
class AppTask
{
public:
    /**
     * When events are triggered outside of the application task (e.g. hardware events such as
     * button press/release, timer that goes off; weave custom commands), the code used to
     * process these events has to execute within the application task.
     * [FIXME: provide additional details on this.]
     * The mechanism used to make this possible is to simply post an AppTaskEvent
     * to the event queue of the application task. That event has two components:
     * 1) information about the specific hardware event (void * data),
     * and 2) the callback function that is to be invoked (AppTaskEventHandler_t).
     */
    typedef void (*AppTaskEventHandler_t)(void * eventData);

    struct AppTaskEvent
    {
        AppTaskEventHandler_t Handler;
        void * Data;
    };

    // The function called at every cycle of the AppTask event loop.
    // This is a static method on the DeviceController so it can update the state
    // of LEDs, Buttons, etc.
    typedef void (*EventLoopCycleCallback_t)(void);

    // Called my 'main' to start the AppTask.
    int StartAppTask(EventLoopCycleCallback_t callback);

    // Posts an event on the AppTask event queue.
    void PostEvent(const AppTaskEvent * event);

private:
    // Callback method called at every event loop cycle.
    static EventLoopCycleCallback_t sEventLoopCycleCallback;

    int Init();
    static void AppTaskMain(void * pvParameter);
    void DispatchEvent(const AppTaskEvent * event);

    // Singleton.
    friend AppTask & GetAppTask(void);
    static AppTask sAppTask;
};

// Singleton.
inline AppTask & GetAppTask(void)
{
    return AppTask::sAppTask;
}

#endif // APP_TASK_H
