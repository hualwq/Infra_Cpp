#include "printable.h"
#include <string>

int main() {
    print_value(42);
    print_value(3.14);
    print_value(std::string("hello interface"));
    return 0;
}
