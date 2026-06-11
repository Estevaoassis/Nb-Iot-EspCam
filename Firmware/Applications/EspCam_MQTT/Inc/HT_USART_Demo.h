/**
 *
 * Copyright (c) 2024 HT Micron Semicondutores S.A.
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

#ifndef __HT_USART_DEMO_H__
#define __HT_USART_DEMO_H__

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "main.h"
#include "htnb32lxxx_hal_usart.h"

#define USART_CHUNK_BUFFER_SIZE 1150

typedef enum {
    BUF_FREE = 0,
    BUF_RECEIVING,
    BUF_READY
} BufferState;

extern uint8_t uart_chunk_buffer_1[USART_CHUNK_BUFFER_SIZE];
extern uint8_t uart_chunk_buffer_2[USART_CHUNK_BUFFER_SIZE];
extern volatile BufferState buf1_state;
extern volatile BufferState buf2_state;
extern volatile uint16_t buf1_idx;
extern volatile uint16_t buf2_idx;

void HT_USART_InitApp(void);
void HT_USART_StartRX(void);
void HT_USART_StopRX(void);
uint16_t append_hex_chars(uint8_t *src, uint16_t len, uint8_t *dst);

#endif /* __HT_USART_DEMO_H__ */

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
