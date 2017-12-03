#include "definitions.h"
#include "mqtt_abstraction.h"


void abstract_topic_handler::callback(char* topic, byte* payload, unsigned int length, void* arg)
{
    abstract_trampoline* tramp = static_cast<abstract_trampoline*>(arg);
    //brute force search
    for (unsigned int counter = 0; counter < tramp->get_size(); ++counter) {
        if (*((*tramp)[counter]) == topic) {
            (*tramp)[counter]->operator()(payload, length);
        }
    }
}

mqtt_publisher::mqtt_publisher(RF24Mesh& mesh)
    : mesh_(mesh)
{
}

void mqtt_publisher::initialize(const String& client_name, const IPAddress& my_ip, const IPAddress& broker_ip, abstract_trampoline* trampoline, unsigned int broker_port)
{
    my_ip_ = my_ip;
    broker_ip_ = broker_ip;
    broker_port_ = broker_port;
    client_name_ = client_name;
    trampoline_ = trampoline;
    mqtt_client_.setClient(eth_client_);
    mqtt_client_.setServer(broker_ip_, broker_port_);
    mqtt_client_.setCallback(abstract_topic_handler::callback, static_cast<void*>(trampoline_));
}

bool mqtt_publisher::publish(const String& topic, const uint8_t* payload, unsigned int plength)
{
    bool return_state = false;
    unsigned int counter = 0;
    do {
        counter++;
        if (mqtt_client_.connected()) {
            DEBUG_PRINT(Serial.print(counter));
            INFO_PRINT(Serial.print(F(" P ")));
            INFO_PRINT(Serial.println(topic));
            return_state = mqtt_client_.publish(topic.c_str(), payload, plength);
            delay(80); // some time for processing
        } else {
            INFO_PRINT(Serial.println(F("P F")));
            break;
        }
    } while (!return_state);

    return return_state;
}

bool mqtt_publisher::publish(const String& topic, const char* value)
{
    bool return_state = false;
    unsigned int counter = 0;
    do {
        counter++;
        if (mqtt_client_.connected()) {
            DEBUG_PRINT(Serial.print(counter));
            INFO_PRINT(Serial.print(F(" PS ")));
            INFO_PRINT(Serial.println(topic));
            return_state = mqtt_client_.publish(topic.c_str(), value);
            delay(80); // some time for processing
        } else {
            INFO_PRINT(Serial.println(F("PS F")));
            break;
        }
    } while (!return_state);

    return return_state;
}

bool mqtt_publisher::start_and_supervise(bool is_sub)
{
    //TODO: check if initialized
    bool return_state = false;
    if (!mesh_.checkConnection()) {
        mesh_.renewAddress();
    }
    if (!mqtt_client_.connected()) {
        if (mqtt_client_.connect(client_name_.c_str())) {
            INFO_PRINT(Serial.println(F("MQ O")));
            if (trampoline_ && is_sub) {
                resubscribe();
            }
            return_state = true;
        } else {
            //TODO: trigger reset if cannot connect for a long time
            INFO_PRINT(Serial.print(F("MQ F ")));
            INFO_PRINT(Serial.println(mqtt_client_.state()));
        }
    }
    return return_state;
}

void mqtt_publisher::stop()
{
    if (mqtt_client_.connected()) {
        mqtt_client_.disconnect();
    }
    mesh_.releaseAddress();
}

bool mqtt_publisher::loop()
{
    return mqtt_client_.loop();
}

mqtt_publisher::~mqtt_publisher()
{
}

void mqtt_publisher::resubscribe()
{
    for (unsigned int i = 0; i < trampoline_->get_size(); ++i) {
        bool return_state = false;
        unsigned int counter = 0;
        do {
            counter++;
            if (mqtt_client_.connected()) {
                DEBUG_PRINT(Serial.print(counter));
                INFO_PRINT(Serial.print(F(" S ")));
                INFO_PRINT(Serial.println((*trampoline_)[i]->get_topic()));
                return_state = mqtt_client_.subscribe((*trampoline_)[i]->get_topic(), 1);
                delay(80);
            } else {
                INFO_PRINT(Serial.println(F("S F")));
                break;
            }
        } while (!return_state);
    }
}
