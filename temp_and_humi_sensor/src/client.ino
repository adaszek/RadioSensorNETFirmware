#include "battery.h"
#include "definitions.h"
#include "serial_reader.h"
#include "temperature_sensor.h"
#include <EEPROM.h>
#include <MQTTClient.h>
#include <RF24.h>
#include <RF24Ethernet.h>
#include <RF24Mesh.h>
#include <RF24Network.h>

#if defined(DEBUG)
#include "memdebug.h"
#endif

namespace {
namespace config {
    struct config_t {
        char version[4]; // version of the config structure
        uint8_t id;
        uint8_t sensor_ip[4];
        uint8_t gateway_ip[4];
        uint8_t broker_ip[4];
        char location[8];
    } data { "001", 1, { 10, 10, 3, 30 }, { 10, 10, 3, 13 }, { 10, 10, 3, 13 }, { "unk" } };

    const char* rl_topic_prefix { "r/l/" };
    const uint8_t pin {9};
    const int sleep_for_s {1};
    const int publish_every_s {1};
}

unsigned long int what_time_is_now {0};
unsigned long int last_loop_timer {0};
unsigned long int sleep_counter {0};
bool startup_flag {true};

namespace rf24 {
    RF24 radio { 7, 8 };
    RF24Network network {radio};
    RF24Mesh mesh {radio, network};
    EthernetClient ethernet;
    
    void start(const IPAddress& my_ip, const IPAddress& gateway_ip)
    {
        DEBUG_PRINT(Serial.println("ME S"));
        Ethernet.begin(my_ip);
        Ethernet.set_gateway(gateway_ip);
        if (mesh.begin()) {
            INFO_PRINT(Serial.println(F("ME O")));
        } else {
            INFO_PRINT(Serial.println(F("ME F")));
        }
    }
}
}

//This must be global and visible outside
RF24EthernetClass RF24Ethernet(rf24::radio, rf24::network, rf24::mesh); // Can't be wrapped due to some extern-magic

namespace {
namespace mqtt {
    MQTTClient client;

    void connect()
    {
        DEBUG_PRINT(Serial.print(F("MQ")));
        while (!client.connect("arduino")) {
            DEBUG_PRINT(Serial.print(F(".")));
            delay(1000);
        }
        DEBUG_PRINT(Serial.println(F("OK")));
    }

    void loop()
    {
        client.loop();
        if (!client.connected()) {
            connect();
        }
    }
}


void configuration_mode()
{
    INFO_PRINT(Serial.println(F("CM")));
    String location(config::data.location);
    IPAddress sip(config::data.sensor_ip[0], config::data.sensor_ip[1], config::data.sensor_ip[2], config::data.sensor_ip[3]);
    IPAddress gip(config::data.gateway_ip[0], config::data.gateway_ip[1], config::data.gateway_ip[2], config::data.gateway_ip[3]);
    IPAddress bip(config::data.broker_ip[0], config::data.broker_ip[1], config::data.broker_ip[2], config::data.broker_ip[3]);

    uint8_t initial_id = config::data.id;

    set_action<uint8_t, 4> set_id("id", config::data.id);
    set_action<String, 4> set_location("loc", location);
    set_action<IPAddress, 4> set_sip("sip", sip);
    set_action<IPAddress, 4> set_gip("gip", gip);
    set_action<IPAddress, 4> set_bip("bip", bip);

    get_action<uint8_t, 4> get_id("id", config::data.id);
    get_action<char[8], 4> get_location("loc", config::data.location);
    get_action<IPAddress, 4> get_sip("sip", sip);
    get_action<IPAddress, 4> get_gip("gip", gip);
    get_action<IPAddress, 4> get_bip("bip", bip);

    typedef action_handler<5, 5> action_handler_type;
    action_handler_type handler;
    serial_reader<action_handler_type, 22> serial_receiver(handler);

    handler.set_actions[0] = &set_id;
    handler.set_actions[1] = &set_location;
    handler.set_actions[2] = &set_sip;
    handler.set_actions[3] = &set_gip;
    handler.set_actions[4] = &set_bip;

    handler.get_actions[0] = &get_location;
    handler.get_actions[1] = &get_id;
    handler.get_actions[2] = &get_sip;
    handler.get_actions[3] = &get_gip;
    handler.get_actions[4] = &get_bip;

    while (digitalRead(config::pin) == LOW) {
        serial_receiver.get_serial_data();
        serial_receiver.handle_new_data();
    }

    if (initial_id != config::data.id) {
        startup_flag = true;
    }

    strncpy(config::data.location, location.c_str(), sizeof(config::data.location));
    config::data.location[sizeof(config::data.location) - 1] = '\0';

    config::data.sensor_ip[0] = sip[0];
    config::data.sensor_ip[1] = sip[1];
    config::data.sensor_ip[2] = sip[2];
    config::data.sensor_ip[3] = sip[3];

    config::data.gateway_ip[0] = gip[0];
    config::data.gateway_ip[1] = gip[1];
    config::data.gateway_ip[2] = gip[2];
    config::data.gateway_ip[3] = gip[3];

    config::data.broker_ip[0] = bip[0];
    config::data.broker_ip[1] = bip[1];
    config::data.broker_ip[2] = bip[2];
    config::data.broker_ip[3] = bip[3];

    EEPROM.put(0, config::data);
}

void processing_mode()
{
    what_time_is_now = millis() + (sleep_counter * config::sleep_for_s * 1000);
    DEBUG_PRINT(Serial.println(what_time_is_now));
    if ((what_time_is_now - last_loop_timer > config::publish_every_s * 1000) || startup_flag) {
        rf24::radio.powerUp();

        last_loop_timer = what_time_is_now;
#if defined(DEBUG)
        Serial.print("free memory = ");
        Serial.println(getFreeMemory());
#endif
        mqtt::client.begin(config::data.broker_ip, 1883, rf24::ethernet);
        mqtt::loop();

        temperature_sensor temp_sensor(2);

        dht22_reading temp_reading {temp_sensor.read()};
        uint32_t voltage_reading {static_cast<uint32_t>(battery_voltage::read())};

        String publish_prefix;
        publish_prefix.reserve(20);
        publish_prefix.concat(config::rl_topic_prefix);
        publish_prefix.concat(config::data.location);
        publish_prefix.concat("/s/");
        publish_prefix.concat(String(config::data.id));
        publish_prefix.concat("/");

        if (startup_flag) {
            String capabilities = "r:t,h,v;w:0;s:";
            capabilities += config::sleep_for_s;
            mqtt::client.publish(publish_prefix + "a", capabilities);
            startup_flag = false;
        }
        mqtt::client.publish(publish_prefix + "t", String(temp_reading.temperature));
        mqtt::client.publish(publish_prefix + "h", String(temp_reading.humidity));
        mqtt::client.publish(publish_prefix + "v", String(voltage_reading));
        mqtt::client.disconnect();
        rf24::radio.powerDown();
    } else {
        DEBUG_PRINT(Serial.println("timer else"));
    }
}

void sleeping_mode()
{
    DEBUG_PRINT(Serial.println("sleeping"));
    Serial.flush();
    rf24::network.sleepNode(config::sleep_for_s, 255);
    sleep_counter++;
}
}

template <>
void execute_impl(char* str_buffer, String& value_to_set)
{
    value_to_set = String(str_buffer);
}

template <>
void execute_impl(char* str_buffer, IPAddress& value_to_set)
{
    IPAddress address;
    address.fromString(str_buffer);
    value_to_set = address;
}

template <>
void execute_impl(char* str_buffer, uint8_t& value_to_set)
{
    value_to_set = atoi(str_buffer);
}

void setup()
{
    Serial.begin(57600);
    if (EEPROM[0] == config::data.version[0] && EEPROM[1] == config::data.version[1] && EEPROM[2] == config::data.version[2]) {
        EEPROM.get(0, config::data);
        DEBUG_PRINT(Serial.println(F("C R")));
    } else {
        DEBUG_PRINT(Serial.println(F("C D")));
    }

    pinMode(config::pin, INPUT);
    digitalWrite(config::pin, HIGH);

    rf24::network.setup_watchdog(7);
    rf24::start(config::data.sensor_ip, config::data.gateway_ip);

    last_loop_timer = millis();
}

void loop()
{
    if (digitalRead(config::pin) == LOW) {
        configuration_mode();
    } else {
        processing_mode();
        //TODO: sleep should be optional
        sleeping_mode();
    }
}
