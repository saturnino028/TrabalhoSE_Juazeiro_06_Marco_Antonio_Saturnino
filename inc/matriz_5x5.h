/**
 * @brief arquivo de cabeçalho da biblioteca de controle da matriz de led ws2812
 */
#ifndef MATRIZ_5X5_H
#define MATRIZ_5X5_H

/********************* Includes *********************/
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "InterfacePio.pio.h"

/********************* Defines *********************/

#define Mat_pin 7 //Pino da matriz de leds

/********************* Variaveis Globais *********************/

extern uint8_t brilho_matriz;

extern PIO pio;
extern int sm;
extern int offset;

/********************* Prototipo de Funções *********************/

void init_matriz();
void desenhar_fig(uint32_t *_matriz, uint8_t _intensidade);

#endif //MATRIZ_5X5_H