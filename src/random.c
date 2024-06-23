#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "random.h"

uint16_t random(uint16_t low_limit, uint16_t high_limit)
{
    if (low_limit > high_limit) {
        swap(low_limit, high_limit);
    }

    return (uint16_t)((rand() % (high_limit-low_limit)) + low_limit);
}