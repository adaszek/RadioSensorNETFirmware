#pragma once
//#include <Arduino.h>

//class IPAddress;

namespace tools {
//String ip_to_string(const IPAddress& address);
void copy_string_to_buffer(const char* str, char* buffer, unsigned int buffer_size);
void copy_payload_to_buffer(const char* payload, unsigned int length, char* buffer, unsigned int buffer_size);
}
