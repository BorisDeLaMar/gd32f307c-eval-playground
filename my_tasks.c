#include "my_tasks.h"

#include "gd32f30x_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <inttypes.h>

#include "defines.h"
#include "structs.h"

extern SemaphoreHandle_t xTxSemaphore;

volatile cycle_buf rx_buf = {0};
volatile uint8_t tx_buf[TX_RX_BUF_SIZE];

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void) xTask;
    (void) pcTaskName;
    
    while(1) {}
}

void vTaskBlink(void *pvParameters) 
{
    while(1){
        gpio_bit_set(GPIOC, GPIO_PIN_0);
        gpio_bit_reset(GPIOE, GPIO_PIN_1);

        vTaskDelay(1000);

        gpio_bit_set(GPIOC, GPIO_PIN_2);
        gpio_bit_reset(GPIOC, GPIO_PIN_0);
        
        vTaskDelay(1000);

        gpio_bit_set(GPIOE, GPIO_PIN_0);
        gpio_bit_reset(GPIOC, GPIO_PIN_2);
        
        vTaskDelay(1000);
        
        gpio_bit_set(GPIOE, GPIO_PIN_1);
        gpio_bit_reset(GPIOE, GPIO_PIN_0);
        
        vTaskDelay(1000);
    }
}

void vTaskRW(void *pvParameters)
{
    uint32_t dma_counter = TX_RX_BUF_SIZE;
    
    while(1)
    {
        xTaskNotifyWait(0, 0, &dma_counter, portMAX_DELAY);
        
        rx_buf.head = TX_RX_BUF_SIZE - dma_counter;
        
        dma_channel_disable(DMA0, DMA_CH3);
        dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
        
        uint16_t packet_len = rx_buf.head > rx_buf.tail ? rx_buf.head - rx_buf.tail : TX_RX_BUF_SIZE - rx_buf.tail + rx_buf.head;
         
        tx_buf[0] = '\r'; tx_buf[1] = '\n';
        for(size_t i = 0; i < packet_len; i++)
        {
            if(rx_buf.tail + i >= TX_RX_BUF_SIZE)
                tx_buf[i+2] = rx_buf.buf[rx_buf.tail + i - TX_RX_BUF_SIZE];
            else
                tx_buf[i+2] = rx_buf.buf[rx_buf.tail + i];
        }
        rx_buf.tail = rx_buf.head;
        
        dma_memory_address_config(DMA0, DMA_CH3, (uint32_t) tx_buf);
        dma_transfer_number_config(DMA0, DMA_CH3, packet_len+2);
        dma_channel_enable(DMA0, DMA_CH3);
        
        xSemaphoreTake(xTxSemaphore, portMAX_DELAY);
    }
}