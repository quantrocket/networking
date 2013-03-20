#!/bin/python
# -*- coding: utf-8 -*-

"""
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
"""

"""
This file can be used to easily create event.hpp and event.cpp using an
event.cfg

An event description inside event.cfg can look like:

    Example (
        string[255] username
        ClientID client_id
        uint16 any_number
        float any_floatings
        bool success
        string[20000] long_message
    )

The result event declaration will be

    struct Example: Event {
        char username[255];
        ClientID client_id;
        uint16_t any_number;
        float any_floatings;
        bool success;
        char long_message[20000];

        Example(const std::string& username, ClientID client_id,
                uint16_t any_number, float any_floatings, bool success,
                const std::string& long_message);
    };

It also will create an EventID:

    const EventID E_EXAMPLE = 5;

The integer value is up to the number of given events and this event's
position is this list.

And there will be an implementation for die constructor and event-assembling
method:

    Example::Example(const std::string& username, ClientID client_id,
                     uint16_t any_number, float any_floatings, bool success,
                     const std::string& long_message)
        : Event(E_EXAMPLE) {
        strncpy(this->username, username.c_str(), 255);
        this->client_id = client_id;
        this->any_number = any_number;
        this->any_floatings = any_floatings;
        this->success = success;
        strncpy(this->long_message, long_message.c_str(), 20000);
    }

    Event* Event::assemble(void* buffer) {
        Event* event = reinterpret_cast<Event*>(buffer);
        EventID id = event->event_id;
        switch (id) {
            case E_EXAMPLE:
                event = new Example(*(Example*)buffer);
                break;
        };
        return event;
    }

There will be additional cases for all your other events.
"""

import sys

# convert TestCase => TEST_CASE
def upper(text):
    s = ""
    for char in text:
        if char.isupper():
            s += "_" + char
        else:
            s += char.upper()
    if s[0] == "_":
        s = s[1:]
    return s

if (len(sys.argv) < 4):
    print "Usage: python generate.py source.cfg result ../src"
    sys.exit(1)

cfg = sys.argv[1] # path for source configuration
res = sys.argv[2] # path for resulting header
fmw = sys.argv[3] # path for framework headers

# load config
handle = open(cfg, "r")
content = handle.read()
handle.close()

# parse config
data = list()
events = content.split(")");
for event in events:
    lines = event.split("\n")
    ident = ""
    members = list()
    for line in lines:
        if (len(line) == 0):
            continue
        if (len(ident) == 0):
            ident = line.split(" (")[0]
            #print ident
        else:
            tmp = line.lstrip()
            datatype = tmp.split(" ")[0]
            variable = tmp.split(" ")[1]
            # string[len]
            if (datatype[:6] == "string"):
                length = datatype[7:-1]
                datatype = ("char %s[{0}]".format(length), "const std::string& %s", length)
            # uintBITS / intBITS
            elif (datatype[:4] == "uint" or datatype[:3] == "int"):
                datatype = "{0}_t %s".format(datatype)
            # unsigned short / unsigned int
            elif (datatype == "unsigned short" or datatype == "unsigned int"):
                datatype = "uint16_t %s"
            # unsigned long
            elif (datatype == "unsigned lont"):
                datatype = "uint32_t %s"
            # char, bool, float, double
            elif (datatype in ["char", "bool", "float", "double"]):
                datatype = "{0} %s".format(datatype)
            else:
                print "Warning: datatype {0} assumed".format(datatype)
                datatype = "{0} %s".format(datatype)

            members.append((variable, datatype))

    if (len(ident) != 0):
        data.append((ident, members))

# create events
content = ""
content2 = ""
for tupel in data:
    key = tupel[0]
    value = tupel[1]
    content += "struct %s: public Event {\n" % key
    parameters = ""
    # create member variables
    for v in value:
        var = v[0]
        val = v[1]
        datatype = val
        if not isinstance(datatype, tuple):
            content += "    {0};\n".format(datatype % var)
            parameters += (datatype % var)
        else:
            content += "    {0};\n".format(datatype[0] % var)
            parameters += (datatype[1] % var)
        parameters += ", "
    # create default constructor
    if (len(parameters) != 0):
        parameters = parameters[:-2]
        content += "\n"
    content += "    {0}({1});\n".format(key, parameters)
    content2 += "{0}::{0}({1})\n".format(key, parameters)
    content2 += "    : Event(E_%s) {\n" % upper(key)
    for v in value:
        var = v[0]
        val = v[1]
        datatype = val
        if not isinstance(datatype, tuple):
            content2 += "    this->{0} = {0};\n".format(var)
        else:
            content2 += "    strncpy(this->{0}, {0}.c_str(), {1});\n".format(var, datatype[2])
    content2 += "}\n\n"
    content += "};\n\n"

# create event assembling
cases = ""
for value in data:
    key = value[0]
    val = value[1]
    cases += """
        case E_{0}:
            event = new {1}(*({1}*)buffer);
            break;""".format(upper(key), key)

content2 += """
Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) {%s
    };
    return event;
}
""" % cases

# add header, guard and include
header = """/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

"""
guard_open = """#ifndef {0}_res
#define {0}_res
""".format(res[:-4].upper())

guard_close = """
#endif
"""

include = """
#include \"{0}/eventsystem.hpp\"
#include \"{0}/serverclient.hpp\"

#include <cstdint>
#include <string.h>
#include <string>
""".format(fmw)

using = """
using networking::EventID;
using networking::Event;
using networking::ClientID;
"""

# constants
constants = "\n"
i = 1
for value in data:
    key = value[0]
    constants += "const EventID E_{0} = {1};\n".format(upper(key), i)
    i += 1
constants += "\n"

# generate header code and save to file
content = header + guard_open + include + using + constants + content + guard_close
handle = open(res + ".hpp", "w")
handle.write(content)
handle.close()

content2 = header + "#include \"{0}.hpp\"\n\n".format(res) + content2
handle = open(res + ".cpp", "w")
handle.write(content2)
handle.close()

print "=" * 20
print content
print "=" * 20
print content2
print "=" * 20


