#pragma once
#include "tools.h"

#include <string.h>
#include <Arduino.h>
#include <IPAddress.h>

class abstract_set_action {
    public:
        virtual ~abstract_set_action() {}
        virtual void execute(char* str_buffer) = 0;
        virtual const char* get_value_name() = 0;

};

class abstract_get_action {
    public:
        virtual ~abstract_get_action() {}
        virtual void execute() = 0;
        virtual const char* get_value_name() = 0;

};

template<unsigned int SET_SIZE, unsigned int GET_SIZE>
class action_handler {
    public:
        void process(char* str_buffer)
        {
            char* word = strtok(str_buffer, ",");
            if (strcmp("S", word) == 0) {
                word = strtok(NULL, ",");
                for(auto n : set_actions) {
                    if(strcmp(word, n->get_value_name()) == 0) {
                        word = strtok(NULL, ",");
                        n->execute(word);
                        return;
                    }
                }
            } else if (strcmp("G", word) == 0) {
                word = strtok(NULL, ",");
                for(auto n : get_actions) {
                    if(strcmp(word, n->get_value_name()) == 0) {
                        n->execute();
                        return;
                    }
                }
            } else {
                return;
            }
        }

        abstract_set_action* set_actions[SET_SIZE];
        abstract_get_action* get_actions[GET_SIZE];
};


template<typename T>
void execute_impl(char* str_buffer, T& value_to_set);

template<typename T, unsigned int BUFFER_SIZE>
class set_action : public abstract_set_action {
    public:
        set_action (const char* value_name, T& value_to_set)
            : value_to_set_(value_to_set)
        {
            tools::copy_string_to_buffer(value_name, value_name_, BUFFER_SIZE);
        }

        virtual ~set_action() {}

        const char* get_value_name() override
        {
            return value_name_;
        }

        void execute(char* str_buffer) override
        {
            execute_impl<T>(str_buffer, value_to_set_);
        }

    private:
        char value_name_[BUFFER_SIZE];
        T& value_to_set_;

};

template<typename T, unsigned int BUFFER_SIZE>
class get_action : public abstract_get_action {
    public:
        get_action (const char* value_name, T& value_to_get)
            : value_to_get_(value_to_get)
        {
            tools::copy_string_to_buffer(value_name, value_name_, BUFFER_SIZE);
        }

        virtual ~get_action() {}

        const char* get_value_name() override
        {
            return value_name_;
        }

        void execute() override
        {
            Serial.println(value_to_get_);
        }

    private:
        char value_name_[BUFFER_SIZE];
        T& value_to_get_;

};

template<typename T, unsigned int BUFFER_SIZE>
class serial_reader {
public:
    serial_reader(T& handler)
        : new_data_(false)
        , char_index_(0)
        , handler_(handler)
    {
    }

    void get_serial_data()
    {
        while (Serial.available() > 0 && new_data_ == false) {
            last_received_char_ = Serial.read();
            if (last_received_char_ != '\n') {
                reception_buffer_[char_index_] = last_received_char_;
                char_index_++;
                if (char_index_ >= BUFFER_SIZE) {
                    char_index_ = BUFFER_SIZE - 1;
                }
            } else {
                reception_buffer_[char_index_] = '\0';
                char_index_ = 0;
                new_data_ = true;
            }
        }
    }

    void handle_new_data()
    {
        if (new_data_ == true) {
            handler_.process(reception_buffer_);
            new_data_ = false;
        }
    }

    ~serial_reader()
    {
    }

private:
    serial_reader(const serial_reader&);
    serial_reader& operator=(const serial_reader&);

    char reception_buffer_[BUFFER_SIZE];
    bool new_data_;
    unsigned int char_index_;
    char last_received_char_;
    T& handler_;
};
