/*
 * C++ 右值引用与移动语义详解
 *
 * 本文件涵盖：
 * 1. 左值 vs 右值
 * 2. 右值引用（&&）
 * 3. std::move 语义
 * 4. 移动构造函数和移动赋值运算符
 * 5. 完美转发（std::forward）
 * 6. 万能引用（forwarding reference）
 */

#include <iostream>
#include <string>
#include <utility>  // std::move, std::forward
#include <vector>

// ========================
// 1. 左值 vs 右值
// ========================
/*
 * 左值（lvalue）：
 *   - 有名字，可以取地址
 *   - 通常出现在赋值号的左边
 *   - 例子：变量名、返回左值引用的函数调用、前置++的结果
 *
 * 右值（rvalue）：
 *   - 没有名字，临时对象，不能取地址（或即将销毁）
 *   - 通常出现在赋值号的右边
 *   - 例子：字面量、临时对象、返回非引用类型的函数调用、后置++的结果
 *
 * 简单判断：能不能对表达式取地址（&）？能就是左值，不能就是右值。
 */

void demo_lvalue_rvalue() {
    std::cout << "\n=== 1. 左值 vs 右值 ===" << std::endl;

    int x = 10;           // x 是左值，10 是右值
    int y = x + 5;        // y 是左值，x + 5 是右值（临时结果）
    (void)y;                 // 避免 unused variable 警告

    std::string s = "hello";  // s 是左值，"hello" 是右值（临时字符串）

    // 取地址测试
    int* p1 = &x;         // ✅ x 是左值，可以取地址
    // int* p2 = &10;     // ❌ 10 是右值，不能取地址
    // int* p3 = &(x + 5); // ❌ x + 5 是右值，不能取地址

    std::cout << "x 是左值，可以取地址: " << p1 << std::endl;
}

// ========================
// 2. 右值引用（&&）
// ========================
/*
 * 右值引用：
 *   - 语法：T&&
 *   - 只能绑定到右值（临时对象）
 *   - 作用：延长临时对象的生命周期，允许"窃取"其资源
 *
 * 左值引用 vs 右值引用：
 *   - T&  只能绑定左值
 *   - T&& 只能绑定右值（除非是万能引用，见后面）
 */

void demo_rvalue_reference() {
    std::cout << "\n=== 2. 右值引用（&&）===" << std::endl;

    int a = 10;
    int& lref = a;        // ✅ 左值引用绑定左值
    (void)lref;              // 避免 unused variable 警告
    // int& lref2 = 10;   // ❌ 左值引用不能绑定右值

    int&& rref = 10;      // ✅ 右值引用绑定右值
    // int&& rref2 = a;   // ❌ 右值引用不能绑定左值（除非用 std::move）
    int&& rref3 = std::move(a);  // ✅ std::move 把左值转为右值
    (void)rref3;                     // 避免 unused variable 警告

    rref = 20;            // 可以修改右值引用绑定的对象
    std::cout << "rref = " << rref << std::endl;  // 20
    std::cout << "a = " << a << std::endl;        // 20（a 被 move 后状态不确定，但这里是可修改的）

    // 右值引用延长临时对象的生命周期
    std::string&& str_ref = std::string("temporary");  // 临时对象的生命周期延长到 str_ref 的作用域结束
    std::cout << "str_ref = " << str_ref << std::endl;
}

// ========================
// 3. std::move 语义
// ========================
/*
 * std::move：
 *   - 不是"移动"，而是"把左值强制转换为右值引用"
 *   - 它的作用是告诉编译器："我不再需要这个对象了，你可以把它的资源偷走"
 *   - 被 move 后的对象处于"有效但未指定状态"，不能再依赖它的值
 *
 * 注意：std::move 本身不做任何移动，真正的移动发生在移动构造函数/移动赋值中
 */

void demo_std_move() {
    std::cout << "\n=== 3. std::move 语义 ===" << std::endl;

    std::string s1 = "hello world";
    std::cout << "s1 before move: " << s1 << std::endl;

    std::string s2 = std::move(s1);  // s1 转为右值，触发移动构造
    std::cout << "s1 after move: \"" << s1 << "\"" << std::endl;  // s1 状态不确定，通常是空字符串
    std::cout << "s2 after move: " << s2 << std::endl;

    // 重要：被 move 的对象仍然有效（可以赋值、销毁），但值不确定
    s1 = "reset";  // ✅ 可以对 move 后的对象重新赋值
    std::cout << "s1 after reset: " << s1 << std::endl;
}

// ========================
// 4. 移动构造函数和移动赋值运算符
// ========================
/*
 * 移动构造：用右值初始化新对象，窃取资源而不是拷贝
 * 移动赋值：用右值赋值给已有对象，窃取资源而不是拷贝
 *
 * 签名：
 *   移动构造：ClassName(ClassName&& other) noexcept;
 *   移动赋值：ClassName& operator=(ClassName&& other) noexcept;
 *
 * 三/五法则扩展为：如果你需要自定义析构、拷贝、赋值中的任何一个，
 * 考虑是否需要移动构造和移动赋值。
 */

class Buffer {
private:
    size_t size;  // 先声明 size，和初始化列表顺序一致
    int* data;

public:
    // 普通构造
    Buffer(size_t s) : size(s), data(new int[s]) {
        std::cout << "Buffer(size_t) 构造，分配 " << s << " 个 int" << std::endl;
    }

    // 析构
    ~Buffer() {
        std::cout << "~Buffer() 析构，释放 " << size << " 个 int" << std::endl;
        delete[] data;
    }

    // 拷贝构造（深拷贝）
    Buffer(const Buffer& other) : size(other.size), data(new int[other.size]) {
        std::copy(other.data, other.data + other.size, data);
        std::cout << "Buffer(const Buffer&) 拷贝构造" << std::endl;
    }

    // 拷贝赋值
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = new int[other.size];
            std::copy(other.data, other.data + other.size, data);
            std::cout << "operator=(const Buffer&) 拷贝赋值" << std::endl;
        }
        return *this;
    }

    // 移动构造（noexcept 很重要，影响 vector 等容器的行为）
    Buffer(Buffer&& other) noexcept
        : size(other.size), data(other.data) {  // 按声明顺序初始化
        // 窃取资源：直接拿 other 的指针
        other.data = nullptr;  // 重要：让 other 不再拥有这块内存
        other.size = 0;
        std::cout << "Buffer(Buffer&&) 移动构造" << std::endl;
    }

    // 移动赋值
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data;         // 释放自己原来的资源
            data = other.data;     // 窃取 other 的资源
            size = other.size;
            other.data = nullptr;  // other 不再拥有资源
            other.size = 0;
            std::cout << "operator=(Buffer&&) 移动赋值" << std::endl;
        }
        return *this;
    }

    size_t getSize() const { return size; }
};

void demo_move_constructor() {
    std::cout << "\n=== 4. 移动构造函数和移动赋值 ===" << std::endl;

    Buffer b1(100);  // 普通构造

    std::cout << "\n--- 触发移动构造 ---" << std::endl;
    Buffer b2 = std::move(b1);  // 移动构造（b1 转为右值）

    std::cout << "\nb1.size = " << b1.getSize() << " (被移动后通常为 0)" << std::endl;
    std::cout << "b2.size = " << b2.getSize() << std::endl;

    std::cout << "\n--- 触发移动赋值 ---" << std::endl;
    Buffer b3(50);
    b3 = std::move(b2);  // 移动赋值

    std::cout << "\n--- vector 扩容时的移动 ---" << std::endl;
    std::vector<Buffer> vec;
    vec.reserve(2);  // 预留空间，避免多次扩容
    vec.push_back(Buffer(10));  // 传递临时对象，触发移动构造
    vec.push_back(Buffer(20));  // 传递临时对象，触发移动构造
}

// ========================
// 5. 完美转发（std::forward）
// ========================
/*
 * 问题：如何写一个函数，把参数原封不动地转发给另一个函数？
 *       "原封不动" 指：左值还是左值，右值还是右值。
 *
 * 直接转发的问题：
 *   参数在函数中是左值（有名字），转发时丢失了右值属性。
 *
 * 完美转发：
 *   使用万能引用（T&&） + std::forward<T>()
 *   std::forward 有条件地转换为右值：如果原始参数是右值，就转为右值；否则保持左值。
 */

// 被调用的目标函数（重载版本）
void process(int& x) {
    std::cout << "process(int&): 左值引用，x = " << x << std::endl;
}

void process(int&& x) {
    std::cout << "process(int&&): 右值引用，x = " << x << std::endl;
}

void process(const std::string& s) {
    std::cout << "process(const string&): 左值 string，s = " << s << std::endl;
}

void process(std::string&& s) {
    std::cout << "process(string&&): 右值 string，s = " << s << std::endl;
}

// 转发函数：使用万能引用和 std::forward
template<typename T>
void forwarder(T&& arg) {
    std::cout << "forwarder 收到参数，转发给 process..." << std::endl;
    process(std::forward<T>(arg));  // 完美转发：保持 arg 的原始值类别
}

void demo_perfect_forwarding() {
    std::cout << "\n=== 5. 完美转发（std::forward）===" << std::endl;

    int x = 10;
    const std::string s = "hello";

    std::cout << "\n--- 直接调用 ---" << std::endl;
    process(x);           // 调用 process(int&)
    process(20);          // 调用 process(int&&)
    process(s);           // 调用 process(const string&)
    process(std::string("tmp"));  // 调用 process(string&&)

    std::cout << "\n--- 通过 forwarder 转发 ---" << std::endl;
    forwarder(x);         // T = int&，forward 后还是左值 → process(int&)
    forwarder(20);        // T = int，forward 后变为右值 → process(int&&)
    forwarder(s);         // T = const string&，forward 后还是左值 → process(const string&)
    forwarder(std::string("tmp"));  // T = string，forward 后变为右值 → process(string&&)
}

// ========================
// 6. 万能引用（Forwarding Reference / Universal Reference）
// ========================
/*
 * 万能引用：
 *   - 看起来像 T&&，但 T 是模板参数，且需要类型推导
 *   - 可以绑定左值，也可以绑定右值
 *   - 当传入左值时，T 推导为左值引用；传入右值时，T 推导为非引用类型
 *
 * 注意区分：
 *   - T&&（模板参数，需要推导）→ 万能引用
 *   - Foo&&（具体类型）→ 右值引用
 */

// 万能引用：T 需要推导
template<typename T>
void universal_ref(T&& param) {
    (void)param;
    std::cout << "universal_ref 被调用" << std::endl;
}

// 右值引用：Foo 是具体类型，不需要推导
void rvalue_ref(std::string&& param) {
    (void)param;
    std::cout << "rvalue_ref 被调用" << std::endl;
}

void demo_universal_reference() {
    std::cout << "\n=== 6. 万能引用 ===" << std::endl;

    std::string s = "test";

    std::cout << "\n--- 万能引用 ---" << std::endl;
    universal_ref(s);     // T = string&，param 是左值引用
    universal_ref(std::move(s));  // T = string，param 是右值引用

    std::cout << "\n--- 右值引用 ---" << std::endl;
    // rvalue_ref(s);           // ❌ 不能绑定左值
    rvalue_ref(std::move(s));    // ✅ 只能绑定右值
    rvalue_ref("hello");         // ✅ 字面量是右值
}

// ========================
// 总结
// ========================
/*
 * 左值 vs 右值：
 *   - 左值：有名字，可取地址
 *   - 右值：临时对象，不可取地址
 *
 * 右值引用（T&&）：
 *   - 只能绑定右值
 *   - 用于实现移动语义
 *
 * std::move：
 *   - 把左值转为右值引用（告诉编译器可以偷资源）
 *   - 本身不做移动
 *
 * 移动构造/移动赋值：
 *   - 窃取资源而不是拷贝
 *   - 被移动的对象置为有效但未指定状态
 *
 * 完美转发：
 *   - 模板参数 T&&（万能引用）+ std::forward<T>()
 *   - 保持参数的原始值类别（左值/右值）
 *
 * 万能引用 vs 右值引用：
 *   - T&&（模板推导）→ 万能引用，可绑定左值或右值
 *   - Foo&&（具体类型）→ 右值引用，只能绑定右值
 */

int main() {
    demo_lvalue_rvalue();
    demo_rvalue_reference();
    demo_std_move();
    demo_move_constructor();
    demo_perfect_forwarding();
    demo_universal_reference();
    return 0;
}
