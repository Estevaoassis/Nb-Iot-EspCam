from core.serial_comm import conexao
from config import settings
from datetime import datetime
from PIL import Image

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
            
            # Se for formato de tons de cinza cru (160x120 QQVGA = 19200 bytes)
            if len(bytes_imagem) == 19200:
                # Converte os pixels crus para um objeto de imagem e salva como JPEG real
                img = Image.frombytes('L', (160, 120), bytes_imagem)
                img.save(caminho, 'JPEG')
                print(f"✅ Imagem Grayscale convertida e salva como JPEG real: {caminho}")
            else:
                # Caso os bytes já sejam um JPEG comprimido direto da câmera
                with open(caminho, 'wb') as f:
                    f.write(bytes_imagem)
                print(f"✅ Imagem salva diretamente: {caminho}")
        except ValueError as e:
            print(f"Erro na conversão: {e}")
        except Exception as e:
            print(f"Erro ao salvar a imagem: {e}")
    
    conexao.fechar()

if __name__ == "__main__":
    executar()
