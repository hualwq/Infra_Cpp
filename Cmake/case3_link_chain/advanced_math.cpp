#include "advanced_math.h"
#include "core_math.h"

int advanced_add_twice(int a, int b) {
    // 内部使用了 core_math
    return core_add(core_add(a, b), core_add(a, b));
}
