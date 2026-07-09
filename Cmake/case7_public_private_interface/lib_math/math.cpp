#include "math.h"
#include "core.h"

int mul(int a, int b) {
    int result = 0;
    for (int i = 0; i < b; i++)
        result = add(result, a);
    return result;
}

int div_(int a, int b) {
    int result = 0;
    int remaining = a;
    while (remaining >= b) {
        remaining = sub(remaining, b);
        result++;
    }
    return result;
}
