import base64
from io import BytesIO
from PIL import Image
from flask import Flask, render_template
from core import db_manager


app = Flask(__name__, 
            template_folder="../web/templates",
            static_folder="../web/static")

# Garante que o db_manager esteja conectado antes do primeiro request
with app.app_context():
    db_manager.conectar_mongodb()

@app.route("/")
def index():
    # Verifica se há conexão com o banco
    if db_manager.collection is None:
        return "Erro: Não foi possível conectar ao banco de dados.", 500
    
    # Busca todas as imagens ordenadas da mais recente para a mais antiga
    # Trazemos apenas os campos necessários
    cursor = db_manager.collection.find().sort("timestamp", -1)
    
    imagens = []
    for doc in cursor:
        dados_binarios = doc.get("dados_imagem", b"")
        formato = doc.get("formato", "Desconhecido")
        
        # Se a imagem for Raw/Grayscale, precisamos convertê-la para um JPEG real
        # antes de mandar para o navegador exibir no HTML
        if formato == "Grayscale/Raw" and dados_binarios:
            try:
                raw_bytes = dados_binarios
                # Se for menor, preenchemos com cinza como feito no receiver
                if len(raw_bytes) < 19200:
                    raw_bytes = raw_bytes + b'\x80' * (19200 - len(raw_bytes))
                
                img = Image.frombytes('L', (160, 120), raw_bytes[:19200])
                buffered = BytesIO()
                img.save(buffered, format="JPEG")
                dados_binarios = buffered.getvalue() # Pega os bytes do JPEG
            except Exception as e:
                print(f"Erro ao converter Grayscale para JPEG: {e}")

        # Converte o binário (agora em JPEG) para Base64 para exibir no HTML
        base64_encoded = base64.b64encode(dados_binarios).decode('utf-8')
        
        # Formata a data para exibir bonito na tela
        ts = doc.get("timestamp")
        data_formatada = ts.strftime("%d/%m/%Y %H:%M:%S") if ts else "Data Desconhecida"
        
        imagens.append({
            "id": str(doc["_id"]),
            "timestamp": data_formatada,
            "formato": formato,
            "tamanho": f"{doc.get('tamanho_bytes', 0)} bytes",
            "base64": base64_encoded
        })
        
    return render_template("index.html", imagens=imagens)

if __name__ == "__main__":
    # Roda o Flask escutando em todas as interfaces (0.0.0.0) na porta 5000
    app.run(host="0.0.0.0", port=5000, debug=False)
