/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a json-based networking framework for games and other software.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#ifndef JSON_INCLUDE_GUARD
#define JSON_INCLUDE_GUARD

#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace json {

    enum Type {
        UNDEFINED = 0,
        STRING    = 1,
        INTEGER   = 2,
        FLOAT     = 3,
        BOOLEAN   = 4,
        ARRAY     = 5,
        OBJECT    = 6
    };

    enum Inside {
        OPEN_STRING = 0,
        OPEN_ARRAY  = 1,
        OPEN_OBJECT = 2
    };

    class Value;

    typedef std::vector<Value> Array;
    typedef std::map<std::string, Value> Object;

    std::string type2string(Type type);

    std::string trim(const std::string& str);

    // split text for token and ignore token's inside string, array or object
    std::vector<std::string> split(const std::string& text, char token);

    class ParseError: public std::exception {

        protected:
            std::string msg;

        public:
            ParseError(const std::string& msg) throw() {
                this->msg = "ParseError: '" + msg + "' does not contain"
                           + " valid JSON-styled information";
                std::cerr << "JSON error occured: "
                          << this->msg << std::endl;
            }

            virtual ~ParseError() throw() {}

            virtual const char* what() const throw() {
                return this->msg.c_str();
            }

    };

    class TypeError: public std::exception {

        protected:
            std::string msg;

        public:
            TypeError(Type type, Type wrong) throw() {
                this->msg = "TypeError: " + type2string(type) + " cannot be"
                           +" access as if " + type2string(wrong);
                std::cerr << "JSON error occured: "
                          << this->msg << std::endl;
            }

            virtual ~TypeError() throw() {}

            virtual const char* what() const throw() {
                return this->msg.c_str();
            }

    };

    class Value {

        protected:
            Type type;
            std::string string;
            long integer;
            float floating;
            bool boolean;
            Array array;
            Object object;

            std::string dumpString();
            std::string dumpInteger();
            std::string dumpFloat();
            std::string dumpBoolean();
            std::string dumpArray(long indent);
            std::string dumpObject(long indent);
            void parseArray(const std::string& dump);
            void parseObject(const std::string& dump);

        public:
            Value();
            Value(char c);
            Value(const char* string);
            Value(const std::string& string);
            Value(short integer);
            Value(unsigned short integer);
            Value(int integer);
            Value(unsigned int integer);
            Value(long integer);
            Value(unsigned long integer);
            Value(float floating);
            Value(bool boolean);
            Value(Array array);
            Value(Object object);

            inline Value& operator=(char c) {
                this->type = STRING;
                this->string = "";
                this->string += c;
                return *this;
            }
            inline Value& operator=(const char* string) {
                this->type = STRING;
                this->string = std::string(string);
                return *this;
            }
            inline Value& operator=(const std::string& string) {
                this->type = STRING;
                this->string = string;
                return *this;
            }
            inline Value& operator=(short integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(unsigned short integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(int integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(unsigned int integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(long integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(unsigned long integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Value& operator=(float floating) {
                this->type = FLOAT;
                this->floating = floating;
                return *this;
            }
            inline Value& operator=(bool boolean) {
                this->type = BOOLEAN;
                this->boolean = boolean;
                return *this;
            }
            inline Value& operator=(Array array) {
                this->type = ARRAY;
                this->array = array;
                return *this;
            }
            inline Value& operator=(Object object) {
                this->type = OBJECT;
                this->object = object;
                return *this;
            }

            bool isString();
            bool isInteger();
            bool isFloat();
            bool isBoolean();
            bool isArray();
            bool isObject();
            bool isNull();

            std::string getString();
            long getInteger();
            float getFloat();
            bool getBoolean();
            Array getArray();
            Object getObject();

            bool operator==(const Value& other);
            bool operator!=(const Value& other);
            Value& operator=(const Value& other);
            Value& operator[](const std::string& key);
            Value& operator[](const long& index);

            void append(Value value);

            void load(const std::string& str, bool trim_it=true);
            std::string dump(long indent=-1);
    };

}

#endif
