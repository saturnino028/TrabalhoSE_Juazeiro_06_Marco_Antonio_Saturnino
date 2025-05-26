#include "figuras_ssd1306.h"
#include "figuras_ws2812.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "matriz_5x5.h"
#include "sistema.h"
#include "semphr.h"
#include "queue.h"

/********************* Variaveis Globais *********************/
ssd1306_t ssd; volatile uint8_t slice_b;
volatile uint16_t top_wrap = 1000; //Valor máximo do pwm
volatile uint32_t passado = 0; //Usada para implementar o debouncing
uint16_t joy_pos[6] = {2047,4095,0,2047,4095,0};
volatile float volume_buzzer = 2.0;

const uint8_t sisMAX_PESSOAS = 10;

typedef struct
{
    uint8_t Lotacao;
    uint8_t Ruido;
} MonitorLib_t;

typedef struct 
{
    uint _slice;
    uint8_t _pin;
} def_canais_pwm;

def_canais_pwm dados;
MonitorLib_t dados_lib;

SemaphoreHandle_t xDisplayMutex; //Exibir mensagens no display
QueueHandle_t xRuidoQueue; //Monitorar nível de ruído na sala
SemaphoreHandle_t xResetSem; //Semáforo binário para reset
SemaphoreHandle_t xBotaoIncSem; //Semáforo binário para incrementar
SemaphoreHandle_t xBotaoDecSem; //Semáforo binário para decrementar
SemaphoreHandle_t xContSem; //Semáforo de contagem para número de pessoas

/************************** Funções *************************/

//Funções de configuração

/**
 * @brief função para iniciar o sistema
 */
uint8_t InitSistema()
{
    bool cor = true;
    
    set_sys_clock_khz(1250000,false); //Cofigura o clock

    stdio_init_all();

    config_pins_gpio(); //Inicia os pinos GPIO

    init_matriz(); //Inicia a matriz de LEDs 5x5 WS2812

    config_i2c_display(&ssd);

    slice_b = config_pwm(buz_B, 1*KHz); //Configura um slice para 1KHz

    adc_init();
    adc_gpio_init(adc_jX);

    desenhar_fig(open, 10);

    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "  EMBARCATECH", 5, 15); // Desenha uma string
    ssd1306_draw_string(&ssd, "RESTIC 37", 26, 29); // Desenha uma string  
    ssd1306_draw_string(&ssd, "  FASE 2", 26, 43); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(2000);
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_draw_image(&ssd, fig_principal);     
    ssd1306_send_data(&ssd); // Atualiza o display

    uint8_t flag_led = 1;
    for(uint8_t etapa = 0; etapa<6; etapa++)
    {
        gpio_put(LED_R, (flag_led & 0b00000001));
        gpio_put(LED_G, (flag_led & 0b00000010));
        gpio_put(LED_B, (flag_led & 0b00000100));
        for(int i = 0; i<500; i++)
            sleep_ms(1);
        flag_led = flag_led*2;
    }

    xRuidoQueue = xQueueCreate(10, sizeof(uint8_t)); //Criar fila para leituras do MIC
    xResetSem = xSemaphoreCreateBinary(); //Cria semáforo binário para indicar RESET
    xBotaoIncSem = xSemaphoreCreateBinary();             // evento do botão INC
    xBotaoDecSem = xSemaphoreCreateBinary();             // evento do botão DEC
    xContSem = xSemaphoreCreateCounting(10,0); //Cria semáforo de contagem
    xDisplayMutex = xSemaphoreCreateMutex(); //Cria mutex para o display

    gpio_set_irq_enabled_with_callback(bot_A, GPIO_IRQ_EDGE_FALL, true, &botoes_callback);
    gpio_set_irq_enabled_with_callback(bot_B, GPIO_IRQ_EDGE_FALL, true, &botoes_callback);
    gpio_set_irq_enabled_with_callback(bot_joy, GPIO_IRQ_EDGE_FALL, true, &botoes_callback);

    desenhar_fig(apagado, 10);

    campainha(volume_buzzer, 1000,slice_b, buz_B);

    sleep_ms(1000);

    AtualizarDisplay();
    leds_put(0,0,1);
}

void leds_put(bool r, bool g, bool b)
{
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}

/**
 * @brief inicia os pinos de GPIO
 */
void config_pins_gpio()
{
    //Configuração do botao A
    gpio_init(bot_A);
    gpio_pull_up(bot_A);
    gpio_set_dir(bot_A, GPIO_IN);

    //Configuração do botao B
    gpio_init(bot_B);
    gpio_pull_up(bot_B);
    gpio_set_dir(bot_B, GPIO_IN);

    //Configuração do botao do Joystick
    gpio_init(bot_joy);
    gpio_pull_up(bot_joy);
    gpio_set_dir(bot_joy, GPIO_IN);

    //Configuração do LED vermelho
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    //Configuração do LED verde
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    //Configuração do LED azul
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
}

/**
 * @brief Configura os pinos PWM
 */
uint config_pwm(uint8_t _pin, uint16_t _freq_Hz)
{
    uint slice; float Fpwm;
    top_wrap = 1000000/_freq_Hz;
    gpio_set_function(_pin, GPIO_FUNC_PWM); //Habilita a função PWM
    slice = pwm_gpio_to_slice_num(_pin);//Obter o valor do slice correspondente ao pino
    pwm_set_clkdiv(slice, 125.0); //Define o divisor de clock
    pwm_set_wrap(slice, top_wrap); //Define valor do wrap
    Fpwm = 125000000/(125.0*top_wrap);
    printf("PWM definido para %.2f Hz\n", Fpwm);
    return slice; //Retorna o slice correspondente
}

///Funções PWM
/**
 * @brief ajusta o duty cicle
 */
void duty_cicle(float _percent, uint _slice, uint8_t _pin)
{
    pwm_set_enabled(_slice, false); //Desabilita PWM
    uint16_t valor_pwm = (_percent/100)*top_wrap; //Configura DutyCicle
    pwm_set_gpio_level(_pin, valor_pwm); //Configura DutyCicle
    pwm_set_enabled(_slice, true); //Habilitar PWM
}

/**
 * @brief função de callback para desativar a campainha
 */
int64_t fim_campainha(alarm_id_t id, void *user_data)
{
    def_canais_pwm *data = (def_canais_pwm *)user_data;
    uint _slice = data -> _slice;
    uint8_t _pin = data -> _pin;
    duty_cicle(0.0, _slice, _pin);
    return 0;
}

/**
 * @brief função para som no buzzer
 */
void campainha(float _dc, uint32_t _duracao_ms, uint _slice, uint8_t _pin)
{
    duty_cicle(_dc, _slice, _pin);
    dados._slice = _slice;
    dados._pin = _pin;
    add_alarm_in_ms(_duracao_ms, fim_campainha, &dados, false);
}

/**
 * @brief trata a interrupção gerada pelos botões A e B da BitDog
 * @param gpio recebe o pino que gerou a interrupção
 * @param events recebe o evento que causou a interrupção
 */
void botoes_callback(uint gpio, uint32_t events)
{
    printf("Interrupcao");
    // Obtém o tempo atual em microssegundos
    uint32_t agora = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (agora - passado > 200000) // 200 ms de debouncing
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if(gpio == bot_A)
            xSemaphoreGiveFromISR(xBotaoIncSem, &xHigherPriorityTaskWoken);
        else if(gpio == bot_B)
            xSemaphoreGiveFromISR(xBotaoDecSem, &xHigherPriorityTaskWoken);
        else //botão joystick
            xSemaphoreGiveFromISR(xResetSem, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        passado = agora;
    }

}

void AtualizarDisplay()
{
    bool cor = true;
    char buffer_dado1[5];  // Buffer para armazenar a string com os dados do sensor pluviométrico
    char buffer_dado2[5];  // Buffer para armazenar a string com os dados do sensor volumétrico

    sprintf(buffer_dado1, "%d", dados_lib.Lotacao);
    sprintf(buffer_dado2, "%d", dados_lib.Ruido);

    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
    ssd1306_line(&ssd, 3, 16, 123, 16, cor); // Desenha uma linha
    ssd1306_line(&ssd, 3, 37, 123, 37, cor); // Desenha uma linha   
    ssd1306_draw_string(&ssd, " LibraryAcess", 5, 6); // Desenha uma string
    ssd1306_draw_string(&ssd, "Max.10 pessoas", 8, 18); // Desenha uma string
    ssd1306_draw_string(&ssd, "Silencio", 8, 28); // Desenha uma string 
    ssd1306_draw_string(&ssd, "L    R    ", 20, 41); // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor); // Desenha uma linha vertical     
    ssd1306_draw_string(&ssd, buffer_dado1, 8, 52); // Desenha uma string  
    ssd1306_line(&ssd, 84, 37, 84, 60, cor); // Desenha uma linha vertical 
    ssd1306_draw_string(&ssd, buffer_dado2, 49, 52); // Desenha uma string
    ssd1306_send_data(&ssd);
}

/**
 * @brief Monitora o nível de ruído na sala
 */
void vTaskSensorRuido()
{
    while(true)
    {
        uint32_t somador = 0; uint8_t val = 0; 
        uint16_t amostras = 100;
        for(int i = 0; i<amostras; i++)
        {
            adc_select_input(1); // Canal do MIC
            somador = adc_read() + somador;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        somador = somador/amostras;
        somador = abs(somador - 2048);
        val = ((int)somador*100)/2048;
        xQueueSend(xRuidoQueue, &val, pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Monitora o nível de barulho
 * 
 */
void vTaskSilencio()
{
    uint8_t valor_ruido = 0;
    while (true)
    {
        if(xQueueReceive(xRuidoQueue, &valor_ruido, pdMS_TO_TICKS(100)))
        {
            dados_lib.Ruido = valor_ruido;

            if(valor_ruido <10)
                desenhar_fig(apagado, brilho_matriz);
            else if(valor_ruido < 20)
                desenhar_fig(dez_a_20, brilho_matriz);
            else if(valor_ruido < 40)
                desenhar_fig(vinte_a_40, brilho_matriz);
            else if(valor_ruido < 60)
                desenhar_fig(quarenta_a_60, brilho_matriz);
            else if(valor_ruido < 80)
                desenhar_fig(sessenta_a_80, brilho_matriz);
            else
            {
                desenhar_fig(oitenta_a_100, brilho_matriz);
                campainha(volume_buzzer, 1000, slice_b, buz_B);
            }
            
            if(xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(500)) == pdTRUE) 
            {
                AtualizarDisplay();
                xSemaphoreGive(xDisplayMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Monitora a entrada de pessoas na sala
 */
void vTaskEntrada()
{
    while(true)
    {
        if (xSemaphoreTake(xBotaoIncSem, portMAX_DELAY) == pdTRUE)
        {
            xSemaphoreGive(xContSem);
            dados_lib.Lotacao = uxSemaphoreGetCount(xContSem);

            if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE) 
            {
                if(dados_lib.Lotacao == sisMAX_PESSOAS)
                {
                    leds_put(1,0,0);
                    ssd1306_fill(&ssd, false); // Limpa o display
                    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // Desenha um retângulo 
                    ssd1306_draw_string(&ssd, "Sala Cheia", 8, 30); // Desenha uma string
                    ssd1306_send_data(&ssd);
                    campainha(volume_buzzer, 200, slice_b, buz_B);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    AtualizarDisplay();
                }
                else if(dados_lib.Lotacao == sisMAX_PESSOAS-1)
                {
                    AtualizarDisplay();
                    leds_put(1,1,0);
                }
                else if(dados_lib.Lotacao >= 1)
                {
                    AtualizarDisplay();
                    leds_put(0,1,0);
                }
                else
                    leds_put(0,0,1);
                
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

}

/**
 * @brief monitora a saída de pessoas na sala
 */
void vTaskSaida()
{
    while(true)
    {
        if (xSemaphoreTake(xBotaoDecSem, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(xContSem, 0) == pdTRUE);
            dados_lib.Lotacao = uxSemaphoreGetCount(xContSem);

            if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                if(dados_lib.Lotacao == sisMAX_PESSOAS)
                    leds_put(1,0,0);
                else if(dados_lib.Lotacao == sisMAX_PESSOAS-1)
                    leds_put(1,1,0);
                else if(dados_lib.Lotacao >= 1)
                    leds_put(0,1,0);
                else
                    leds_put(0,0,1);

                AtualizarDisplay();
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

}

/**
 * @brief reseta o sistema
 */
void vTaskReset()
{
    while(true)
    {
        if(xSemaphoreTake(xResetSem, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
            {
                ssd1306_fill(&ssd, false); // Limpa o display
                ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // Desenha um retângulo 
                ssd1306_draw_string(&ssd, "RESTART PRESS", 8, 30); // Desenha uma string
                ssd1306_send_data(&ssd);
                campainha(volume_buzzer, 200, slice_b, buz_B);
                vTaskDelay(pdMS_TO_TICKS(300));
                campainha(volume_buzzer, 200, slice_b, buz_B);
                vTaskDelay(pdMS_TO_TICKS(2000));
                while (xSemaphoreTake(xContSem, 0) == pdTRUE);
                dados_lib.Lotacao =  uxSemaphoreGetCount(xContSem);
                dados_lib.Ruido = 0;
                AtualizarDisplay();
                leds_put(0,0,1);
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

}