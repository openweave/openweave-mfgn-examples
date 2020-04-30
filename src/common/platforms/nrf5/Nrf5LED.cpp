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

#include "Nrf5LED.h"

#include "boards.h"

Nrf5LED::Nrf5LED(uint32_t aLEDId)
{
    mLEDId = aLEDId;
}

void Nrf5LED::On()
{
    nrf_gpio_pin_write(mLEDId, LEDS_ACTIVE_STATE ? 1 : 0);
}

void Nrf5LED::Off()
{
    nrf_gpio_pin_write(mLEDId, LEDS_ACTIVE_STATE ? 0 : 1);
}
