#include "temperature_sensor.h"

temperature_sensor::temperature_sensor(int pin):
    pin_(pin),
    stats_{0, 0, 0, 0, 0, 0, 0, 0}
{
}

temperature_sensor::~temperature_sensor()
{}

// yep i know items 21 and 28 from Effective C++ by S. Meyers
// did it for performance reasons
const dht22_reading& temperature_sensor::read()
{
    int chk = dht_.read22(pin_);
    stats_.total++;
    //in case of failure function shall return latest know good values
    switch (chk)
    {
    case DHTLIB_OK:
        stats_.ok++;
        reading_.temperature = dht_.temperature; //double to float
        reading_.humidity = dht_.humidity; //double to float
        reading_.time_of_measurement = micros();
        break;
    case DHTLIB_ERROR_CHECKSUM:
        stats_.crc_error++;
        delay(2000); //TODO: check if neded
        break;
    case DHTLIB_ERROR_TIMEOUT:
        stats_.time_out++;
        break;
    case DHTLIB_ERROR_CONNECT:
        stats_.connect++;
        break;
    case DHTLIB_ERROR_ACK_L:
        stats_.ack_l++;
        break;
    case DHTLIB_ERROR_ACK_H:
        stats_.ack_h++;
        break;
    default:
        stats_.unknown++;
        break;
    }
    return reading_;
}
