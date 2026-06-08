# Receptor de Imagens MQTT ESP32-CAM (NB-IoT)

Este repositório contém o software responsável por receber, processar e montar imagens transmitidas por um dispositivo ESP32-CAM através da rede NB-IoT (utilizando o protocolo MQTT) ou via comunicação serial.

O sistema recebe a imagem em fragmentos (chunks) e se encarrega de reconstruí-la em formato `.jpg`, salvando o resultado no diretório de saída.

## Estrutura do Projeto

- `config/`: Configurações globais do projeto (ex: porta serial, host MQTT, diretórios de saída).
- `core/`: Módulos centrais para comunicação serial (`serial_comm.py`) e processamento de imagem (`image_processor.py`).
- `scripts/`: Scripts principais para execução do sistema. Destaca-se o `mqtt_receiver.py` para recebimento das imagens via MQTT.
- `output/`: Diretório padrão onde as imagens montadas e processadas são armazenadas.
- `requirements.txt`: Lista das dependências e bibliotecas Python necessárias para rodar o software.

## Pré-requisitos

- Python 3.8 ou superior instalado.
- Acesso à internet para instalação dos pacotes e comunicação com o broker MQTT.

## Como Configurar e Construir (Build)

Recomenda-se fortemente a utilização de um ambiente virtual (Virtual Environment - `venv`) para instalar as dependências do projeto, isolando-as do resto do sistema operacional.

Siga os passos abaixo para preparar o ambiente:

### 1. Criar o Ambiente Virtual

Abra o terminal na pasta raiz do software (`Software/`) e execute o seguinte comando:

```bash
python -m venv .venv
```

### 2. Ativar o Ambiente Virtual

Ative o ambiente criado de acordo com o seu sistema operacional:

**No Windows:**
```cmd
.venv\Scripts\activate
```

**No Linux / macOS:**
```bash
source .venv/bin/activate
```

*(Quando ativado, você verá `(.venv)` no início da linha de comando do seu terminal.)*

### 3. Instalar as Dependências

Com o ambiente virtual ativado, instale os pacotes listados no arquivo `requirements.txt`:

```bash
pip install -r requirements.txt
```

Isso instalará automaticamente bibliotecas essenciais como `numpy`, `pyserial`, `pillow` (para processamento de imagem) e `paho-mqtt` (para comunicação MQTT).

## Como Usar

Para iniciar o receptor MQTT e começar a escutar as imagens enviadas pelo ESP32-CAM, certifique-se de que o ambiente virtual está ativado e execute:

```bash
python -m scripts.mqtt_receiver
```

O script irá conectar-se ao Broker MQTT configurado, inscrever-se nos tópicos de chunks e montar automaticamente as fotos recebidas no diretório `output/`. Para demais ajustes como o endereço IP do Broker ou a Porta Serial, você pode editar os arquivos dentro de `config/` e os respectivos scripts.
