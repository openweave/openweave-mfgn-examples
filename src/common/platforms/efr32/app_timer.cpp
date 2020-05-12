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

#include "app_timer.h"

ret_code_t app_timer_init(void)
{
    // Provided for efr32 build compatibility.

    return 0; // Just return Ok
}

ret_code_t app_timer_create(xTimerHandle *handle, uint8_t timerMode, void (* timerEventHandler)(void * context))
{
    *handle = xTimerCreate("tmr",   // Just a text name, not used by the RTOS kernel.
                           1,       // == default timer period (mS).
                           false,   // no timer reload (==one-shot).
                           NULL,    // timerId context = NULL.
                           (TimerCallbackFunction_t)timerEventHandler  // timer callback fn.
    );

    if (*handle == NULL)
    {
        return 1;  // Error.
    }

    return 0;  // Ok.
}

ret_code_t app_timer_start(xTimerHandle handle, uint32_t timerTicks, void * context)
{
    if (xTimerIsTimerActive(handle))
    {
        if (xTimerStop(handle, 0) == pdFAIL)
        {
            return 1;  // Error.
        }
    }

    // timer is not active, change its period to required value (== restart).
    // FreeRTOS- Block for a maximum of 100 ticks if the change period command
    // cannot immediately be sent to the timer command queue.
    if (xTimerChangePeriod(handle, timerTicks, 100) != pdPASS)
    {
        return 1;  // Error.
    }

    return 0;  // Ok.
}

ret_code_t app_timer_stop(xTimerHandle handle)
{
    if (xTimerStop(handle, 0) == pdFAIL)
    {
        return 1;  // Error.
    }

    return 0;  // Ok.
}
