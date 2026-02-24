#ifndef RC522_H
#define RC522_H

#include <stdint.h>

void rc522_init(void);
void rc522_escrever_registrador(uint8_t endereco, uint8_t valor);
uint8_t rc522_ler_registrador(uint8_t endereco);
void rc522_antena_on(void);
void rc522_antena_off(void);
void rc522_reset(void);
int rc522_comunicar(uint8_t *dados_enviar, uint8_t tamanho_enviar, uint8_t *resposta, uint8_t *tamanho_resposta, uint8_t bit_framing);

#endif
