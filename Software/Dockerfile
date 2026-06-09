FROM python:3.9-slim

WORKDIR /app

# Copia apenas o requirements primeiro para aproveitar cache de camadas do Docker
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copia o restante do código
COPY . .

# Variáveis de ambiente para não bufferizar stdout e para conectar ao Mongo
ENV PYTHONUNBUFFERED=1
ENV MONGO_URI="mongodb://mongodb:27017/"

# Comando padrão
CMD ["python", "-m", "scripts.mqtt_receiver"]
