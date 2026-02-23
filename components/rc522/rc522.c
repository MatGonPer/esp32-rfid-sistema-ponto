#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "string.h"

#define MOSI 23
#define MISO 19
#define SCK  18

#define CLOCK_SPEED 5000000
#define CS   5

spi_device_handle_t rc522_handle;

void rc522_init(void) {
    //configuração do barramento do protocolo SPI
    spi_bus_config_t barramento = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    //configuração do sensor RC522 que usará o barramento configurado acima
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
