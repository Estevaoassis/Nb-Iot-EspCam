from pymongo import MongoClient
from pymongo.errors import ConnectionFailure
from config import settings
import datetime

client = None
db = None
collection = None

def conectar_mongodb():
    global client, db, collection
    if client is None:
        try:
            client = MongoClient(settings.MONGO_URI, serverSelectionTimeoutMS=5000)
            # Tenta conectar para confirmar a disponibilidade
            client.admin.command('ping')
            db = client[settings.MONGO_DB]
            collection = db[settings.MONGO_COLLECTION]
            print(f"[MongoDB] Conectado com sucesso em: {settings.MONGO_URI}")
            print(f"[MongoDB] Banco de Dados: {settings.MONGO_DB} | Collection: {settings.MONGO_COLLECTION}")
        except ConnectionFailure as e:
            print(f"[ERRO MongoDB] Falha na conexão: {e}")
            client = None
        except Exception as e:
            print(f"[ERRO MongoDB] Erro inesperado: {e}")
            client = None

def salvar_imagem_mongodb(dados_binarios, formato="JPEG"):
    if client is None:
        conectar_mongodb()
        
    if client is not None:
        try:
            # Documento no BSON usa automaticamente tipo Binário para objetos bytes em Python
            documento = {
                "timestamp": datetime.datetime.utcnow(),
                "formato": formato,
                "tamanho_bytes": len(dados_binarios),
                "dados_imagem": dados_binarios
            }
            resultado = collection.insert_one(documento)
            print(f"[MongoDB] Imagem salva com sucesso! ID: {resultado.inserted_id}")
            return resultado.inserted_id
        except Exception as e:
            print(f"[ERRO MongoDB] Falha ao salvar imagem: {e}")
            return None
    else:
        print("[AVISO MongoDB] Imagem não foi salva no banco pois não há conexão ativa.")
        return None
