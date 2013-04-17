#include <iostream>
#include <map>

struct Data {
    int id, payload;
};

typedef void (*funktionszeiger)(Data);

void func1(Data data) {
    std::cout << "func 1: " << data.payload << std::endl;
}

void func2(Data data) {
    std::cout << "func 2: " << data.payload << std::endl;
}

int main() {
    // Funktionszeiger-Zuordnung
    std::map<int, funktionszeiger> mapper;
    mapper[1] = &func1;
    mapper[2] = &func2;

    // "Datenpakete"
    Data first, second;
    first.id = 1;
    first.payload = 13;
    second.id = 2;
    second.payload = -7;

    mapper[first.id](first);
    mapper[second.id](second);
}
