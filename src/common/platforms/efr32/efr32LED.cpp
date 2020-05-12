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

#include "Efr32LED.h"

#include "bsp.h"

Efr32LED::Efr32LED(uint32_t aLEDId)
{
    mLEDId = aLEDId;
}

void Efr32LED::On()
{
    BSP_LedSet(mLEDId);
}

void Efr32LED::Off()
{
    BSP_LedClear(mLEDId);
}
