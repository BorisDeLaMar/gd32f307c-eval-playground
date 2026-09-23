#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stddef.h>

#include "cfg.h"
#include "my_tasks.h"
#include "irqs.h"
#include "utils.h"

SemaphoreHandle_t xTxSemaphore;

TaskHandle_t xRxTaskHandle = NULL;

int main(void)
{
    rcu_config();
    nvic_priorities();
    
    leds_config();
    
    xTxSemaphore = xSemaphoreCreateBinary();
    
    xTaskCreate(vTaskBlink, (char*) "TaskBlink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vTaskRW, (char*) "TaskRW", configMINIMAL_STACK_SIZE*4, NULL, configMAX_PRIORITIES - 1, &xRxTaskHandle);
    
    usart_config();
    dma_for_usart0_config();
    
    reset_IDLE();
    
    vTaskStartScheduler();
    
    return 0;
}
