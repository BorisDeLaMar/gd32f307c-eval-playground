#include "utils.h"

#include "gd32f30x_usart.h"

#include "defines.h"

void reset_IDLE()
{
    uint32_t dummy = USART_DATA(USART0);
    (void) dummy;
}

uint32_t countCS()
{
    // Приводим числовой адрес к указателю на 32-битные слова
    volatile uint32_t *flash_ptr = (volatile uint32_t *) MAIN_FLASH_START_ADDR;
    
    // Рассчитываем точное количество 32-битных слов во Flash памяти
    // (Количество байт разницы / 4)
    uint32_t words_count = (MAIN_FLASH_END_ADDR - MAIN_FLASH_START_ADDR + 1) / 4;
    
    uint32_t cs = flash_ptr[0]; // Берем самое первое слово
    
    for(uint32_t i = 1; i < words_count; i++)
    {
        cs ^= flash_ptr[i]; // Шагаем строго по 32-битным словам, исключая выход за границы
    }
    
    return cs;
}
