/**
 * @brief implementação da biblioteca de controle da matriz de leds ws2812
 */
#include "matriz_5x5.h"

uint8_t brilho_matriz = 10; //Variavel de controle do brilho (padrão 10%)
//Criação de Pio
PIO pio = pio0;    
int offset;
int sm;

/**
 * @brief inicializa a matriz de LEDs WS2812
 * 
 */
void init_matriz()
{
    offset = pio_add_program(pio, &Matriz_5x5_program); //Carrega o programa no PIO e armazena a posição
    sm = pio_claim_unused_sm(pio, true); //Busca uma máquina vazia
 
    Matriz_5x5_program_init(pio, sm, offset, Mat_pin); //Inica o programa
}

/**
 * @brief Desenha figura (aceita um padrão hexa, vindo direto do https://www.piskelapp.com/)
 * @param _matriz aceita uma matriz 1x25 com o padrão dos leds
 * @param _intensidade aceita um inteiro de 0 a 100 refente a intensidade do led
 * 
    //Mapa leds

    25, 24, 23, 22, 21,

    16, 17, 18, 19, 20,  

    15, 14, 13, 12, 11,

    06, 07, 08, 09, 10,    

    05, 04, 03, 02, 01  
 */
void desenhar_fig(uint32_t *_matriz, uint8_t _intensidade)
{
    uint32_t pixel = 0; uint8_t r, g, b;

    for(int i = 24; i>19; i--) //Linha 1
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF)*(_intensidade/100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF)*(_intensidade/100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF)*(_intensidade/100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel<<8u);
    }

    for(int i = 15; i<20; i++) //Linha 2
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF)*(_intensidade/100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF)*(_intensidade/100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF)*(_intensidade/100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (b << 16) | (r << 8) | g;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel<<8u);

    }

    for(int i = 14; i>9; i--) //Linha 3
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF)*(_intensidade/100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF)*(_intensidade/100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF)*(_intensidade/100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel<<8u);
    }

    for(int i = 5; i<10; i++) //Linha 4
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF)*(_intensidade/100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF)*(_intensidade/100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF)*(_intensidade/100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel<<8u);
    }

    for(int i = 4; i > -1 ; i--) //Linha 5
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF)*(_intensidade/100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF)*(_intensidade/100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF)*(_intensidade/100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel<<8u);
    }
}
