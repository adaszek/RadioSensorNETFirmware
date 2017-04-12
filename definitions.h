#pragma once
#include <stdint.h>

struct dht22_reading {
  uint32_t time_of_measurement;
  float temperature;
  float humidity;
};

extern uint32_t voltage_reading;

#if defined(DEBUG_PRINT_ENABLED)
#define DEBUG_PRINT(x) \
    {                  \
        x;             \
    }
#else
#define DEBUG_PRINT(x)
#endif

#if defined(INFO_PRINT_ENABLED)
#define INFO_PRINT(x) \
    {                 \
        x;            \
    }
#else
#define INFO_PRINT(x)
#endif
