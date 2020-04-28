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

#ifndef NRF5_LED_H
#define NRF5_LED_H

#include "PlatformLED.h"

/**
 * Base class that encapsulates the functionality of a LED.
 */
class Nrf5LED : public PlatformLED
{
public:
    Nrf5LED(uint32_t ledId);
    void On(void);
    void Off(void);

private:
    uint32_t mLEDId;
};

#endif // NRF5_LED_H
