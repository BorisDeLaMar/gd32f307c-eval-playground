#include "utils.h"

#include "gd32f30x_usart.h"

void reset_IDLE()
{
    uint32_t dummy = USART_DATA(USART0);
    (void) dummy;
}