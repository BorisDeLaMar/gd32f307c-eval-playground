#include "my_tasks.h"

#include "gd32f30x_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#include <inttypes.h>
#include <stdbool.h>

#include "defines.h"
#include "structs.h"
#include "utils.h"

extern SemaphoreHandle_t xTxSemaphore;
extern EventGroupHandle_t xI2CEventGroup;

volatile cycle_buf rx_buf = {0};
volatile uint8_t tx_buf[TX_RX_BUF_SIZE];

extern volatile uint8_t i2c_rx_bytes[4]; // from irq

uint32_t curCS = 0;
uint32_t eeprom_cs = 0;

bool rw_flag = 0; // 0 for reading, 1 for writting

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
        tx_buf[2] = '|';  tx_buf[3] = eeprom_cs; tx_buf[4] = eeprom_cs >> 8; tx_buf[5] = eeprom_cs >> 16; tx_buf[6] = eeprom_cs >> 24; tx_buf[7] = '|';
        for(size_t i = 0; i < packet_len; i++)
        {
            if(rx_buf.tail + i >= TX_RX_BUF_SIZE)
                tx_buf[i+8] = rx_buf.buf[rx_buf.tail + i - TX_RX_BUF_SIZE];
            else
                tx_buf[i+8] = rx_buf.buf[rx_buf.tail + i];
        }
        rx_buf.tail = rx_buf.head;
        
        dma_memory_address_config(DMA0, DMA_CH3, (uint32_t) tx_buf);
        dma_transfer_number_config(DMA0, DMA_CH3, packet_len+8);
        dma_channel_enable(DMA0, DMA_CH3);
        
        xSemaphoreTake(xTxSemaphore, portMAX_DELAY);
    }
}

void vTaskCsEEPROM(void *pvParameters)
{
    // check
    curCS = countCS(); 
    
    while(I2C_STAT1(I2C0) & (1 << 1)); // I2CBSY  
    I2C_CTL0(I2C0) |= (1 << 8); // START
    
    xEventGroupWaitBits(xI2CEventGroup, 0x01, pdTRUE, pdFALSE, portMAX_DELAY); // Rest of the I2C in I2C0_EV_IRQHandler
    
    eeprom_cs = 0;
    
    for(uint32_t i = 0; i < 4; i++)
    {
        eeprom_cs |= (i2c_rx_bytes[i] << 8*i);
    }
    
    if(eeprom_cs != curCS) {
        rw_flag = 1;
        
        while(I2C_STAT1(I2C0) & (1 << 1)); // I2CBSY
        I2C_CTL0(I2C0) |= (1 << 8); // START
        
        xEventGroupWaitBits(xI2CEventGroup, 0x01, pdTRUE, pdFALSE, portMAX_DELAY); // Rest of the I2C in I2C0_EV_IRQHandler  
        
        vTaskDelay(10); // for EEPROM to write into flash
    }
    
    rw_flag = 0;
    while(I2C_STAT1(I2C0) & (1 << 1)); // I2CBSY  
    I2C_CTL0(I2C0) |= (1 << 8); // START
    
    xEventGroupWaitBits(xI2CEventGroup, 0x01, pdTRUE, pdFALSE, portMAX_DELAY); // Rest of the I2C in I2C0_EV_IRQHandler
    
    eeprom_cs = 0;
    
    for(uint32_t i = 0; i < 4; i++)
    {
        eeprom_cs |= (i2c_rx_bytes[i] << 8*i);
    }
        
    while(1) 
    {
        vTaskDelay(1000);
    }
}