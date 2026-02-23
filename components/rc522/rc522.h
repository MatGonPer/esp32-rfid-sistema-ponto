#ifndef RC522_H
#define RC522_H

#include <stdint.h>

void rc522_init(void);
void rc522_escrever_registrador(uint8_t endereco, uint8_t valor);
uint8_t rc522_ler_registrador(uint8_t endereco);

#endif
