#include "../definitions.h"
#include "battery.h"
#include "mqtt_abstraction.h"
#include "serial_reader.h"
#include "temperature_sensor.h"

#include <EEPROM.h>
#include <RF24.h>
#include <RF24Ethernet.h>
#include <RF24Mesh.h>
#include <RF24Network.h>

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

RF24 radio{ 7, 8 };
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24EthernetClass RF24Ethernet(radio, network, mesh); // Can't be wrapped due to some extern-magic

namespace rf24_network {
void start(const IPAddress& my_ip, const IPAddress& gateway_ip, RF24Mesh& mesh_ref)
{
    DEBUG_PRINT(Serial.println("ME S"));
    Ethernet.begin(my_ip);
    Ethernet.set_gateway(gateway_ip);

    if (mesh_ref.begin()) {
        INFO_PRINT(Serial.println(F("ME O")));
    } else {
        INFO_PRINT(Serial.println(F("ME F")));
    }
}

void check_state(RF24Mesh& mesh_ref)
{
    static uint32_t mesh_timer = 0;
    if (millis() - mesh_timer > 30000) { //Every 30 seconds, test mesh connectivity
        mesh_timer = millis();
        if (!mesh_ref.checkConnection()) {
            mesh_ref.renewAddress();
        }
    }
}
}

mqtt_publisher my_mqtt;

unsigned long int what_time_is_now = 0;
unsigned long int last_loop_timer = 0;
unsigned long int sleep_counter = 0;
const int sleep_for_s = 1;
const int publish_every_s = 5;

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

    pinMode(10, INPUT);
    digitalWrite(10, HIGH);
    network.setup_watchdog(7);

    rf24_network::start(configuration.sensor_ip, configuration.gateway_ip, mesh);
    rf24_network::check_state(mesh);

    last_loop_timer = millis();
}

void loop()
{
    RF24Ethernet.update();
    rf24_network::check_state(mesh);
    if (digitalRead(10) == LOW) {
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

        while (digitalRead(10) == LOW) {
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
    } else {
        my_mqtt.initialize(String(configuration.id), configuration.sensor_ip, configuration.broker_ip, NULL);

        what_time_is_now = millis() + (sleep_counter * sleep_for_s * 1000);
        if ((what_time_is_now - last_loop_timer > publish_every_s * 1000) || startup_flag) {
            last_loop_timer = what_time_is_now;
#if defined(DEBUG)
            Serial.print("free memory = ");
            Serial.println(getFreeMemory());
#endif
            //TODO: if cannot connect should be possible to go into config mode
            while (!my_mqtt.start_and_supervise()) {
                delay(200);
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
                my_mqtt.publish(publish_prefix + "a", "started");
                startup_flag = false;
            }
            my_mqtt.publish(publish_prefix + "t", (byte*)&(temp_reading.temperature), sizeof(((dht22_reading*)0)->temperature));
            my_mqtt.publish(publish_prefix + "h", (byte*)&(temp_reading.humidity), sizeof(((dht22_reading*)0)->humidity));
            my_mqtt.publish(publish_prefix + "v", (byte*)&(voltage_reading), sizeof(voltage_reading));
            my_mqtt.stop();
        }

        //TODO: sleep should be optional
        mesh.releaseAddress();
        radio.powerDown();
        Serial.flush();
        network.sleepNode(sleep_for_s, 255);
        radio.powerUp();
        mesh.renewAddress();
        sleep_counter++;
    }
}
