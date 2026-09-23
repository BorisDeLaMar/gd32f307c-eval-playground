#include "irqs.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "gd32f30x_dma.h"
#include "gd32f30x_misc.h"
#include "gd32f30x_usart.h"

#include <inttypes.h>
#include <stdbool.h>

#include "utils.h"

extern SemaphoreHandle_t xTxSemaphore;

extern TaskHandle_t xRxTaskHandle;

void nvic_priorities()
{
	// compulsory with FreeRTOS
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	
    nvic_irq_enable(DMA0_Channel3_IRQn, 6, 0);
	nvic_irq_enable(USART0_IRQn, 7, 0);
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