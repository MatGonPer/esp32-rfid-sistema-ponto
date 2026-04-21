# Sistema de Ponto RFID - Nexti

Sistema de controle de ponto e acesso utilizando ESP32 e leitor RFID MFRC522. O firmware gerencia conexão Wi-Fi via portal cativo (Modo AP), realiza autenticação em API via HTTPS/JSON e possui lógica de roteamento inteligente para registro de ponto ou cadastro de novos cartões.

---

## Tecnologias e Ferramentas

| Item | Descrição |
|---|---|
| **Framework** | ESP-IDF v5.2 (C nativo) |
| **RTOS** | FreeRTOS (Multitasking) |
| **Protocolos** | SPI (RFID), HTTPS/TLS (API), TCP/IP (Wi-Fi) |
| **Parser** | cJSON |
| **Storage** | NVS (Non-volatile storage) |

---

## Instalação do Ambiente (ESP-IDF)

### Linux (Manjaro)

Abra o terminal e execute:

```bash
# Dependências do sistema
sudo pacman -S --needed gcc git make cmake gperf python-pip ninja dfu-util libusb

# Download do Framework
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf

# Instalação para o ESP32
./install.sh esp32

# Exportar variáveis (Adicione ao seu .zshrc ou .bashrc para ser permanente)
. $HOME/esp/esp-idf/export.sh
```

### Windows 10

1. Baixe o [ESP-IDF Windows Installer](https://dl.espressif.com/dl/esp-idf/).
2. Execute e selecione a versão **v5.2**.
3. Utilize o atalho **ESP-IDF PowerShell Prompt** criado na área de trabalho para todos os comandos abaixo.

---

## Comandos de Execução

Dentro da pasta do seu projeto:

```bash
# 1. Definir o chip
idf.py set-target esp32

# 2. Compilar, Gravar e Monitorar (Linux)
idf.py -p /dev/ttyUSB0 flash monitor

# 2. Compilar, Gravar e Monitorar (Windows)
# Troque COM3 pela sua porta identificada
idf.py -p COM3 flash monitor
```

> **Atalho no monitor:** `Ctrl + ]` para sair.

---

## Pinagem (Hardware Atual)

| Componente | Sinal | GPIO ESP32 |
|---|---|---|
| MFRC522 | SDA (CS) | 21 |
| MFRC522 | SCK | 18 |
| MFRC522 | MOSI | 23 |
| MFRC522 | MISO | 19 |
| MFRC522 | RST | 22 |
| MFRC522 | 3.3V | 3.3V |
| MFRC522 | GND | GND |

---

## Estrutura de Arquivos

```
.
├── main/
│   ├── main.c          # Código fonte principal
│   ├── rc522.c         # Driver de comunicação SPI do leitor
│   └── rc522.h         # Header do driver
├── CMakeLists.txt      # Script de build (CMake)
└── partitions.csv      # Mapa de partição da memória Flash
```
