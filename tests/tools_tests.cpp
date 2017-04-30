#include "../tools.h"
#include <gtest/gtest.h>

TEST(bufferOperationsTC, stringcopy) {
    const char* str = "test text";
    char buffer[20];
    tools::copy_string_to_buffer(str, buffer, sizeof(buffer));

    ASSERT_STREQ(str, buffer);
    ASSERT_EQ(buffer[10], '\0');
}

TEST(bufferOperationsTC, stringtrim) {
    const char* str = "test text";
    char buffer[4];

    tools::copy_string_to_buffer(str, buffer, sizeof(buffer));

    ASSERT_STREQ("tes", buffer);
    ASSERT_EQ(buffer[3], '\0');
}

TEST(bufferOperationsTC, payloadcopy) {
    const char input_buffer[] = { 0, 2, 'A', '@', 0xA };
    char buffer[20];

    tools::copy_payload_to_buffer(input_buffer, sizeof(input_buffer), buffer, sizeof(buffer));

    ASSERT_EQ(memcmp(input_buffer, buffer, sizeof(input_buffer)), 0);
    ASSERT_EQ(buffer[sizeof(input_buffer) + 2], '\0');
}

TEST(bufferOperationsTC, payloadtrim) {
    const char input_buffer[] = { 0, 2, 'A', '@', 0xA };
    char buffer[3];

    tools::copy_payload_to_buffer(input_buffer, sizeof(input_buffer), buffer, sizeof(buffer));

    ASSERT_EQ(memcmp(input_buffer, buffer, sizeof(buffer) - 1), 0);
    ASSERT_GT(memcmp(input_buffer, buffer, sizeof(buffer)), 0);
    ASSERT_EQ(buffer[sizeof(buffer) - 1], '\0');
}
