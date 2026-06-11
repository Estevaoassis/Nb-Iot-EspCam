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

#include "HT_USART_Demo.h"
#include <ctype.h>

extern uint8_t image_buffer[20000];
extern uint32_t image_len;
extern uint32_t last_rx_time;

extern USART_HandleTypeDef huart0;

uint8_t uart_chunk_buffer_1[USART_CHUNK_BUFFER_SIZE] = {0};
uint8_t uart_chunk_buffer_2[USART_CHUNK_BUFFER_SIZE] = {0};

volatile BufferState buf1_state = BUF_FREE;
volatile BufferState buf2_state = BUF_FREE;

volatile uint16_t buf1_idx = 0;
volatile uint16_t buf2_idx = 0;
static uint8_t active_buf_idx = 1; // 1 or 2

// Small temporary buffer for the buggy HAL IRQ receiver
static uint8_t temp_rx_buf[64] = {0};

static uint8_t hex_to_val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    return 0;
}

uint16_t append_hex_chars(uint8_t *src, uint16_t len, uint8_t *dst)
{
    uint16_t copied = 0;
    char high_nibble = 0;
    bool has_high = false;
    
    for (uint16_t i = 0; i < len; i++)
    {
        char c = src[i];
        if (isxdigit((int)c))
        {
            if (!has_high)
            {
                high_nibble = c;
                has_high = true;
            }
            else
            {
                uint8_t val = (hex_to_val(high_nibble) << 4) | hex_to_val(c);
                dst[copied++] = val;
                has_high = false;
            }
        }
    }
    return copied;
}

void HT_USART_Callback(uint32_t event)
{
    if (event & (ARM_USART_EVENT_RECEIVE_COMPLETE | ARM_USART_EVENT_RX_TIMEOUT | 
                 ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_RX_FRAMING_ERROR | 
                 ARM_USART_EVENT_RX_PARITY_ERROR))
    {
        // Read LSR to clear any hardware overrun or framing error flags and prevent lockups
        volatile uint32_t lsr = huart0.reg->LSR;
        (void)lsr;

        // Find the number of bytes read in the temporary buffer
        uint16_t len = 64;
        while (len > 0 && temp_rx_buf[len - 1] == 0)
        {
            len--;
        }

        if (len > 0)
        {
            bool chunk_complete = false;
            
            // Check if a newline character is in the received bytes
            for (uint16_t i = 0; i < len; i++)
            {
                if (temp_rx_buf[i] == '\n')
                {
                    chunk_complete = true;
                    break;
                }
            }

            // Copy to active buffer
            if (active_buf_idx == 1)
            {
                if (buf1_idx + len < USART_CHUNK_BUFFER_SIZE)
                {
                    memcpy(&uart_chunk_buffer_1[buf1_idx], temp_rx_buf, len);
                    buf1_idx += len;
                }
                
                if (chunk_complete)
                {
                    buf1_state = BUF_READY;
                    if (buf2_state == BUF_FREE)
                    {
                        active_buf_idx = 2;
                        buf2_idx = 0;
                        buf2_state = BUF_RECEIVING;
                    }
                }
            }
            else // active_buf_idx == 2
            {
                if (buf2_idx + len < USART_CHUNK_BUFFER_SIZE)
                {
                    memcpy(&uart_chunk_buffer_2[buf2_idx], temp_rx_buf, len);
                    buf2_idx += len;
                }
                
                if (chunk_complete)
                {
                    buf2_state = BUF_READY;
                    if (buf1_state == BUF_FREE)
                    {
                        active_buf_idx = 1;
                        buf1_idx = 0;
                        buf1_state = BUF_RECEIVING;
                    }
                }
            }

            // Clear temporary buffer for the next interrupt
            memset(temp_rx_buf, 0, sizeof(temp_rx_buf));
            
            last_rx_time = osKernelGetTickCount();
        }

        // Restart RX into the temporary buffer immediately!
        HAL_USART_Receive_IT(temp_rx_buf, sizeof(temp_rx_buf));
    }
}

void HT_USART_StartRX(void)
{
    buf1_state = BUF_RECEIVING;
    buf2_state = BUF_FREE;
    buf1_idx = 0;
    buf2_idx = 0;
    active_buf_idx = 1;
    
    memset(uart_chunk_buffer_1, 0, USART_CHUNK_BUFFER_SIZE);
    memset(uart_chunk_buffer_2, 0, USART_CHUNK_BUFFER_SIZE);
    memset(temp_rx_buf, 0, sizeof(temp_rx_buf));

    HAL_USART_IRQnEnable(&huart0, (USART_IER_RX_DATA_REQ_Msk | USART_IER_RX_TIMEOUT_Msk | USART_IER_RX_LINE_STATUS_Msk));
    HAL_USART_Receive_IT(temp_rx_buf, sizeof(temp_rx_buf));
}

void HT_USART_StopRX(void)
{
    HAL_USART_IRQnDisable(&huart0, (USART_IER_RX_DATA_REQ_Msk | USART_IER_RX_TIMEOUT_Msk | USART_IER_RX_LINE_STATUS_Msk));
}

void HT_USART_InitApp(void)
{
    // Enable UART0 Clock
    HT_GPR_ClockDisable(GPR_UART0FuncClk);
    HT_GPR_SetClockSrc(GPR_UART0FuncClk, GPR_UART0ClkSel_26M);
    HT_GPR_ClockEnable(GPR_UART0FuncClk);
    HT_GPR_SWReset(GPR_ResetUART0Func);

    // Initialize UART0 explicitly since HAL_USART_InitPrint is now using huart1 for logs
    HAL_USART_Initialize(HT_USART_Callback, &huart0);
    HAL_USART_PowerControl(ARM_POWER_FULL, &huart0);
    HAL_USART_Control(ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE, 115200, &huart0);
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
