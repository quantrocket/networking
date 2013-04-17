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

#include "json.hpp"

namespace json {

    std::string type2string(Type type) {
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

    std::string trim(const std::string& str) {
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
    std::vector<std::string> split(const std::string& text, char token) {
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
                    throw ParseError(text);
                } else if (inside.top() == OPEN_STRING) {
                    // ']' belongs to string
                } else {
                    // leave array
                    inside.pop();
                }
            } else if (buffer == '}') {
                if (inside.empty() || inside.top() == OPEN_ARRAY) {
                    throw ParseError(text);
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

    std::string Value::dumpString() {
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

    std::string Value::dumpInteger() {
        return std::to_string(this->integer);
    }

    std::string Value::dumpFloat() {
        return std::to_string(this->floating);
    }

    std::string Value::dumpBoolean() {
        if (this->boolean) {
            return "true";
        } else {
            return "false";
        }
    }

    std::string Value::dumpArray(long indent) {
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

    std::string Value::dumpObject(long indent) {
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
            Value value = n->second;
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

    void Value::parseArray(const std::string& dump) {
        std::vector<std::string> parts = split(dump, ',');
        Array result;
        // iterate all values
        for (auto p = parts.begin(); p != parts.end(); p++) {
            Value value;
            // parse and append value
            value.load(*p, false);
            result.push_back(value);
        }
        this->type = ARRAY;
        this->array = result;
    }

    void Value::parseObject(const std::string& dump) {
        std::vector<std::string> parts = split(dump, ',');
        Object result;
        // iterate all pairs
        for (auto p = parts.begin(); p != parts.end(); p++) {
            std::vector<std::string> tmp = split(*p, ':');
            if (tmp.size() != 2) {
                std::cout << tmp.size() << " != 2\n";
                throw ParseError(*p);
            }
            // parse key and value
            std::string key = tmp[0];
            Value value;
            value.load(tmp[1], false);
            result[key] = value;
        }
        this->type = OBJECT;
        this->object = result;
    }

    Value::Value()
        : type(UNDEFINED) {
    }

    Value::Value(char c)
         : type(STRING)
         , string("") {
         this->string += c;
    }

    Value::Value(const char* string)
            : type(STRING)
            , string(string) {
    }

    Value::Value(const std::string& string)
        : type(STRING)
        , string(string) {
    }

    Value::Value(short integer)
        : type(INTEGER)
        , integer(integer) {
    }

    Value::Value(unsigned short integer)
        : type(INTEGER)
        , integer(integer) {
    }

    Value::Value(int integer)
         : type(INTEGER)
         , integer(integer) {
    }

    Value::Value(unsigned int integer)
         : type(INTEGER)
         , integer(integer) {
    }

    Value::Value(long integer)
         : type(INTEGER)
         , integer(integer) {
    }

    Value::Value(unsigned long integer)
         : type(INTEGER)
         , integer(integer) {
    }

    Value::Value(float floating)
        : type(FLOAT)
        , floating(floating) {
    }

    Value::Value(bool boolean)
        : type(BOOLEAN)
        , boolean(boolean) {
    }

    Value::Value(Array array)
        : type(ARRAY)
        , array(array) {
    }

    Value::Value(Object object)
        : type(OBJECT)
        , object(object) {
    }

    bool Value::isString() {
        return this->type == STRING;
    }
    bool Value::isInteger() {
        return this->type == INTEGER;
    }
    bool Value::isFloat() {
        return this->type == FLOAT;
    }
    bool Value::isBoolean() {
        return this->type == BOOLEAN;
    }
    bool Value::isArray() {
        return this->type == ARRAY;
    }
    bool Value::isObject() {
        return this->type == OBJECT;
    }
    bool Value::isNull() {
        return this->type == UNDEFINED;
    }

    std::string Value::getString() {
    if (this->type == STRING) {
        return this->string;
        }
        throw TypeError(STRING, this->type);
    }

    long Value::getInteger() {
        if (this->type == INTEGER) {
            return this->integer;
        }
        throw TypeError(INTEGER, this->type);
    }

    float Value::getFloat() {
        if (this->type == FLOAT) {
            return this->floating;
        }
        throw TypeError(FLOAT, this->type);
    }

    bool Value::getBoolean() {
        if (this->type == BOOLEAN) {
            return this->boolean;
        }
        throw TypeError(BOOLEAN, this->type);
    }

    Array Value::getArray() {
        if (this->type == ARRAY) {
            return this->array;
        }
        throw TypeError(ARRAY, this->type);
    }
    Object Value::getObject() {
        if (this->type == OBJECT) {
            return this->object;
        }
        throw TypeError(OBJECT, this->type);
    }

    bool Value::operator==(const Value& other) {
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

    bool Value::operator!=(const Value& other) {
        return !(*this == other);
    }

    Value& Value::operator=(const Value& other) {
        if (*this != other) {
            // reset all values
            this->string   = "";
            this->integer  = 0;
            this->floating = 0.0;
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

    Value& Value::operator[](const std::string& key) {
        this->type = OBJECT;
        return this->object[key];
    }

    Value& Value::operator[](const long& index) {
        this->type = ARRAY;
        return this->array[index];
    }

    void Value::append(Value value) {
        this->type = ARRAY;
        this->array.push_back(value);
    }

    void Value::load(const std::string& str, bool trim_it) {
        // trim spaces, tabulators and newlines
        std::string dump = str;
        if (trim_it) {
            dump = trim(dump);
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
            // Parse Error
            throw ParseError(dump);
        }
    }

    std::string Value::dump(long indent) {
        switch (this->type) {
            case UNDEFINED:
                return "null";
                break;
            case STRING:
                return Value::dumpString();
                break;
            case INTEGER:
                return Value::dumpInteger();
                break;
            case FLOAT:
                return Value::dumpFloat();
                break;
            case BOOLEAN:
                return Value::dumpBoolean();
                break;
            case ARRAY:
                return Value::dumpArray(indent);
                break;
            case OBJECT:
                return Value::dumpObject(indent);
                break;
        }
        return "";
    }

}
