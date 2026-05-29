import serial
import time
from config import settings

class ModemSerial:
    def __init__(self):
        self.ser = None

    def conectar(self):
        self.ser = serial.Serial(settings.SERIAL_PORT, settings.BAUDRATE, timeout=0.1)
        self.ser.reset_input_buffer()
        print(f"[Serial] Porta {settings.SERIAL_PORT} aberta. Aguardando hex...")

    def ler_hex_completo(self, timeout_silencio=2.0):
        """Coleta o stream de texto hexadecimal e retorna a string bruta."""
        dados_hex = ""
        tempo_ultimo_dado = time.time()
        
        while True:
            if self.ser.in_waiting > 0:
                # Lê e limpa o lixo (quebras de linha, espaços, etc)
                pedaco = self.ser.read(self.ser.in_waiting).decode('ascii', errors='ignore')
                dados_hex += pedaco.replace('\n', '').replace('\r', '').replace(' ', '')
                tempo_ultimo_dado = time.time()
            
            # Timeout para finalizar a leitura
            if (time.time() - tempo_ultimo_dado) > timeout_silencio and len(dados_hex) > 0:
                break
        return dados_hex

    def fechar(self):
        if self.ser: self.ser.close()

conexao = ModemSerial()