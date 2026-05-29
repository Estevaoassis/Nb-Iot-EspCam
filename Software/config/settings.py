from pathlib import Path

# Configurações da comunicação Serial / NB-IoT
SERIAL_PORT = "COM3"  # No Linux/Mac seria '/dev/ttyUSB0'
BAUDRATE = 115200
TIMEOUT = 5000.0

# Configurações de Imagem (Caminho Genérico e Automático)
# Descobre automaticamente a pasta 'Software' onde o projeto está rodando
BASE_DIR = Path(__file__).resolve().parent.parent

# Cria o caminho para a pasta 'output' dentro de 'Software'
OUTPUT_IMAGE_DIR = BASE_DIR / "output"

# Garante que a pasta exista (se não existir, o Python cria ela na hora)
OUTPUT_IMAGE_DIR.mkdir(parents=True, exist_ok=True)