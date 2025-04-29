#pragma once
#include <stdint.h>

#define PATTERN_MAX_N 8

class PeriodPattern
{
public:
    uint16_t periods[PATTERN_MAX_N] = {0};
    uint8_t _tolerance_mul_256 = 0; // Tolerance (0 - 1.0) t = _tolerance_mul_256 / 256

    int getLength() const;

    PeriodPattern(const uint16_t signal_pattern[PATTERN_MAX_N], uint8_t tolerance = 0);

    bool match_window(const uint16_t signal_pattern[PATTERN_MAX_N]);

    void print();
};