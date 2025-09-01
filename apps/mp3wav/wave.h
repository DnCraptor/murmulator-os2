#pragma once
#include "m-os-api.h"
#include "m-os-api-timer.h"
#undef switch

#include <stdint.h>
#include <stdbool.h>

bool writeWavHeader(FIL* fo, uint32_t sample_rate, uint16_t num_channels);
bool updateWavHeader(FIL* fo, uint32_t num_samples, uint16_t num_channels);
