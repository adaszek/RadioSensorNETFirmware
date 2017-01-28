#pragma once
#include "tools.h"

#include <Arduino.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <RF24Ethernet.h> // for EthernetClass
#include <assert.h>
#include <stdlib.h>

class abstract_topic_handler;

class abstract_trampoline {
public:
    abstract_trampoline(const abstract_trampoline&) = delete;
    abstract_trampoline(const abstract_trampoline&&) = delete;
    abstract_trampoline& operator=(const abstract_trampoline&) = delete;

    abstract_trampoline() {}
    virtual ~abstract_trampoline() {}
    virtual abstract_topic_handler*& operator[](unsigned int idx) = 0;
    virtual unsigned int get_size() = 0;
};

template <unsigned int SIZE>
class tramp : public abstract_trampoline {
public:
    unsigned int get_size() override
    {
        return SIZE;
    }

    abstract_topic_handler*& operator[](unsigned int idx) override
    {
        return array_[idx];
    }

private:
    abstract_topic_handler* array_[SIZE];
};

class abstract_topic_handler {
public:
    abstract_topic_handler(const char* topic)
    {
        set_topic(topic);
    }

    abstract_topic_handler()
        : subscribed_topic_{ '\0' }
    {
    }

    abstract_topic_handler(const abstract_topic_handler&) = delete;
    abstract_topic_handler(const abstract_topic_handler&&) = delete;
    abstract_topic_handler& operator=(const abstract_topic_handler&) = delete;

    virtual ~abstract_topic_handler()
    {
    }

    bool operator==(char* T2)
    {
        return strcmp(subscribed_topic_, T2) == 0;
    }

    virtual void operator()(byte* payload, unsigned int length) = 0;

    static void callback(char* topic, byte* payload, unsigned int length, void* arg);

    void set_topic(const char* topic)
    {
        strncpy(subscribed_topic_, topic, SIZE);
    }

    const char* get_topic() const
    {
        return subscribed_topic_;
    }

private:
    static const unsigned int SIZE = 12;
    char subscribed_topic_[SIZE];
};

template <unsigned int MAX_PAYLOAD_BUFFER_SIZE>
class string_topic_handler : public abstract_topic_handler {
public:
    string_topic_handler()
        : abstract_topic_handler()
        , buffer_{ "unknown" }
    {
    }

    virtual ~string_topic_handler() {}

    virtual void operator()(byte* payload, unsigned int length) override
    {
        tools::copy_payload_to_buffer(payload, length, buffer_, MAX_PAYLOAD_BUFFER_SIZE);
    }

    void set_string(const char* str)
    {
        tools::copy_string_to_buffer(str, buffer_, MAX_PAYLOAD_BUFFER_SIZE);
    }

    const char* get_string() const
    {
        return buffer_;
    }

private:
    char buffer_[MAX_PAYLOAD_BUFFER_SIZE];
};

template <unsigned int MAX_PAYLOAD_BUFFER_SIZE>
class int_topic_handler : public abstract_topic_handler {
public:
    int_topic_handler()
        : abstract_topic_handler()
        , buffer_{ 0 }
        , value_(0)
    {
    }

    virtual ~int_topic_handler() {}

    virtual void operator()(byte* payload, unsigned int length) override
    {
        tools::copy_payload_to_buffer(payload, length, buffer_, MAX_PAYLOAD_BUFFER_SIZE);
        value_ = atoi(buffer_);
    }

    void set_int(int value)
    {
        value_ = value;
    }

    int get_int() const
    {
        return value_;
    }

private:
    char buffer_[MAX_PAYLOAD_BUFFER_SIZE];
    int value_;
};

class mqtt_publisher {
public:
    mqtt_publisher();
    void initialize(const String& client_name, const IPAddress& my_ip, const IPAddress& broker_ip, abstract_trampoline* trampoline = NULL, unsigned int broker_port = 1883);
    mqtt_publisher(const mqtt_publisher&) = delete;
    mqtt_publisher& operator=(const mqtt_publisher&) = delete;
    ~mqtt_publisher();
    bool publish(const String& topic, const uint8_t* payload, unsigned int plength);
    bool publish(const String& topic, const char* value);
    bool start_and_supervise(bool is_sub = false);
    void stop();
    bool loop();

private:
    IPAddress my_ip_;
    IPAddress broker_ip_;
    unsigned int broker_port_;
    String client_name_;
    EthernetClient eth_client_;
    abstract_trampoline* trampoline_;
    PubSubClient mqtt_client_;

    void resubscribe();
};
