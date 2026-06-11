#ifndef __HT_FSM_H__
#define __HT_FSM_H__

#include "stdint.h"
#include "main.h"
#include "HT_MQTT_Api.h"
#include "MQTTFreeRTOS.h"
#include "bsp.h"
#include "HT_GPIO_Api.h"
#include "cmsis_os2.h"
#include "MQTTClient.h"
#include "HT_LED_Task.h"
#include "HT_USART_Demo.h"

/* Defines  ------------------------------------------------------------------*/
#define HT_MQTT_KEEP_ALIVE_INTERVAL 240
#define HT_MQTT_VERSION 4

#if MQTT_TLS_ENABLE == 1
#define HT_MQTT_PORT 8883
#else
#define HT_MQTT_PORT 1883
#endif

#define HT_MQTT_SEND_TIMEOUT 60000
#define HT_MQTT_RECEIVE_TIMEOUT 60000
#define HT_MQTT_BUFFER_SIZE 1024

/* Typedefs  ------------------------------------------------------------------*/

typedef enum
{
    HT_CONNECTED = 0,
    HT_NOT_CONNECTED
} HT_ConnectionStatus;

typedef enum
{
    HT_WAIT_FOR_UART_CHUNK = 0,
    HT_PUBLISH_CHUNK
} HT_FSM_States;

/* Functions ------------------------------------------------------------------*/

void HT_Fsm(void);

#endif /* __HT_FSM_H__ */
