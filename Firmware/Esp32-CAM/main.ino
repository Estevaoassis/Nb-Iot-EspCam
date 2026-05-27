#include <Arduino.h>
#include "esp_camera.h"
#include <HardwareSerial.h>
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"  

// --- Pinos AI-Thinker ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- Constantes do Sistema ---
const int kChunkSize = 512; 
const uint8_t kIdFoto = 0xAA;
const int kRxPin = 12;
const int kTxPin = 13;
const int kBaudRate = 115200;

// Inicializa a UART1 para comunicação com o módulo NB-IoT
HardwareSerial SerialApp(1);

void InitCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  // Clock mais baixo (10MHz) para maior estabilidade elétrica
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA; // 160x120
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro na inicializacao da camera: 0x%x\n", err);
    return;
  }
  Serial.println("Camera inicializada com sucesso.");
}

void setup() {
  // Mata o Brownout logo no início para evitar Reboot
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  // UART0: Debug no PC
  Serial.begin(kBaudRate);
  
  // UART1: Comunicação com o módulo NB-IoT
  SerialApp.begin(kBaudRate, SERIAL_8N1, kRxPin, kTxPin);
  
  delay(500);
  Serial.println("\n--- INICIANDO SISTEMA ---");

  InitCamera();

  Serial.println("Capturando imagem...");
  delay(1000);

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erro ao pegar buffer do frame!");
    return;
  }

  // Lógica de Fatiamento para NB-IoT
  int totalLen = fb->len;
  int numPacotes = (totalLen + kChunkSize - 1) / kChunkSize;
  int imageWidth = 160;

  for (int i = 0; i < numPacotes; i++) {
    int offset = i * kChunkSize;
    int size = (totalLen - offset > kChunkSize) ? kChunkSize : (totalLen - offset);

    // LOG: Mostra no monitor serial o progresso
    Serial.printf("Enviando PKT:%d/%d | OFF:%d | LEN:%d\n", i+1, numPacotes, offset, size);

    // PAYLOAD: Envia os dados pela UART1 (SerialApp) para o modem
    for (int j = 0; j < size; j++) {
      int idxGlobal = offset + j;
      uint8_t pixel = fb->buf[idxGlobal];

      // Envia no formato hexadecimal (ajuste caso seu módulo exija envio binário cru com SerialApp.write)
      if (pixel < 0x10) {
        SerialApp.print("0");
      }
      SerialApp.print(pixel, HEX);
    }
    
    // Quebra de linha indicando o fim do chunk para o modem (ajuste conforme o comando AT)
    SerialApp.println(); 
    
    // Delay crucial para o rádio NB-IoT respirar e processar o buffer
    delay(50); 
  }

  esp_camera_fb_return(fb);
  Serial.println("\n--- FIM DA TRANSMISSAO ---");
}

void loop() {
  // Delay prolongado para evitar superaquecimento
  delay(10000); 
}