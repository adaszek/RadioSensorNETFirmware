#pragma once
#include <dht.h>

#include "../definitions.h"

class temperature_sensor
{
    public:
        temperature_sensor(int pin);
        ~temperature_sensor();

        struct debug_stats
        {
            uint32_t total;
            uint32_t ok;
            uint32_t crc_error;
            uint32_t time_out;
            uint32_t connect;
            uint32_t ack_l;
            uint32_t ack_h;
            uint32_t unknown;
        };
        
        // yep i know items 21 and 28 from Effective C++ by S. Meyers
        // did it for performance reasons
        const dht22_reading& read();

        // yep i know items 21 and 28 from Effective C++ by S. Meyers
        // did it for performance reasons
        const debug_stats& get_stats() const
        {
            return stats_;
        }

    private:
        temperature_sensor(const temperature_sensor&);
        temperature_sensor& operator=(const temperature_sensor&);

        int pin_;
        dht dht_;
        debug_stats stats_;
        dht22_reading reading_;
};
