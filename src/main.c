#include "sistema.h"

int main()
{
    InitSistema();

    // Criação das tasks
    xTaskCreate(vTaskSensorRuido, "Recebe dados dos sensores", 256, NULL, 1, NULL);

    xTaskCreate(vTaskSilencio, "Leitura do valor do sensor de ruído", 256, NULL,1,NULL);

    xTaskCreate(vTaskEntrada, "Controla as entradas na sala", 256, NULL, 1, NULL);

    xTaskCreate(vTaskSaida, "Controla as saídas na sala", 256, NULL, 1, NULL);

    xTaskCreate(vTaskReset, "Reseta o sistema", 256, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();

    panic_unsupported();
}