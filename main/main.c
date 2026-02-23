#include <stdio.h>
#include "rc522.h"

void app_main(void) {
    printf("Sistema de Ponto RFID Iniciado!\n");
    
    rc522_init();
    
    uint8_t versao = rc522_ler_registrador(0x37);

    printf("Versao do MFRC522: 0x%X\n", versao);    
}
