#include "arduino-mock/Serial.h"
#include "../serial_reader.h"
#include <gtest/gtest.h>
#include "arduino-mock/Arduino.h"

using ::testing::Return;

template <>
void execute_impl(char* str_buffer, String& value_to_set) {
    value_to_set = String(str_buffer);
}

template <>
void execute_impl(char* str_buffer, IPAddress& value_to_set) {
    IPAddress address;
    address.fromString(str_buffer);
    value_to_set = address;
}

template <>
void execute_impl(char* str_buffer, uint8_t& value_to_set) {
    value_to_set = atoi(str_buffer);
}


/*
TEST(loop, pushed) {
    ArduinoMock* arduinoMock = arduinoMockInstance();
    SerialMock* serialMock = serialMockInstance();
    EXPECT_CALL(*arduinoMock, digitalRead(2))
        .WillOnce(Return(1));
    EXPECT_CALL(*serialMock, println(1, 10));
    EXPECT_CALL(*arduinoMock, delay(1));
    releaseSerialMock();
    releaseArduinoMock();
}*/
