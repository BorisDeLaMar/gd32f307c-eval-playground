#include "irqs.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#include "gd32f30x_dma.h"
#include "gd32f30x_i2c.h"
#include "gd32f30x_misc.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_usart.h"

#include <inttypes.h>
#include <stdbool.h>

#include "defines.h"
#include "utils.h"

extern SemaphoreHandle_t xTxSemaphore;
extern EventGroupHandle_t xI2CEventGroup;

extern TaskHandle_t xRxTaskHandle;

extern bool rw_flag;

extern uint32_t curCS;

volatile uint8_t i2c_rx_bytes[4];
volatile uint8_t i2c_rx_idx = 0;

volatile uint8_t bits_cnt = 0;

typedef enum {
    I2C_STATE_IDLE = 0,
    I2C_STATE_WRITE_ADDR,
    I2C_STATE_WRITE_REG,
    I2C_STATE_WAIT_BTC,
    I2C_STATE_READ_ADDR,
    I2C_STATE_READ_DATA,
    I2C_STATE_WRITE_DATA,
    I2C_STATE_WRITE_END
} i2c_state_t;

volatile i2c_state_t i2c_main_state = I2C_STATE_IDLE;

void nvic_priorities()
{
	// compulsory with FreeRTOS
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
    nvic_irq_enable(DMA0_Channel3_IRQn, 6, 0);
	nvic_irq_enable(USART0_IRQn, 7, 0);
    nvic_irq_enable(I2C0_EV_IRQn, 7, 0);
}

void DMA0_Channel3_IRQHandler(void)
{
    if(RESET != dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF))
    {
        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_FTF);
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xTxSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void USART0_IRQHandler(void)
{
	if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
	{
        reset_IDLE();
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        xTaskNotifyFromISR(xRxTaskHandle, dma_transfer_number_get(DMA0, DMA_CH4), eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void I2C0_EV_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // SBSEND
    if (I2C_STAT0(I2C0) & (1 << 0)) 
    {
        if (i2c_main_state == I2C_STATE_IDLE) 
        {
            i2c_main_state = I2C_STATE_WRITE_ADDR;
            I2C_DATA(I2C0) = (EEPROM_ADDRESS & 0xFE); // Address + W (0xA0)
        } 
        else if (i2c_main_state == I2C_STATE_WAIT_BTC) 
        {
            i2c_main_state = I2C_STATE_READ_ADDR;
            I2C_DATA(I2C0) = EEPROM_ADDRESS | 0x01; // Address + R (0xA1)
        }
    }

    // ADDSEND
    if (I2C_STAT0(I2C0) & (1 << 1)) 
    {
        // To reset ADDSEND via hardware
        (void)I2C_STAT0(I2C0);
        (void)I2C_STAT1(I2C0);

        if (i2c_main_state == I2C_STATE_WRITE_ADDR) 
        {
            i2c_main_state = I2C_STATE_WRITE_REG;
            // wait for TBE
        } 
        else if (i2c_main_state == I2C_STATE_READ_ADDR) 
        {
            i2c_main_state = I2C_STATE_READ_DATA;
            i2c_rx_idx = 0;
        }
    }

    // TBE
    if ((I2C_STAT0(I2C0) & (1 << 7)) && (i2c_main_state == I2C_STATE_WRITE_REG)) 
    {
        I2C_DATA(I2C0) = 0x00; // first EEPROM byte
        i2c_main_state = I2C_STATE_WAIT_BTC;
    }

    // BTC
    if ((I2C_STAT0(I2C0) & (1 << 2)) && (i2c_main_state == I2C_STATE_WAIT_BTC)) 
    {
        if(!rw_flag)
            I2C_CTL0(I2C0) |= (1 << 8); // REPEATED_START for reading, i2c_main_state will be changed in SBSEND irq
        else
            i2c_main_state = I2C_STATE_WRITE_DATA; // true write, not dummy
    }
    
    // BTC while writting
    if ((I2C_STAT0(I2C0) & (1 << 2)) && (i2c_main_state == I2C_STATE_WRITE_DATA))
    {   
        I2C_DATA(I2C0) = (curCS >> bits_cnt) & 0xFF;
        bits_cnt += 8;
        
        /*if(bits_cnt == 24) 
        {   
            I2C_CTL0(I2C0) &= ~(1 << 10);
            I2C_CTL0(I2C0) |= (1 << 9);
        }*/
        
        if(bits_cnt == 32)
        {
            bits_cnt = 0;
            
            i2c_main_state = I2C_STATE_IDLE;
            //I2C_CTL0(I2C0) |= (1 << 10);
            I2C_CTL0(I2C0) |= (1 << 9); // STOP
            
            xEventGroupSetBitsFromISR(xI2CEventGroup, 0x01, &xHigherPriorityTaskWoken);
        }
    }

    // RBNE
    if (I2C_STAT0(I2C0) & (1 << 6)) 
    {
        if (i2c_main_state == I2C_STATE_READ_DATA) 
        {
            i2c_rx_bytes[i2c_rx_idx] = (uint8_t)I2C_DATA(I2C0);
            i2c_rx_idx++;

            if (i2c_rx_idx == 3) 
            {
                I2C_CTL0(I2C0) &= ~(1 << 10); // NACK (ACKEN = 0)
                I2C_CTL0(I2C0) |= (1 << 9);   // STOP
            }
            
            if (i2c_rx_idx == 4) 
            {     
                i2c_main_state = I2C_STATE_IDLE;
                I2C_CTL0(I2C0) |= (1 << 10); // ACKEN = 1
                
                xEventGroupSetBitsFromISR(xI2CEventGroup, 0x01, &xHigherPriorityTaskWoken);
            }
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}