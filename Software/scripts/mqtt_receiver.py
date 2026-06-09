import time
import sys
from pathlib import Path
from PIL import Image
import paho.mqtt.client as mqtt
from config import settings
from core import db_manager


# Configurações do MQTT
BROKER = "131.255.82.115"
PORT = 1883
TOPIC = "nb-iot-espcam/chunck/+"

# Estado global para armazenamento dos chunks
chunks = {}
TOTAL_CHUNKS = 74          # índices esperados: 0 até 74
EXPECTED_INDICES = set(range(TOTAL_CHUNKS))

def is_hex(s):
    """Verifica se a string/bytes consiste apenas em caracteres hexadecimais válidos."""
    try:
        if isinstance(s, bytes):
            s = s.decode('ascii', errors='ignore')
        # Remove caracteres de controle comuns
        cleaned = s.replace('\r', '').replace('\n', '').replace(' ', '')
        int(cleaned, 16)
        return len(cleaned) % 2 == 0
    except ValueError:
        return False

def processar_imagem(bytes_totais):
    """Garante que a imagem seja salva corretamente dependendo do formato enviado."""
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    caminho_foto = settings.OUTPUT_IMAGE_DIR / f"foto_mqtt_{timestamp}.jpg"
    
    # Verifica se os dados começam com o cabeçalho JPEG standard (\xff\xd8)
    eh_jpeg = bytes_totais.startswith(b'\xff\xd8')
    formato_img = "JPEG" if eh_jpeg else "Grayscale/Raw"
    
    # Salvar no MongoDB
    print(f"[Banco] Enviando imagem ({len(bytes_totais)} bytes) para o MongoDB...")
    db_manager.salvar_imagem_mongodb(bytes_totais, formato=formato_img)
    
    if not eh_jpeg and len(bytes_totais) <= 19200:
        try:
            print(f"[Processador] Detectados bytes de tons de cinza ({len(bytes_totais)} bytes).")
            if len(bytes_totais) < 19200:
                print(f"[Processador] Imagem incompleta/truncada. Preenchendo com fundo cinza...")
                # Preenche o restante da imagem com cinza médio (0x80) para torná-la visualizável
                bytes_totais = bytes_totais + b'\x80' * (19200 - len(bytes_totais))
            
            img = Image.frombytes('L', (160, 120), bytes_totais[:19200])
            img.save(caminho_foto, 'JPEG')
            print(f"[SUCESSO] Imagem Grayscale processada e salva em: {caminho_foto}")
        except Exception as e:
            print(f"[ERRO] Erro ao converter imagem Grayscale: {e}")
    else:
        try:
            # Caso seja JPEG direto (ou maior que 19200 bytes)
            with open(caminho_foto, 'wb') as f:
                f.write(bytes_totais)
            print(f"[SUCESSO] Imagem salva diretamente (JPEG) em: {caminho_foto}")
        except Exception as e:
            print(f"[ERRO] Erro ao salvar imagem direta: {e}")

def tentar_montar_imagem():
    global chunks, last_received_time
    if not chunks:
        return
        
    print(f"\n[Compilador] Iniciando montagem com {len(chunks)} chunks recebidos...")
    
    # Ordena os índices recebidos
    indices = sorted(chunks.keys())
    
    # Verifica se há falhas na sequência
    min_idx = indices[0]
    max_idx = indices[-1]
    esperados = set(range(min_idx, max_idx + 1))
    recebidos = set(indices)
    faltantes = esperados - recebidos
    
    if faltantes:
        print(f"[AVISO] Chunks ausentes na sequência: {sorted(list(faltantes))}")
    
    # Junta todos os payloads na ordem correta
    dados_totais = bytearray()
    for idx in indices:
        payload = chunks[idx]
        
        # Se os dados forem hex string, converte para binário
        if is_hex(payload):
            try:
                if isinstance(payload, bytes):
                    payload = payload.decode('ascii', errors='ignore')
                cleaned = payload.replace('\r', '').replace('\n', '').replace(' ', '')
                dados_totais.extend(bytes.fromhex(cleaned))
            except ValueError:
                print(f"[ERRO] Erro ao converter chunk {idx} de hexadecimal.")
                dados_totais.extend(payload if isinstance(payload, bytes) else payload.encode())
        else:
            # Dados binários crus
            if isinstance(payload, bytes):
                dados_totais.extend(payload)
            else:
                dados_totais.extend(payload.encode())
                
    print(f"[Compilador] Total de bytes reconstruídos: {len(dados_totais)}")
    
    if len(dados_totais) > 0:
        processar_imagem(bytes(dados_totais))
    else:
        print("[ERRO] Nenhum dado reconstruído com sucesso.")
        
    # Limpa o buffer para a próxima foto
    chunks.clear()
    last_received_time = 0

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[OK] Conectado ao Broker MQTT ({BROKER}:{PORT})!")
        client.subscribe(TOPIC)
        print(f"[MQTT] Inscrito no tópico: {TOPIC}")
    else:
        print(f"[ERRO] Falha na conexão. Código de retorno: {rc}")

def on_message(client, userdata, msg):
    global chunks
    
    # Extrai o índice do chunk a partir do tópico (ex: nb-iot-espcam/chunck/3 -> 3)
    try:
        topic_parts = msg.topic.split('/')
        chunk_idx = int(topic_parts[-1])
    except ValueError:
        print(f"[AVISO] Tópico inválido recebido: {msg.topic}")
        return
        
    payload = msg.payload
    chunks[chunk_idx] = payload
    
    recebidos = len(chunks)
    print(f"[MQTT] Recebido Chunk #{chunk_idx:02d} | Tamanho: {len(payload)} bytes | Progresso: {recebidos}/{TOTAL_CHUNKS}")
    
    # --- Condição principal: todos os 75 chunks (0-74) presentes ---
    if EXPECTED_INDICES.issubset(chunks.keys()):
        print(f"[MQTT] Todos os {TOTAL_CHUNKS} chunks recebidos! Montando imagem...")
        tentar_montar_imagem()
        return

    # --- Fallback: último chunk menor que 512 bytes indica fim da transmissão ---
    if len(payload) < 512 and 0 in chunks:
        indices_recebidos = set(chunks.keys())
        esperados_ate_aqui = set(range(0, chunk_idx + 1))
        if esperados_ate_aqui.issubset(indices_recebidos):
            print(f"[MQTT] Último chunk ({chunk_idx}) detectado com tamanho reduzido — montando imagem...")
            tentar_montar_imagem()

def main():
    print("=" * 60)
    print("        RECEPTOR DE IMAGENS MQTT ESP32-CAM (NB-IoT)")
    print("=" * 60)
    
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(BROKER, PORT, 60)
    except Exception as e:
        print(f"[ERRO] Não foi possível conectar ao broker {BROKER}: {e}")
        sys.exit(1)
        
    client.loop_start()
    
    print(f"\nAguardando {TOTAL_CHUNKS} chunks (índices 0–{TOTAL_CHUNKS - 1}) do broker.")
    print("Pressione Ctrl+C para encerrar.\n")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Finalizado.")

if __name__ == "__main__":
    main()
