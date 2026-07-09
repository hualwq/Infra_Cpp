#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    // ========== 基础语法 ==========
    // [捕获列表](参数列表) -> 返回类型 { 函数体 }

    // 1. 最简单的 lambda：无参数，无捕获
    auto hello = []() {
        std::cout << "Hello lambda!" << std::endl;
    };
    hello();

    // 2. 带参数的 lambda
    auto add = [](int a, int b) -> int {
        return a + b;
    };
    std::cout << "add(3, 5) = " << add(3, 5) << std::endl;

    // 3. 省略返回类型（自动推导）
    auto mul = [](int a, int b) { return a * b; };
    std::cout << "mul(4, 7) = " << mul(4, 7) << std::endl;

    // ========== 捕获外部变量 ==========
    int x = 10, y = 20;

    // 值捕获 [=]
    auto by_value = [=]() {
        std::cout << "by value: x=" << x << ", y=" << y << std::endl;
    };
    by_value();

    // 引用捕获 [&]
    auto by_ref = [&]() {
        x = 100;
        y = 200;
    };
    by_ref();
    std::cout << "after by_ref: x=" << x << ", y=" << y << std::endl;

    // 混合捕获
    int z = 30;
    auto mixed = [x, &y]() {
        // x 是值捕获（只读），y 是引用捕获（可修改）
        // x = 999; // 编译错误！值捕获默认只读
        y = 999;
        std::cout << "mixed: x=" << x << ", y=" << y << std::endl;
    };
    mixed();
    std::cout << "after mixed: y=" << y << std::endl;

    // mutable：允许修改值捕获的副本
    int count = 0;
    auto counter = [count]() mutable {
        count++;
        std::cout << "count inside = " << count << std::endl;
    };
    counter();
    counter();
    std::cout << "count outside = " << count << std::endl; // 外部的 count 不变

    // ========== 在标准库算法中使用 ==========
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8};

    // 用 lambda 做条件过滤
    auto even = std::count_if(nums.begin(), nums.end(),
                              [](int n) { return n % 2 == 0; });
    std::cout << "even count = " << even << std::endl;

    // 用 lambda 做遍历
    std::cout << "nums: ";
    std::for_each(nums.begin(), nums.end(),
                  [](int n) { std::cout << n << " "; });
    std::cout << std::endl;

    // 用 lambda 做变换
    std::vector<int> squared(nums.size());
    std::transform(nums.begin(), nums.end(), squared.begin(),
                   [](int n) { return n * n; });
    std::cout << "squared: ";
    for (int s : squared) std::cout << s << " ";
    std::cout << std::endl;

    // 捕获外部容器的 lambda（引用捕获避免拷贝）
    int threshold = 5;
    auto greater_than = std::count_if(nums.begin(), nums.end(),
                                      [&threshold](int n) { return n > threshold; });
    std::cout << "numbers > " << threshold << " count = " << greater_than << std::endl;

    // ========== Lambda 赋值给 std::function ==========
    // std::function<返回类型(参数类型...)> 可以存储任何可调用对象
    // lambda 本质上是一个匿名仿函数，std::function 通过类型擦除来容纳它

    // 例1：无捕获的 lambda → std::function（可退化为函数指针，开销最小）
    std::function<int(int, int)> f1 = [](int a, int b) { return a + b; };
    std::cout << "f1(10, 20) = " << f1(10, 20) << std::endl;

    // 可以重新绑定不同的 lambda（只要签名匹配）
    f1 = [](int a, int b) { return a * b; };
    std::cout << "f1(10, 20) = " << f1(10, 20) << std::endl;

    // 例2：有捕获的 lambda → std::function（携带状态）
    int factor = 3;
    std::function<int(int)> f2 = [factor](int n) { return n * factor; };
    std::cout << "f2(7) = " << f2(7) << std::endl;

    // f2 捕获了 factor，但 std::function 依然能统一存储
    // 可以存在容器里做策略模式
    std::vector<std::function<int(int)>> ops;
    int offset = 100;
    ops.push_back([](int n) { return n + 1; });
    ops.push_back([](int n) { return n * 2; });
    ops.push_back([offset](int n) { return n + offset; });
    for (auto &op : ops)
        std::cout << "op(5) = " << op(5) << std::endl;

    return 0;
}
