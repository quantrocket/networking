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
#include <cstdint>

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

    class ParseError: public std::exception {
        protected:
            std::string msg;
        public:
            ParseError(std::string const & msg) throw() {
                this->msg = "ParseError: " + msg;
                std::cerr << "JSON ParseError thrown: "
                          << this->msg << std::endl;
            }
            virtual ~ParseError() throw() {}
            virtual const char* what() const throw() {
                return this->msg.c_str();
            }
    };

    class Var {

        protected:
            Type type;
            std::string string;
            std::uint32_t integer;
            float floating; // string conversion cuts some decimal place
            bool boolean;
            Array array;
            Object object;

            inline std::string dumpString() {
                // @todo: special char escape, these are: "/\bfnrt
                //  as well was u-Unicode with 4 hex digits
                std::string dump = "";
                dump += '"';
                for (std::size_t i = 0; i < this->string.size(); i++) {
                    char buffer = this->string.at(i);
                    if (buffer == '"') {
                        dump += '\\';
                    }
                    dump += buffer;
                }
                dump += '"';
                return dump;
            }

            inline std::string dumpInteger() {
                return std::to_string(this->integer);
            }

            inline std::string dumpFloat() {
                return std::to_string(this->floating);
            }

            inline std::string dumpBoolean() {
                if (this->boolean) {
                    return "true";
                } else {
                    return "false";
                }
            }

            inline std::string dumpArray(long const indent) {
                std::string dump = "";
                dump += '[';
                if (indent > -1) {
                    dump += '\n';
                }
                for (auto n = this->array.begin(); n != this->array.end(); n++) {
                    if (n != this->array.begin()) {
                        dump += ',';
                        if (indent > -1) {
                            dump += '\n';
                        }
                    }
                    for (long i = 0; i <= indent; i++) {
                        dump += '\t';
                    }
                    if (indent > -1) {
                        dump += n->dump(indent + 1);
                    } else {
                        dump += n->dump(indent);
                    }
                }
                if (indent > -1) {
                    dump += '\n';
                }
                for (long i = 0; i < indent; i++) {
                    dump += '\t';
                }
                dump += ']';
                return dump;
            }

            inline std::string dumpObject(long const indent) {
                std::string dump = "";
                dump += '{';
                if (indent > -1) {
                    dump += '\n';
                }
                for (auto n = this->object.begin(); n != this->object.end(); n++) {
                    if (n != this->object.begin()) {
                        dump += ',';
                        if (indent > -1) {
                            dump += '\n';
                        }
                    }
                    for (long i = 0; i <= indent; i++) {
                        dump += '\t';
                    }
                    std::string key = n->first;
                    Var value = n->second;
                    dump += key + ':';
                    if (indent > -1) {
                        dump += '\t' + value.dump(indent + 1);
                    } else {
                        dump += value.dump(indent);
                    }
                }
                if (indent > -1) {
                    dump += '\n';
                }
                for (long i = 0; i < indent; i++) {
                    dump += '\t';
                }
                dump += '}';
                return dump;
            }

            inline void parseArray(std::string const & dump) {
                std::vector<std::string> parts = Var::split(dump, ',');
                Array result;
                // iterate all values
                for (auto p = parts.begin(); p != parts.end(); p++) {
                    Var value;
                    // parse and append value
                    value.load(*p, false);
                    result.push_back(value);
                }
                this->type = ARRAY;
                this->array = result;
            }

            inline void parseObject(std::string const & dump) {
                std::vector<std::string> parts = Var::split(dump, ',');
                Object result;
                // iterate all pairs
                for (auto p = parts.begin(); p != parts.end(); p++) {
                    std::vector<std::string> tmp = Var::split(*p, ':');
                    if (tmp.size() != 2) {
                        throw ParseError("object needs key and value");
                    }
                    // parse key and value
                    std::string key = tmp[0];
                    Var value;
                    value.load(tmp[1], false);
                    result[key] = value;
                }
                this->type = OBJECT;
                this->object = result;
            }

            static std::string trim(std::string const & str) {
                std::string out = "";
                bool opened_string = false;
                for (std::size_t i = 0; i < str.size(); i++) {
                    char buffer = str.at(i);
                    if (buffer == '"') {
                        // toggle being inside string
                        opened_string = !opened_string;
                        // but keep '"' character
                        out += buffer;
                    } else {
                        if (!opened_string) {
                            if (buffer == ' ' || buffer == '\t' || buffer == '\n') {
                                // outside string - cut them off
                            } else {
                                out += buffer;
                            }
                        } else {
                            // whitespace, tabulator or newline belongs to string
                            out += buffer;
                        }
                    }
                }
                return out;
            }

            // split text for token and ignore token's inside string, array or object
            static std::vector<std::string> split(std::string const & text, char const token) {
                std::vector<std::string> result;
                std::size_t i = 0;
                std::size_t last = 0;
                std::stack<Inside> inside;
                for (i = 0; i < text.size(); i++) {
                    char buffer = text.at(i);
                    if (buffer == '"') {
                        if (!inside.empty() && inside.top() == OPEN_STRING) {
                            // leave string
                            inside.pop();
                        } else {
                            // enter string
                            inside.push(OPEN_STRING);
                        }
                    } else if (buffer == '[') {
                        if (!inside.empty() && inside.top() == OPEN_STRING) {
                            // '[' belongs to string
                        } else {
                            // enter array
                            inside.push(OPEN_ARRAY);
                        }
                    } else if (buffer == '{') {
                        if (!inside.empty() && inside.top() == OPEN_STRING) {
                            // '{' belongs to string
                        } else {
                            // enter object
                            inside.push(OPEN_OBJECT);
                        }
                    } else if (buffer == ']') {
                        if (inside.empty() || inside.top() == OPEN_OBJECT) {
                            throw ParseError("found ']', but '}' expected");
                        } else if (inside.top() == OPEN_STRING) {
                            // ']' belongs to string
                        } else {
                            // leave array
                            inside.pop();
                        }
                    } else if (buffer == '}') {
                        if (inside.empty() || inside.top() == OPEN_ARRAY) {
                            throw ParseError("found '}' but expected ']'");
                        } else if (inside.top() == OPEN_STRING) {
                            // '}' belongs to string
                        } else {
                            // leave object
                            inside.pop();
                        }
                    } else if (inside.empty() && buffer == token) {
                        std::string tmp = text.substr(last, i - last);
                        result.push_back(tmp);
                        last = i + 1;
                    }
                }
                std::string tmp = text.substr(last, i - last);
                result.push_back(tmp);

                return result;
            }

        public:
            Var()
                : type(UNDEFINED) {
            }
            Var(char const c)
                 : type(STRING)
                 , string("") {
                 this->string += c;
            }
            Var(char const * string)
                : type(STRING)
                , string(string) {
            }
            Var(std::string const & string)
                : type(STRING)
                , string(string) {
            }
            Var(std::int16_t const integer)
                : type(INTEGER)
                , integer(integer) {
            }
            Var(std::uint16_t const integer)
                : type(INTEGER)
                , integer(integer) {
            }
            Var(std::int32_t const integer)
                 : type(INTEGER)
                 , integer(integer) {
            }
            Var(std::uint32_t const integer)
                 : type(INTEGER)
                 , integer(integer) {
            }
            Var(float const floating)
                : type(FLOAT)
                , floating(floating) {
            }
            Var(bool const boolean)
                : type(BOOLEAN)
                , boolean(boolean) {
            }
            Var(Array const array)
                : type(ARRAY)
                , array(array) {
            }
            Var(Object const object)
                : type(OBJECT)
                , object(object) {
            }

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
            inline Var& operator=(std::int16_t const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(std::uint16_t const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(std::int32_t const integer) {
                this->type = INTEGER;
                this->integer = integer;
                return *this;
            }
            inline Var& operator=(std::uint32_t const integer) {
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

            bool isNull() {
                return (this->type == UNDEFINED);
            }
            bool get(std::string & target) {
                if (this->type == STRING) {
                    target = this->string;
                    return true;
                }
                return false;
            }
            bool get(std::int16_t & target) {
                if (this->type == INTEGER) {
                    target = this->integer;
                    return true;
                }
                return false;
            }
            bool get(std::uint16_t & target) {
                if (this->type == INTEGER) {
                    target = this->integer;
                    return true;
                }
                return false;
            }
            bool get(std::int32_t & target) {
                if (this->type == INTEGER) {
                    target = this->integer;
                    return true;
                }
                return false;
            }
            bool get(std::uint32_t & target) {
                if (this->type == INTEGER) {
                    target = this->integer;
                    return true;
                }
                return false;
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
            bool get(float & target) {
                if (this->type == FLOAT) {
                    target = this->floating;
                    return true;
                }
                return false;
            }
            bool get(bool & target) {
                if (this->type == BOOLEAN) {
                    target = this->boolean;
                    return true;
                }
                return false;
            }
            bool get(Array & target) {
                if (this->type == ARRAY) {
                    target = this->array;
                    return true;
                }
                return false;
            }
            bool get(Object & target) {
                if (this->type == OBJECT) {
                    target = this->object;
                    return true;
                }
                return false;
            }

            bool operator==(Var const & other) {
                if (this->type == other.type) {
                    switch (this->type) {
                        case UNDEFINED:
                            return false;
                        case STRING:
                            return (this->string == other.string);
                            break;
                        case INTEGER:
                            return (this->integer == other.integer);
                            break;
                        case FLOAT:
                            return (this->floating == other.floating);
                            break;
                        case BOOLEAN:
                            return (this->boolean == other.boolean);
                            break;
                        case ARRAY: {
                            if (this->array.size() != other.array.size()) {
                                return false;
                            }
                            auto p = this->array.begin();
                            auto q = other.array.begin();
                            while (p != this->array.end()) {
                                if (*p != *q) {
                                    return false;
                                }
                                p++;
                                q++;
                            }
                            return true;
                            break;
                        }
                        case OBJECT: {
                            if (this->object.size() != other.object.size()) {
                                return false;
                            }
                            auto p = this->object.begin();
                            auto q = other.object.begin();
                            while (p != this->object.end()) {
                                if (p->first != q->first || p->second != q->second) {
                                    return false;
                                }
                                p++;
                                q++;
                            }
                            return true;
                            break;
                        }
                    }
                }
                return false;
            }

            bool operator!=(Var const & other) {
                return !(*this == other);
            }

            Var& operator=(Var const & other) {
                if (*this != other) {
                    // reset all values
                    this->string   = "";
                    this->integer  = 0l;
                    this->floating = 0.0f;
                    this->boolean  = false;
                    this->array.clear();
                    this->object.clear();
                    // adapt type
                    this->type = other.type;
                    // adapt value
                    switch (this->type) {
                        case UNDEFINED:
                            break;
                        case STRING:
                            this->string = other.string;
                            break;
                        case INTEGER:
                            this->integer = other.integer;
                            break;
                        case FLOAT:
                            this->floating = other.floating;
                            break;
                        case BOOLEAN:
                            this->boolean = other.boolean;
                            break;
                        case ARRAY:
                            this->array = other.array;
                            break;
                        case OBJECT:
                            this->object = other.object;
                            break;
                    }
                }
                return *this;
            }

            Var& operator[](std::string const & key) {
                this->type = OBJECT;
                return this->object[key];
            }

            Var& operator[](std::size_t const & index) {
                this->type = ARRAY;
                return this->array[index];
            }

            void append(Var const value) {
                this->type = ARRAY;
                this->array.push_back(value);
            }

            // @todo void remove(Var const value)

            void load(std::string const & str, bool const trim_it=true) {
                // trim spaces, tabulators and newlines
                std::string dump = str;
                if (trim_it) {
                    dump = Var::trim(dump);
                }
                // Begin Parsing ...
                if (dump == "null") {
                    this->type = UNDEFINED;
                    return;
                } else if (dump.front() == '{' && dump.back() == '}') {
                    this->parseObject(dump.substr(1, dump.size() - 2));
                } else if (dump.front() == '[' && dump.back() == ']') {
                    this->parseArray(dump.substr(1, dump.size() - 2));
                } else if (dump == "true" || dump == "false") {
                    this->boolean = (dump == "true");
                    this->type = BOOLEAN;
                } else {
                    // Try to Parse Integer Number
                    try {
                        long integer = std::stol(dump);
                        if (std::to_string(integer) == dump) {
                            this->integer = integer;
                            this->type = INTEGER;
                            return;
                        }
                    } catch (const std::out_of_range& oor) {
                    } catch (const std::invalid_argument& ia) {
                    }
                    // Try to Parse Floating Point Number
                    try {
                        this->floating = std::stod(dump);
                        this->type = FLOAT;
                        return;
                    } catch (const std::out_of_range& oor) {
                    } catch (const std::invalid_argument& ia) {
                    }
                    // Try to Parse String
                    if (dump.front() == '"' && dump.back() == '"') {
                        this->string = dump.substr(1, dump.size() - 2);
                        this->type = STRING;
                        return;
                    }
                    if (dump.size() == 0) {
                        // Empty set of data - Nothing to load
                        return;
                    }
                    // Parse Error
                    throw ParseError(dump);
                }
            }

            std::string dump(std::size_t const indent=-1) {
                switch (this->type) {
                    case UNDEFINED:
                        return "null";
                        break;
                    case STRING:
                        return Var::dumpString();
                        break;
                    case INTEGER:
                        return Var::dumpInteger();
                        break;
                    case FLOAT:
                        return Var::dumpFloat();
                        break;
                    case BOOLEAN:
                        return Var::dumpBoolean();
                        break;
                    case ARRAY:
                        return Var::dumpArray(indent);
                        break;
                    case OBJECT:
                        return Var::dumpObject(indent);
                        break;
                }
                return "";
            }

            static std::string type2string(Type const type) {
                switch (type) {
                    case UNDEFINED:
                        return "undefined";
                        break;
                    case STRING:
                        return "String";
                        break;
                    case INTEGER:
                        return "Integer";
                        break;
                    case FLOAT:
                        return "Float";
                        break;
                    case BOOLEAN:
                        return "Boolean";
                        break;
                    case ARRAY:
                        return "Array";
                        break;
                    case OBJECT:
                        return "Object";
                        break;
                }
                return "unknown";
            }
    };

}

#endif
