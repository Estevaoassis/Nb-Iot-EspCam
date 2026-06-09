# ESP-CAM IoT Image Receiver & Dashboard

Este repositório contém o software completo (Backend, Banco de Dados e Frontend) responsável por receber, processar, armazenar e exibir imagens transmitidas por um dispositivo ESP32-CAM através de redes IoT (NB-IoT/MQTT ou Serial).

## O que o sistema faz?
1. **MQTT Receiver:** Fica escutando um Broker MQTT por fragmentos de imagem (*chunks*) enviados pela placa.
2. **Reconstrução:** Ao receber todos os *chunks*, ele monta a imagem (seja formato JPEG padrão ou Grayscale/Raw).
3. **Persistência Dupla:** Salva o arquivo de imagem fisicamente na pasta `output/` e armazena a versão codificada e seus metadados diretamente no banco de dados **MongoDB**.
4. **Web Dashboard:** Disponibiliza uma página web sofisticada para visualização das imagens capturadas em tempo real.

---

## Estrutura do Projeto

- `config/`: Configurações globais do projeto (variáveis de banco, porta serial, host MQTT).
- `core/`: 
  - `db_manager.py`: Comunicação com o MongoDB (inserção e consulta).
  - `image_processor.py` / `serial_comm.py`: Processamento e comunicação.
- `scripts/`: 
  - `mqtt_receiver.py`: Serviço em segundo plano que recebe as fotos pelo MQTT.
  - `web_dashboard.py`: Servidor Web em Flask para a interface visual.
- `web/`: Código-fonte do Frontend (HTML5 / CSS3 Moderno com *Glassmorphism*).
- `output/`: Diretório onde as imagens brutas são espelhadas localmente.
- `docker-compose.yml` e `Dockerfile`: Arquivos para subir e orquestrar a infraestrutura toda com o Docker.

---

## Como Rodar (Usando Docker - Recomendado)

A forma mais fácil e recomendada de rodar todo esse sistema de uma vez (banco de dados, receptor e dashboard) é utilizando o **Docker Compose**.

### Pré-requisitos
- [Docker Desktop](https://www.docker.com/products/docker-desktop) instalado e rodando.

### Iniciando a Infraestrutura

1. Abra o terminal na raiz desta pasta (`Software/`).
2. Execute o comando para construir as imagens e subir os três containers em segundo plano:
   ```bash
   docker-compose up --build -d
   ```

3. **Acompanhar os logs:**
   Se quiser ver as conexões MQTT sendo feitas e os chunks chegando em tempo real, use:
   ```bash
   docker-compose logs -f app
   ```

4. **Acessar o Web Dashboard:**
   Abra seu navegador e acesse: 👉 **http://localhost:5000**

---

## Como Rodar (Manual / Sem Docker)

Caso prefira rodar apenas scripts locais usando um ambiente virtual (`venv`), siga os passos:

1. **Crie e ative o ambiente virtual:**
   ```bash
   python -m venv .venv
   
   # No Windows:
   .venv\Scripts\activate
   # No Linux/Mac:
   source .venv/bin/activate
   ```

2. **Instale as dependências:**
   ```bash
   pip install -r requirements.txt
   ```

3. **Inicie o Receptor MQTT:**
   *(Nota: O MongoDB precisa estar rodando localmente na porta 27017, caso contrário a imagem será salva apenas na pasta `output/`)*
   ```bash
   python -m scripts.mqtt_receiver
   ```

4. **Inicie o Servidor do Web Dashboard:** (Abra em outro terminal com o `.venv` ativado)
   ```bash
   python -m scripts.web_dashboard
   ```
