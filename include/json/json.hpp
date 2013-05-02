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

    class Var;

    typedef std::vector<Var> Array;
    typedef std::map<std::string, Var> Object;

    std::string type2string(Type const type);

    std::string trim(std::string const & str);

    // split text for token and ignore token's inside string, array or object
    std::vector<std::string> split(std::string const & text, char const token);

    class ParseError: public std::exception {
        protected:
            std::string msg;
        public:
            ParseError(std::string const & msg) throw() {
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
            TypeError(Type const type, Type const wrong) throw() {
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

    class Var {

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
            std::string dumpArray(long const indent);
            std::string dumpObject(long const indent);
            void parseArray(std::string const & dump);
            void parseObject(std::string const & dump);

        public:
            Var();
            Var(char const c);
            Var(char const* string);
            Var(std::string const & string);
            Var(short const integer);
            Var(unsigned short const integer);
            Var(int const integer);
            Var(unsigned int const integer);
            Var(long const integer);
            Var(unsigned long const integer);
            Var(float const floating);
            Var(bool const boolean);
            Var(Array const array);
            Var(Object const object);

            inline Var& operator=(char const c) {
                this->type = STRING;
                this->string = "";
                this->string += c;
                return *this;
            }
            inline Var& operator=(char const * string) {
                this->type = STRING;
                this->string = std::string(string);
                return *this;
            }
            inline Var& operator=(std::string const & string) {
                this->type = STRING;
                this->string = string;
                return *this;
            }
            inline Var& operator=(short const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(unsigned short const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(int const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(unsigned int const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(long const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(unsigned long const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(float const floating) {
                this->type = FLOAT;
                this->floating = floating;
                return *this;
            }
            inline Var& operator=(bool const boolean) {
                this->type = BOOLEAN;
                this->boolean = boolean;
                return *this;
            }
            inline Var& operator=(Array const array) {
                this->type = ARRAY;
                this->array = array;
                return *this;
            }
            inline Var& operator=(Object const object) {
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

            bool operator==(Var const & other);
            bool operator!=(Var const & other);
            Var& operator=(Var const & other);
            Var& operator[](std::string const & key);
            Var& operator[](std::size_t const & index);

            void append(Var const value);

            void load(std::string const & str, bool const trim_it=true);
            std::string dump(long const indent=-1);
    };

}

#endif
