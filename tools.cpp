#include "tools.h"
#include <IPAddress.h>

namespace tools {
/*
String ip_to_string(const IPAddress& address)
{
    char ret_buffer[16];
    // sprintf if very heavy function - think twice before use
    sprintf(ret_buffer, "%u.%u.%u.%u", address[0], address[1], address[2], address[3]);
    return String(ret_buffer);
}
*/
void copy_string_to_buffer(const char* str, char* buffer, unsigned int buffer_size)
{
    memset(buffer, '\0', buffer_size);
    strncpy(buffer, str, buffer_size);
    buffer[buffer_size - 1] = '\0';

}

void copy_payload_to_buffer(const byte* payload, unsigned int length, char* buffer, unsigned int buffer_size)
{
    memset(buffer, '\0', buffer_size);
    if (length < buffer_size) {
        memcpy(buffer, payload, length);
    } else {
        memcpy(buffer, payload, buffer_size);
        buffer[buffer_size - 1] = '\0';
    }
}
}
