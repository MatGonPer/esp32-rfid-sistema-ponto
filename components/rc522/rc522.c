#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "string.h"

#define MOSI 23
#define MISO 19
#define SCK  18

#define CLOCK_SPEED 5000000
#define CS   5

#define TX_CONTROL_REG  0x14
#define COMMAND_REG     0x01
#define PCD_RESETPHASE  0x0F
#define COM_IRQ_REG     0x04
#define FIFO_LEVEL_REG  0x0A
#define FIFO_DATA_REG   0x09
#define BIT_FRAMING_REG 0x0D

#define TX_ASK_REG      0x15
#define T_MODE_REG      0x2A
#define T_PRESCALER_REG 0x2B
#define T_RELOAD_REG_H  0x2C
#define T_RELOAD_REG_L  0x2D

spi_device_handle_t rc522_handle;

void rc522_init(void) {
    spi_bus_config_t barramento = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    spi_device_interface_config_t dispositivo = {
        .clock_speed_hz = CLOCK_SPEED, //5Mhz
        .mode = 0,
        .spics_io_num = CS,
        .queue_size = 7
    };

    spi_bus_initialize(SPI3_HOST, &barramento, 0);

    spi_bus_add_device(SPI3_HOST, &dispositivo, &rc522_handle);
}

void rc522_escrever_registrador(uint8_t endereco, uint8_t valor) {
    uint8_t tx_data[2];
    
    tx_data[0] = (endereco << 1) & 0x7E;
    tx_data[1] = valor;

    spi_transaction_t transacao;
    memset(&transacao, 0, sizeof(transacao));

    transacao.length = 16;
    transacao.tx_buffer = tx_data;

    spi_device_polling_transmit(rc522_handle, &transacao);
}

uint8_t rc522_ler_registrador(uint8_t endereco) {
    uint8_t tx_data[2];

    tx_data[0] = (((endereco << 1) & 0x7E) | 0x80);
    tx_data[1] = 0x00;

    uint8_t rx_data[2];

    spi_transaction_t transacao;
    memset(&transacao, 0, sizeof(transacao));

    transacao.length = 16;
    transacao.tx_buffer = tx_data;
    transacao.rx_buffer = rx_data;

    spi_device_polling_transmit(rc522_handle, &transacao);

    return rx_data[1];
}

void rc522_antena_on(void) {
    uint8_t valor = rc522_ler_registrador(TX_CONTROL_REG);
    valor |= 0x03;
    
    rc522_escrever_registrador(TX_CONTROL_REG, valor);
}

void rc522_antena_off(void) {
    uint8_t valor = rc522_ler_registrador(TX_CONTROL_REG);
    valor &= 0xFC;

    rc522_escrever_registrador(TX_CONTROL_REG, valor);
}

void rc522_reset(void) {
    rc522_escrever_registrador(COMMAND_REG, PCD_RESETPHASE);
    vTaskDelay(pdMS_TO_TICKS(50));

    rc522_escrever_registrador(T_MODE_REG, 0x8D);
    rc522_escrever_registrador(T_PRESCALER_REG, 0x3E);
    rc522_escrever_registrador(T_RELOAD_REG_L, 30);
    rc522_escrever_registrador(T_RELOAD_REG_H, 0);

    rc522_escrever_registrador(TX_ASK_REG, 0x40);
}

int rc522_comunicar(uint8_t *dados_enviar, uint8_t tamanho_enviar, uint8_t *resposta, uint8_t *tamanho_resposta, uint8_t bit_framing) {
    rc522_escrever_registrador(COM_IRQ_REG, 0x7F);
    rc522_escrever_registrador(FIFO_LEVEL_REG, 0x80);

    for (int k = 0; k < tamanho_enviar; k++) {
        rc522_escrever_registrador(FIFO_DATA_REG, dados_enviar[k]);
    }

    rc522_escrever_registrador(COMMAND_REG, 0x0C);
    rc522_escrever_registrador(BIT_FRAMING_REG, bit_framing);

    int i = 2000;
    uint8_t n;
    while (i > 0) {
        n = rc522_ler_registrador(COM_IRQ_REG);
        if (n & 0x30) {
            break;
        }
        i--;
    }

    if (i > 0) {
        uint8_t erro = rc522_ler_registrador(0x06);
        if (erro & 0x1B) {
            return 0;
        }

        uint8_t quantidade = rc522_ler_registrador(FIFO_LEVEL_REG);
        *tamanho_resposta = quantidade;

        for (int j = 0; j < quantidade; j++) {
            resposta[j] = rc522_ler_registrador(FIFO_DATA_REG);
        }

        return 1;
    }

    return 0;
}
