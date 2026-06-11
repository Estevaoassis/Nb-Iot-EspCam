#include "HT_Fsm.h"
#include <ctype.h>
#include <stdbool.h>

static MQTTClient mqttClient;
static Network mqttNetwork;

static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE] = {0};
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE] = {0};

static const char clientID[] = {"SIP_HTNB32L-XXX-ESPCAM"};
static const char username[] = {""};
static const char password[] = {""};

// MQTT broker host address
static const char addr[] = {"131.255.82.115"};
static char topic[50] = {0};

volatile HT_FSM_States state = HT_WAIT_FOR_UART_CHUNK;
static uint32_t chunk_sequence = 0;

uint8_t image_buffer[20000];
uint32_t image_len = 0;
uint32_t last_rx_time = 0;

static StaticTask_t yield_thread;
static uint8_t yieldTaskStack[1024 * 4];

static void HT_YieldThread(void *arg)
{
    while (1)
    {
        MQTTYield(&mqttClient, 10);
    }
}

static void HT_Yield_Thread(void *arg)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    memset(yieldTaskStack, 0xA5, LED_TASK_STACK_SIZE);
    task_attr.name = "yield_thread";
    task_attr.stack_mem = yieldTaskStack;
    task_attr.stack_size = LED_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &yield_thread;
    task_attr.cb_size = sizeof(StaticTask_t);

    osThreadNew(HT_YieldThread, NULL, &task_attr);
}

static void HT_FSM_MQTTReconnect(void)
{
    printf("[MQTT] Connection lost or inactive. Reconnecting...\n");
    
    // 1. Close old socket safely to avoid descriptor leaks
    mqttNetwork.disconnect(&mqttNetwork);
    
    // 2. Re-initialize network structure
    NetworkInit(&mqttNetwork);
    
    // 3. Set new socket timeouts
    if (NetworkSetConnTimeout(&mqttNetwork, HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT) != 0)
    {
        printf("[MQTT] NetworkSetConnTimeout failed\n");
        return;
    }
    
    // 4. Re-establish TCP connection to the broker
    if (NetworkConnect(&mqttNetwork, (char *)addr, HT_MQTT_PORT) != 0)
    {
        printf("[MQTT] NetworkConnect reconnect failed\n");
        return;
    }
    
    // 5. Connect MQTT protocol level
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    connectData.MQTTVersion = HT_MQTT_VERSION;
    connectData.clientID.cstring = (char *)clientID;
    connectData.username.cstring = (char *)username;
    connectData.password.cstring = (char *)password;
    connectData.keepAliveInterval = HT_MQTT_KEEP_ALIVE_INTERVAL;
    connectData.will.qos = QOS0;
    connectData.cleansession = false;
    
    if (MQTTConnect(&mqttClient, &connectData) != 0)
    {
        printf("[MQTT] MQTTConnect protocol reconnect failed!\n");
        mqttClient.isconnected = 0;
    }
    else
    {
        printf("[MQTT] MQTT Reconnection SUCCESS!\n");
        mqttClient.isconnected = 1;
    }
}

static HT_ConnectionStatus HT_FSM_MQTTConnect(void)
{
    if (HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)addr, HT_MQTT_PORT, HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT,
                        (char *)clientID, (char *)username, (char *)password, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL, mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE))
    {
        return HT_NOT_CONNECTED;
    }

    printf("MQTT Connection Success!\n");
    return HT_CONNECTED;
}


void HT_Fsm(void)
{
    if (HT_FSM_MQTTConnect() == HT_NOT_CONNECTED)
    {
        printf("\n MQTT Connection Error!\n");
        while (1)
            ;
    }

    HT_LED_GreenLedTask(NULL);
    HT_Yield_Thread(NULL);

    printf("Executing EspCam fsm...\n");
    HT_USART_StartRX();
    last_rx_time = osKernelGetTickCount();

    while (1)
    {
        switch (state)
        {
        case HT_WAIT_FOR_UART_CHUNK:
            if (buf1_state == BUF_READY)
            {
                if (image_len < 20000)
                {
                    uint16_t space_left = 20000 - image_len;
                    uint16_t copied = append_hex_chars(uart_chunk_buffer_1, buf1_idx, &image_buffer[image_len]);
                    if (copied > space_left) copied = space_left;
                    
                    image_len += copied;
                }
                memset(uart_chunk_buffer_1, 0, USART_CHUNK_BUFFER_SIZE);
                buf1_idx = 0; // Reset index to prevent deadlock
                buf1_state = BUF_FREE;
            }

            if (buf2_state == BUF_READY)
            {
                if (image_len < 20000)
                {
                    uint16_t space_left = 20000 - image_len;
                    uint16_t copied = append_hex_chars(uart_chunk_buffer_2, buf2_idx, &image_buffer[image_len]);
                    if (copied > space_left) copied = space_left;
                    
                    image_len += copied;
                }
                memset(uart_chunk_buffer_2, 0, USART_CHUNK_BUFFER_SIZE);
                buf2_idx = 0; // Reset index to prevent deadlock
                buf2_state = BUF_FREE;
            }

            if (image_len > 0 && (osKernelGetTickCount() - last_rx_time > 4000))
            {
                // Timeout de 4 segundos sem dados da câmera = Recepção Completa!
                state = HT_PUBLISH_CHUNK;
            }
            break;

        case HT_PUBLISH_CHUNK:
            printf("\n--- Image Reception Complete! Total size: %lu bytes. Starting MQTT Upload ---\n", image_len);

            // Ensure MQTT client is connected before commencing upload loop
            uint8_t retry_count = 0;
            while (!mqttClient.isconnected && retry_count < 3)
            {
                HT_FSM_MQTTReconnect();
                if (!mqttClient.isconnected)
                {
                    printf("[MQTT] Reconnect attempt %d failed. Retrying in 2 seconds...\n", retry_count + 1);
                    osDelay(2000);
                    retry_count++;
                }
            }
            
            if (!mqttClient.isconnected)
            {
                printf("[MQTT] CRITICAL: Reconnection failed completely. Aborting current frame.\n");
                
                // Discard and go back to listening
                image_len = 0;
                last_rx_time = osKernelGetTickCount();
                HT_USART_StartRX();
                state = HT_WAIT_FOR_UART_CHUNK;
                break;
            }

            uint32_t published = 0;
            chunk_sequence = 0;

            while (published < image_len)
            {
                uint32_t chunk_size = image_len - published;
                // Dividimos em pedaços de no máximo 256 bytes binários
                // que serão convertidos em 512 caracteres hexadecimais ao enviar
                if (chunk_size > 256)
                {
                    chunk_size = 256;
                }

                // Converte o pedaço binário de volta para string HEX "on-the-fly" antes de enviar
                uint8_t temp_hex[520];
                for (uint32_t i = 0; i < chunk_size; i++)
                {
                    const char hex_digits[] = "0123456789ABCDEF";
                    uint8_t val = image_buffer[published + i];
                    temp_hex[i * 2] = hex_digits[val >> 4];
                    temp_hex[i * 2 + 1] = hex_digits[val & 0x0F];
                }
                uint32_t hex_len = chunk_size * 2;

                memset(topic, 0, sizeof(topic));
                sprintf(topic, "nb-iot-espcam/chunck/%lu", chunk_sequence);

                // Envia o chunk como texto HEX via MQTT (exatamente como vinha da UART!)
                HT_MQTT_Publish(&mqttClient, topic, temp_hex, hex_len, QOS0, 0, 0, 0);
                printf("Published chunk %lu to topic %s (size: %lu HEX chars)\n", chunk_sequence, topic, hex_len);

                published += chunk_size;
                chunk_sequence++;

                // Delay para dar fôlego ao modem NB-IoT
                osDelay(500);
            }

            printf("\n--- All chunks published! Waiting for next image... ---\n\n");

            image_len = 0;
            last_rx_time = osKernelGetTickCount();
            HT_USART_StartRX();
            state = HT_WAIT_FOR_UART_CHUNK;
            break;

        default:
            break;
        }
        osDelay(10); // Small delay to avoid tight looping
    }
}
