#include <stdio.h>
#include "rc522.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("Sistema de Ponto RFID Iniciado!\n");
    
    rc522_init();
    rc522_reset();
    rc522_antena_on();

    uint8_t versao = rc522_ler_registrador(0x37);
    printf("Versao do MFRC522: 0x%X\n", versao);

    while(1) {
        uint8_t buffer_resp[16];
        uint8_t tam_resp;
        
        uint8_t cmd_reqa = 0x26; 
        int status = rc522_comunicar(&cmd_reqa, 1, buffer_resp, &tam_resp, 0x87);

        if (status == 1) {
            uint8_t cmd_anticol[] = {0x93, 0x20};
            uint8_t uid_buffer[16];
            uint8_t uid_tam;

            int status_uid = rc522_comunicar(cmd_anticol, 2, uid_buffer, &uid_tam, 0x80);

            if (status_uid == 1) {
                printf("\n--- CARTAO LIDO COM SUCESSO ---\n");
                printf("ID do Funcionario (UID): ");
                for (int k = 0; k < 4; k++) { // O UID padrão tem 4 bytes
                    printf("%02X ", uid_buffer[k]);
                }
                printf("\n-------------------------------\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
