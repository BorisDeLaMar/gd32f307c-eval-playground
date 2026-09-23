#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

void vTaskBlink(void *pvParameters);
void vTaskRW(void *pvParameters);