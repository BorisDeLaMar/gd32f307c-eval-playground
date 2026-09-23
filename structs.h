#include "defines.h"

typedef struct
{
    uint16_t head;
    uint16_t tail;
    uint8_t buf[TX_RX_BUF_SIZE];
} cycle_buf;