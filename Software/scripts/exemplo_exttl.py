from core.serial_comm import conexao
from config import settings
from datetime import datetime

def executar():
    print("Iniciando monitoramento...")
    conexao.conectar()
    
    # 1. Captura o texto hex
    hex_string = conexao.ler_hex_completo()
    
    # 2. Converte para bytes e salva
    if hex_string:
        try:
            bytes_imagem = bytes.fromhex(hex_string)
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            caminho = settings.OUTPUT_IMAGE_DIR / f"foto_{timestamp}.jpg"
            
            with open(caminho, 'wb') as f:
                f.write(bytes_imagem)
            print(f"✅ Imagem salva: {caminho}")
        except ValueError as e:
            print(f"Erro na conversão: {e}")
    
    conexao.fechar()

