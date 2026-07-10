/*
 * C++ 四种类型转换详解
 *
 * C++ 提供了四种显式的类型转换操作符，比 C 语言的 (type) 转换更安全、更明确：
 * 1. static_cast   - 静态转换（编译期检查）
 * 2. dynamic_cast  - 动态转换（运行期检查，需要虚函数）
 * 3. const_cast    - 常量转换（去除/添加 const）
 * 4. reinterpret_cast - 重解释转换（低层重新解释）
 */

#include <iostream>
#include <typeinfo>

// ========================
// 用于演示的类
// ========================
class Base {
public:
    virtual ~Base() {}  // 必须有虚函数，dynamic_cast 才能工作
    virtual void print() { std::cout << "Base::print()" << std::endl; }
};

class Derived : public Base {
public:
    void print() override { std::cout << "Derived::print()" << std::endl; }
    void derivedOnly() { std::cout << "Derived::derivedOnly()" << std::endl; }
};

// ========================
// 1. static_cast（静态转换）
// ========================
/*
 * 用途：
 *   - 基本类型之间的转换（如 int -> double）
 *   - 父类和子类指针/引用之间的转换
 *   - void* 和其他指针类型之间的转换
 *
 * 特点：
 *   - 编译期检查，无运行期开销
 *   - 上行转换（子类 -> 父类）安全
 *   - 下行转换（父类 -> 子类）不安全！不会做运行期检查
 */
void demo_static_cast() {
    std::cout << "\n=== 1. static_cast ===" << std::endl;

    // 1.1 基本类型转换
    int i = 42;
    double d = static_cast<double>(i);  // int -> double
    std::cout << "int->double: " << d << std::endl;

    // 1.2 上行转换（子类 -> 父类），安全
    Derived derived;
    Base* basePtr = static_cast<Base*>(&derived);  // 子类指针 -> 父类指针
    basePtr->print();  // 输出: Derived::print()（多态）

    // 1.3 下行转换（父类 -> 子类），不安全！
    // static_cast 不会检查父类指针是否真的指向子类对象
    Base base;
    Derived* derivedPtr = static_cast<Derived*>(&base);  // 危险！但实际指向的是 Base
    // derivedPtr->derivedOnly();  // 未定义行为！不要这样做

    // 1.4 void* 转换
    int value = 100;
    void* voidPtr = static_cast<void*>(&value);
    int* intPtr = static_cast<int*>(voidPtr);  // void* -> int*
    std::cout << "void*->int*: " << *intPtr << std::endl;
}

// ========================
// 2. dynamic_cast（动态转换）
// ========================
/*
 * 用途：
 *   - 安全的下行转换（父类指针/引用 -> 子类指针/引用）
 *   - 交叉转换（多重继承中）
 *
 * 特点：
 *   - 运行期检查（通过 RTTI，运行时类型识别）
 *   - 只能用于含有虚函数的类（多态类型）
 *   - 转换失败：指针返回 nullptr，引用抛出 std::bad_cast
 *   - 有运行期开销
 */
void demo_dynamic_cast() {
    std::cout << "\n=== 2. dynamic_cast ===" << std::endl;

    // 2.1 安全的下行转换
    Base* base1 = new Derived();  // 父类指针实际指向子类对象
    Derived* derived1 = dynamic_cast<Derived*>(base1);
    if (derived1) {
        std::cout << "转换成功: ";
        derived1->derivedOnly();  // 安全调用
    } else {
        std::cout << "转换失败" << std::endl;
    }

    // 2.2 转换失败的情况
    Base* base2 = new Base();  // 父类指针实际指向父类对象
    Derived* derived2 = dynamic_cast<Derived*>(base2);
    if (derived2) {
        std::cout << "转换成功" << std::endl;
    } else {
        std::cout << "转换失败: 返回 nullptr" << std::endl;
    }

    // 2.3 引用版本的 dynamic_cast
    Derived derived;
    Base& baseRef = derived;
    try {
        Derived& derivedRef = dynamic_cast<Derived&>(baseRef);
        std::cout << "引用转换成功" << std::endl;
    } catch (const std::bad_cast& e) {
        std::cout << "引用转换失败: " << e.what() << std::endl;
    }

    delete base1;
    delete base2;
}

// ========================
// 3. const_cast（常量转换）
// ========================
/*
 * 用途：
 *   - 去除或添加 const/volatile 属性
 *   - 是唯一能去除 const 的转换
 *
 * 特点：
 *   - 只能改变 const/volatile 属性，不能改变类型
 *   - 去除 const 后修改原对象，如果原对象本身是 const，是未定义行为
 */
void demo_const_cast() {
    std::cout << "\n=== 3. const_cast ===" << std::endl;

    // 3.1 去除 const（危险！需要确认原对象非 const）
    int value = 10;        // 非 const 对象
    const int* constPtr = &value;
    std::cout << "修改前: " << value << std::endl;

    int* mutablePtr = const_cast<int*>(constPtr);  // 去除 const
    *mutablePtr = 20;  // 安全，因为原对象 value 本身不是 const
    std::cout << "修改后: " << value << std::endl;

    // 3.2 危险示例：修改真正的 const 对象
    const int trueConst = 30;
    // int* p = const_cast<int*>(&trueConst);
    // *p = 40;  // 未定义行为！编译器可能优化掉，或者崩溃

    // 3.3 添加 const
    int another = 50;
    int* mutPtr = &another;
    const int* constAgain = const_cast<const int*>(mutPtr);
    std::cout << "添加 const: " << *constAgain << std::endl;

    // 3.4 常见用途：调用遗留 API
    // 有些老函数参数是非 const，但你只有 const 对象
    auto oldStyleFunc = [](int* p) { if (p) *p = 100; };
    const int* constData = &another;
    // oldStyleFunc(constData);  // 编译错误
    oldStyleFunc(const_cast<int*>(constData));  // 可以调用（需确保函数不会真的修改）
}

// ========================
// 4. reinterpret_cast（重解释转换）
// ========================
/*
 * 用途：
 *   - 任意指针类型之间的转换
 *   - 指针和整数之间的转换
 *   - 函数指针之间的转换
 *
 * 特点：
 *   - 最危险的转换，几乎不做任何检查
 *   - 只是重新解释比特位
 *   - 结果依赖于平台和编译器，不可移植
 *   - 谨慎使用！
 */
void demo_reinterpret_cast() {
    std::cout << "\n=== 4. reinterpret_cast ===" << std::endl;

    // 4.1 指针类型之间的转换
    int intValue = 0x12345678;
    int* intPtr = &intValue;
    char* charPtr = reinterpret_cast<char*>(intPtr);  // int* -> char*
    std::cout << "int* -> char*: " << std::hex << static_cast<int>(*charPtr) << std::endl;

    // 4.2 指针和整数之间的转换
    uintptr_t addr = reinterpret_cast<uintptr_t>(intPtr);  // 指针 -> 整数
    std::cout << "指针值作为整数: 0x" << std::hex << addr << std::dec << std::endl;

    int* restored = reinterpret_cast<int*>(addr);  // 整数 -> 指针
    std::cout << "恢复指针: " << *restored << std::endl;

    // 4.3 函数指针转换（危险！）
    using FuncPtr = void (*)();
    // void* funcPtr = reinterpret_cast<void*>(&demo_static_cast);  // 不推荐
}

// ========================
// 对比总结
// ========================
/*
 * ┌─────────────────┬──────────────────────────────────────────────┐
 * │ 转换方式         │ 适用场景                                      │
 * ├─────────────────┼──────────────────────────────────────────────┤
 * │ static_cast     │ 基本类型、确定的父子类转换、void* 转换          │
 * │ dynamic_cast    │ 不安全的下行转换（需要运行期检查）                │
 * │ const_cast      │ 去除/添加 const（唯一能这样做的方式）            │
 * │ reinterpret_cast│ 底层重新解释（指针↔整数，任意指针互转）          │
 * └─────────────────┴──────────────────────────────────────────────┘
 *
 * 为什么不用 C 风格的 (int)value 转换？
 *   - C++ 的转换更明确，一看就知道想做什么
 *   - C++ 的转换更安全，编译器能做更多检查
 *   - 方便 grep 查找转换代码
 */

int main() {
    demo_static_cast();
    demo_dynamic_cast();
    demo_const_cast();
    demo_reinterpret_cast();
    return 0;
}
