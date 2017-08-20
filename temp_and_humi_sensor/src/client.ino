#include "definitions.h"
#include "battery.h"
#include "mqtt_abstraction.h"
#include "serial_reader.h"
#include "temperature_sensor.h"

#include <EEPROM.h>
#include <RF24.h>
#include <RF24Ethernet.h>
#include <RF24Mesh.h>
#include <RF24Network.h>

#include <memory>
#include <type_traits>

#if defined(DEBUG)
#include "memdebug.h"
#endif

struct config {
    char version[4]; // version of the config structure
    uint8_t id;
    uint8_t sensor_ip[4];
    uint8_t gateway_ip[4];
    uint8_t broker_ip[4];
    char location[8];
} configuration = { "001", 1, { 10, 10, 3, 30 }, { 10, 10, 3, 13 }, { 10, 10, 3, 13 }, { "unk" } };

const char* rl_topic_prefix = { "r/l/" };

const uint8_t config_pin = 9;

RF24 radio{ 7, 8 };
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24EthernetClass RF24Ethernet(radio, network, mesh); // Can't be wrapped due to some extern-magic

namespace rf24_network {
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

mqtt_publisher my_mqtt(mesh);

unsigned long int what_time_is_now = 0;
unsigned long int last_loop_timer = 0;
unsigned long int sleep_counter = 0;
const int sleep_for_s = 1;
const int publish_every_s = 1;

bool startup_flag = true;

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


void setup()
{
    Serial.begin(57600);
    if (EEPROM[0] == configuration.version[0] && EEPROM[1] == configuration.version[1] && EEPROM[2] == configuration.version[2]) {
        EEPROM.get(0, configuration);
        DEBUG_PRINT(Serial.println(F("C R")));
    } else {
        DEBUG_PRINT(Serial.println(F("C D")));
    }

    pinMode(config_pin, INPUT);
    digitalWrite(config_pin, HIGH);
    network.setup_watchdog(7);

    rf24_network::start(configuration.sensor_ip, configuration.gateway_ip);

    last_loop_timer = millis();
}

void configuration_mode()
{
    INFO_PRINT(Serial.println(F("CM")));

    String location(configuration.location);
    IPAddress sip(configuration.sensor_ip[0], configuration.sensor_ip[1], configuration.sensor_ip[2], configuration.sensor_ip[3]);
    IPAddress gip(configuration.gateway_ip[0], configuration.gateway_ip[1], configuration.gateway_ip[2], configuration.gateway_ip[3]);
    IPAddress bip(configuration.broker_ip[0], configuration.broker_ip[1], configuration.broker_ip[2], configuration.broker_ip[3]);

    uint8_t initial_id = configuration.id;
    
    set_action<uint8_t, 4> set_id("id", configuration.id);
    set_action<String, 4> set_location("loc", location);
    set_action<IPAddress, 4> set_sip("sip", sip);
    set_action<IPAddress, 4> set_gip("gip", gip);
    set_action<IPAddress, 4> set_bip("bip", bip);

    get_action<uint8_t, 4> get_id("id", configuration.id);
    get_action<char[8], 4> get_location("loc", configuration.location);
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

    while (digitalRead(config_pin) == LOW) {
        serial_receiver.get_serial_data();
        serial_receiver.handle_new_data();
    }

    if (initial_id != configuration.id) {
        startup_flag = true;
    }

    strncpy(configuration.location, location.c_str(), sizeof(configuration.location));
    configuration.location[sizeof(configuration.location) - 1] = '\0';

    configuration.sensor_ip[0] = sip[0];
    configuration.sensor_ip[1] = sip[1];
    configuration.sensor_ip[2] = sip[2];
    configuration.sensor_ip[3] = sip[3];

    configuration.gateway_ip[0] = gip[0];
    configuration.gateway_ip[1] = gip[1];
    configuration.gateway_ip[2] = gip[2];
    configuration.gateway_ip[3] = gip[3];

    configuration.broker_ip[0] = bip[0];
    configuration.broker_ip[1] = bip[1];
    configuration.broker_ip[2] = bip[2];
    configuration.broker_ip[3] = bip[3];

    EEPROM.put(0, configuration);

}

void processing_mode()
{
//    RF24Ethernet.update();
    what_time_is_now = millis() + (sleep_counter * sleep_for_s * 1000);
    DEBUG_PRINT(Serial.println(what_time_is_now));
    if ((what_time_is_now - last_loop_timer > publish_every_s * 1000) || startup_flag) {
        radio.powerUp();
        my_mqtt.initialize(String(configuration.id), configuration.sensor_ip, configuration.broker_ip, NULL);
        last_loop_timer = what_time_is_now;
#if defined(DEBUG)
        Serial.print("free memory = ");
        Serial.println(getFreeMemory());
#endif
        //TODO: if cannot connect should be possible to go into config mode
        while (!my_mqtt.start_and_supervise()) {
//            RF24Ethernet.update();
            delay(200);
            DEBUG_PRINT(Serial.println("waiting"));
        }

        temperature_sensor temp_sensor(2);
        dht22_reading temp_reading;
        uint32_t voltage_reading;

        temp_reading = temp_sensor.read();
        voltage_reading = battery_voltage::read();
        
        String publish_prefix;
        publish_prefix.reserve(20);
        publish_prefix.concat(rl_topic_prefix);
        publish_prefix.concat(configuration.location);
        publish_prefix.concat("/s/");
        publish_prefix.concat(String(configuration.id));
        publish_prefix.concat("/");
        
        if (startup_flag)
        {
            String capabilities = "r:t,h,v;w:0;s:";
            capabilities += sleep_for_s;
            my_mqtt.publish(publish_prefix + "a", capabilities.c_str());
            startup_flag = false;
        }
        my_mqtt.publish(publish_prefix + "t", (byte*)&(temp_reading.temperature), sizeof(((dht22_reading*)0)->temperature));
        my_mqtt.publish(publish_prefix + "h", (byte*)&(temp_reading.humidity), sizeof(((dht22_reading*)0)->humidity));
        my_mqtt.publish(publish_prefix + "v", (byte*)&(voltage_reading), sizeof(voltage_reading));
        my_mqtt.stop();
        radio.powerDown();
    } else {
        DEBUG_PRINT(Serial.println("timer else"));
    }
}

void sleeping_mode()
{
    DEBUG_PRINT(Serial.println("sleeping"));
    Serial.flush();
    network.sleepNode(sleep_for_s, 255);
    sleep_counter++;
}

void loop()
{
    if (digitalRead(config_pin) == LOW) {
        configuration_mode();
    } else {
        processing_mode();
        //TODO: sleep should be optional
        sleeping_mode();
    }
}
