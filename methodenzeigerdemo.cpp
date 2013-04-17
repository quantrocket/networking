#include <iostream>
#include <map>

typedef int CommandID;

class Base;
typedef void (Base::*CallbackPtr)(const std::string& data);

class Base {
    protected:
        std::map<CommandID, CallbackPtr> callbacks;
    public:
        void trigger(CommandID id, const std::string& data) {
            auto entry = this->callbacks.find(id);
            if (entry == this->callbacks.end()) {
                std::cout << "Undefined callback for #" << id << std::endl;
            } else {
                CallbackPtr callback = entry->second;
                (this->*callback)(data);
            }
        }
};

// ----------------------------------------------------------------------------

CommandID FOO = 1;
CommandID BAR = 2;

class Derived: public Base {
    private:
        float f;
    protected:
        void foo(const std::string& data) {
            std::cout << this->f << data << std::endl;
        }
        void bar(const std::string& data) {
            std::cout << "bar: " << data << std::endl;
        }
    public:
        Derived(): Base() {
            this->f = 1.3f;
            this->callbacks[FOO] = (CallbackPtr)(&Derived::foo);
            this->callbacks[BAR] = (CallbackPtr)(&Derived::bar);
        }
};

// ----------------------------------------------------------------------------

int main() {
    Derived d;
    d.trigger(FOO, "test");
    d.trigger(BAR, "1337");
}


