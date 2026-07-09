#include <cstdio>
#include "greet.h"

void greet(const char* name) {
    printf("Greetings, %s! (from shared library)\n", name);
}
