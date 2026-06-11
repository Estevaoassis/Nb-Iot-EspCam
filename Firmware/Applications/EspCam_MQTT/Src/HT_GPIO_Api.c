/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HT_GPIO_Api.h"
#include "ic_qcx212.h"
#include "slpman_qcx212.h"
#include <stdio.h>
#include "string.h"

void HT_GPIO_WritePin(uint16_t pin, uint32_t instance, uint16_t value)
{
    // Write the value in the GPIO pin
    GPIO_PinWrite(instance, 1 << pin, (value ? 1 << pin : 0));
}

void HT_GPIO_ButtonInit(void)
{
    // Buttons removed as requested.
}

void HT_GPIO_LedInit(void)
{
    pad_config_t padConfig;
    gpio_pin_config_t config;

    // Set alternate function to GPIO
    PAD_GetDefaultConfig(&padConfig);

    padConfig.mux = LED_PAD_ALT_FUNC;
    PAD_SetPinConfig(BLUE_LED_PAD_ID, &padConfig);
    PAD_SetPinConfig(WHITE_LED_PAD_ID, &padConfig);
    PAD_SetPinConfig(GREEN_LED_PAD_ID, &padConfig);

    // Configure GPIO direction to output
    config.pinDirection = GPIO_DirectionOutput;
    config.misc.initOutput = 0;

    // Configure pins with right GPIOs
    GPIO_PinConfig(BLUE_LED_INSTANCE, BLUE_LED_PIN, &config);
    GPIO_PinConfig(WHITE_LED_INSTANCE, WHITE_LED_PIN, &config);
    GPIO_PinConfig(GREEN_LED_INSTANCE, GREEN_LED_PIN, &config);

    // Set IO Voltage to 3.3V
    slpManNormalIOVoltSet(IOVOLT_3_30V);
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/