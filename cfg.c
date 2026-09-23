#include "cfg.h"

#include "gd32f30x_rcu.h"
#include "gd32f30x_dma.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_usart.h"
#include "gd32f307c_eval.h"

#include "defines.h"
#include "structs.h"

extern volatile cycle_buf rx_buf;
extern volatile uint8_t tx_buf[TX_RX_BUF_SIZE];

void rcu_config() 
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOE);
}

void leds_config() 
{
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_bit_reset(GPIOC, GPIO_PIN_0);

    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    gpio_bit_reset(GPIOC, GPIO_PIN_2);
    
    gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_bit_reset(GPIOE, GPIO_PIN_0);
    
    gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_bit_reset(GPIOE, GPIO_PIN_1);
}

void usart_config()
{
    gd_eval_com_init(EVAL_COM0);
    
    usart_interrupt_enable(USART0, USART_INT_IDLE);
}

void dma_for_usart0_config(void)
{
    dma_parameter_struct dma_init_struct;
    
    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH3);
    
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t) tx_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = TX_RX_BUF_SIZE;
    dma_init_struct.periph_addr = (uint32_t) &USART_DATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH3, &dma_init_struct);
    
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_memory_to_memory_disable(DMA0, DMA_CH3);
    
    dma_deinit(DMA0, DMA_CH4);
    
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t) rx_buf.buf;
    dma_init(DMA0, DMA_CH4, &dma_init_struct);
    
    dma_circulation_enable(DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);
    
    
    
    //dma_channel_enable(DMA0, DMA_CH3);
    dma_interrupt_enable(DMA0, DMA_CH3, DMA_INT_FTF);
    dma_channel_enable(DMA0, DMA_CH4);
    
    usart_dma_receive_config(USART0, USART_DENR_ENABLE);
    usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
}