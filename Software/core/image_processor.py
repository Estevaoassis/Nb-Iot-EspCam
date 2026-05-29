import numpy as np
from PIL import Image

def processar_imagem_para_transmissao(caminho_imagem):
    """Carrega uma imagem, converte em matriz NumPy se necessário e otimiza."""
    img = Image.open(caminho_imagem)
    # Exemplo: Redimensionar ou converter para tons de cinza
    img_gray = img.convert("L")
    dados_numpy = np.array(img_gray)
    return dados_numpy