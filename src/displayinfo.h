#pragma once
#include <stdint.h>
#include "common.h"

uint32_t destroy_bounds(struct bounds **bnds_in);

uint32_t init_bounds(struct bounds **bnds_out);

void findBounds(struct bounds *bnds);