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

#ifndef APP_TIMER_EFR32_H
#define APP_TIMER_EFR32_H

#include <FreeRTOS.h>
#include <timers.h>


#define APP_TIMER_DEF(handle) xTimerHandle(handle)

typedef uint32_t ret_code_t;

#define APP_TIMER_MODE_SINGLE_SHOT 0

ret_code_t app_timer_init(void);

ret_code_t app_timer_create(xTimerHandle *handle, uint8_t timerMode, void (* timerEventHandler)(void * context));

ret_code_t app_timer_start(xTimerHandle handle, uint32_t timerTicks, void * context);

ret_code_t app_timer_stop(xTimerHandle handle);


#endif // APP_TIMER_EFR32_H
