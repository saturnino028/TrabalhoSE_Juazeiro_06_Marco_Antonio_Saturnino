/**
 * @brief Arquivo de cabeçalho de cabeçalho do arquivo enchentes. 
 *        Funções da camada de aplicação.
 * @author Marco A J Saturnino
 * @date 18/05/2025
*/

#ifndef ENCHENTES_H
#define ENCHENTES_H
 
/***********************  Includes ***************************/
#include "prefixos_uteis.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "ssd1306.h"
#include "pinout.h"
#include <stdio.h>
#include "task.h"

/***************  Defines e Variaveis Globais ****************/

/****************** Prototipo de Funções *********************/
uint8_t InitSistema();

void config_pins_gpio();
uint config_pwm(uint8_t _pin, uint16_t _freq_Hz);

void duty_cicle(float _percent, uint _slice, uint8_t _pin);
void campainha(float _dc, uint32_t _duracao_ms, uint _slice, uint8_t _pin);
int64_t fim_campainha(alarm_id_t id, void *user_data);

void botoes_callback(uint gpio, uint32_t events);
void AtualizarDisplay();
void leds_put(bool r, bool g, bool b);

void vTaskSensorRuido();
void vTaskSilencio();
void vTaskEntrada();
void vTaskSaida();
void vTaskReset();

#endif //ENCHENTES_H