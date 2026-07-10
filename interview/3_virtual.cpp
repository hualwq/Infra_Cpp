/*
 * C++ 虚函数机制详解
 *
 * 本文件涵盖：
 * 1. 虚函数机制（运行时多态）
 * 2. 虚表（vtable）和虚表指针（vptr）
 * 3. 纯虚函数和抽象类
 * 4. 重载（overload）和重写（override）
 */

#include <iostream>
#include <typeinfo>

// ========================
// 1. 虚函数机制（运行时多态）
// ========================
/*
 * 虚函数允许在派生类中重新定义基类的方法，并通过基类指针/引用调用正确的版本。
 * 关键：调用哪个函数在运行时决定（动态绑定），而不是编译时（静态绑定）。
 *
 * 没有虚函数：编译期绑定，调用基类版本
 * 有虚函数：运行期绑定，调用实际对象版本
 */

class Animal {
public:
    // 非虚函数：静态绑定
    void eat() {
        std::cout << "Animal::eat() - 动物吃东西" << std::endl;
    }

    // 虚函数：动态绑定
    virtual void speak() {
        std::cout << "Animal::speak() - 动物叫" << std::endl;
    }

    // 虚析构函数：非常重要！
    // 如果基类指针指向派生类对象，delete 时需要通过虚函数机制调用正确的析构函数
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    // 重写（override）基类的 speak
    void speak() override {
        std::cout << "Dog::speak() - 汪汪汪" << std::endl;
    }

    // 也可以重写非虚函数，但不会动态绑定
    void eat() {
        std::cout << "Dog::eat() - 狗吃狗粮" << std::endl;
    }
};

class Cat : public Animal {
public:
    void speak() override {
        std::cout << "Cat::speak() - 喵喵喵" << std::endl;
    }
};

void demo_virtual_mechanism() {
    std::cout << "\n=== 1. 虚函数机制 ===" << std::endl;

    // 静态绑定：编译期就知道调用哪个
    Animal a;
    a.speak();  // Animal::speak()
    a.eat();    // Animal::eat()

    Dog d;
    d.speak();  // Dog::speak()
    d.eat();    // Dog::eat()

    // 动态绑定：运行期才知道
    Animal* ptr = &d;
    ptr->speak();  // 输出 Dog::speak() - 虚函数，运行时查找
    ptr->eat();    // 输出 Animal::eat() - 非虚函数，编译期绑定

    // 多态的体现
    Animal* animals[] = { new Dog(), new Cat() };
    for (auto animal : animals) {
        animal->speak();  // 运行时根据实际对象类型调用
    }
    for (auto animal : animals) {
        delete animal;  // 需要虚析构函数
    }
}

// ========================
// 2. 虚表（vtable）和虚表指针（vptr）
// ========================
/*
 * 虚函数机制的实现原理：
 *
 * 虚表（vtable）：
 *   - 每个含有虚函数的类，编译器为其生成一个虚函数表
 *   - 虚表是一个函数指针数组，存放该类所有虚函数的地址
 *   - 如果派生类重写了虚函数，虚表中对应位置存放派生类的函数地址
 *   - 如果派生类没有重写，虚表中存放基类的函数地址
 *
 * 虚表指针（vptr）：
 *   - 每个含有虚函数的类的对象，都包含一个指向该类虚表的指针
 *   - 通常在对象内存布局的最前面（编译器实现相关）
 *   - 对象构造时，编译器自动设置 vptr 指向正确的虚表
 *
 * 内存布局示意：
 *
 * class Animal:
 *   vtable: [ &Animal::speak, &Animal::~Animal ]
 *
 * class Dog:
 *   vtable: [ &Dog::speak, &Dog::~Dog ]
 *
 * Animal obj1:  [ vptr ] → Animal 的 vtable
 * Dog obj2:     [ vptr ] → Dog 的 vtable
 *
 * 调用 ptr->speak() 时：
 *   1. 通过 ptr 找到对象的 vptr
 *   2. 通过 vptr 找到 vtable
 *   3. 在 vtable 中找到 speak 的索引位置
 *   4. 调用该位置的函数指针
 */

class Base {
public:
    virtual void func1() { std::cout << "Base::func1" << std::endl; }
    virtual void func2() { std::cout << "Base::func2" << std::endl; }
    void nonVirtual() { std::cout << "Base::nonVirtual" << std::endl; }
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void func1() override { std::cout << "Derived::func1" << std::endl; }
    // func2 没有重写，继承 Base::func2
    virtual void func3() { std::cout << "Derived::func3" << std::endl; }
    ~Derived() override { std::cout << "Derived::~Derived" << std::endl; }
};

void demo_vtable_vptr() {
    std::cout << "\n=== 2. 虚表（vtable）和虚表指针（vptr）===" << std::endl;

    Base b;
    Derived d;

    std::cout << "Base 对象大小: " << sizeof(b) << " 字节" << std::endl;
    std::cout << "Derived 对象大小: " << sizeof(d) << " 字节" << std::endl;
    // 大小 = 数据成员 + vptr（通常 8 字节在 64 位系统）
    // 如果没有虚函数，大小可能更小

    Base* p = &d;
    p->func1();  // 通过 vtable 找到 Derived::func1
    p->func2();  // 通过 vtable 找到 Base::func2（继承的）
    // p->func3();  // 编译错误：Base 没有 func3

    // 演示虚析构函数的重要性
    Base* basePtr = new Derived();
    delete basePtr;  // 如果 ~Base 不是 virtual，只会调用 ~Base()，Derived 部分泄漏
}

// ========================
// 3. 纯虚函数和抽象类
// ========================
/*
 * 纯虚函数：
 *   - 声明时赋值为 0：virtual void func() = 0;
 *   - 没有函数体，派生类必须重写，否则派生类也是抽象类
 *
 * 抽象类：
 *   - 含有至少一个纯虚函数的类
 *   - 不能实例化（不能创建对象）
 *   - 可以作为指针或引用类型
 *   - 通常用于定义接口
 *
 * 接口类：
 *   - 所有函数都是纯虚函数，没有数据成员
 *   - 类似 Java 的 interface
 */

class Shape {
public:
    // 纯虚函数：计算面积
    virtual double area() const = 0;

    // 纯虚函数：绘制
    virtual void draw() const = 0;

    // 普通虚函数：可以有默认实现
    virtual void printInfo() const {
        std::cout << "这是一个图形" << std::endl;
    }

    // 虚析构函数：抽象类的析构函数通常也应该是 virtual
    virtual ~Shape() {}
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(double r) : radius(r) {}

    // 必须重写纯虚函数
    double area() const override {
        return 3.14159 * radius * radius;
    }

    void draw() const override {
        std::cout << "绘制圆形，半径: " << radius << std::endl;
    }
};

class Rectangle : public Shape {
private:
    double width, height;

public:
    Rectangle(double w, double h) : width(w), height(h) {}

    double area() const override {
        return width * height;
    }

    void draw() const override {
        std::cout << "绘制矩形，宽: " << width << " 高: " << height << std::endl;
    }

    void printInfo() const override {
        std::cout << "这是一个矩形" << std::endl;
    }
};

void demo_pure_virtual() {
    std::cout << "\n=== 3. 纯虚函数和抽象类 ===" << std::endl;

    // Shape s;  // 编译错误：不能实例化抽象类

    Shape* shapes[] = {
        new Circle(5.0),
        new Rectangle(3.0, 4.0)
    };

    for (auto shape : shapes) {
        shape->draw();
        std::cout << "面积: " << shape->area() << std::endl;
        shape->printInfo();
        std::cout << std::endl;
    }

    for (auto shape : shapes) {
        delete shape;
    }
}

// ========================
// 4. 重载（overload）和重写（override）
// ========================
/*
 * 重载（overload）：
 *   - 同一作用域内，函数名相同，参数列表不同（类型、数量、顺序）
 *   - 返回类型可以不同
 *   - 编译期决定调用哪个（静态绑定）
 *   - 与虚函数无关
 *
 * 重写/覆盖（override）：
 *   - 派生类重新定义基类中同名同参数的虚函数
 *   - 函数名、参数列表、返回类型必须相同（协变返回类型除外）
 *   - 访问修饰符可以不同
 *   - 运行期决定调用哪个（动态绑定）
 *   - 基类中函数必须是 virtual
 *
 * 隐藏（hide）：
 *   - 派生类的函数与基类函数同名，但不是重写（参数不同，或基类不是虚函数）
 *   - 派生类函数会隐藏基类同名函数
 */

class OverloadDemo {
public:
    // 重载示例：同一类中的同名函数
    void func() {
        std::cout << "func()" << std::endl;
    }

    void func(int x) {
        std::cout << "func(int): " << x << std::endl;
    }

    void func(double x) {
        std::cout << "func(double): " << x << std::endl;
    }

    void func(int x, double y) {
        std::cout << "func(int, double): " << x << ", " << y << std::endl;
    }

    // 返回类型不同也可以重载
    int func(double x, double y) {
        std::cout << "func(double, double): " << x << ", " << y << std::endl;
        return 0;
    }
};

class BaseForOverride {
public:
    virtual void foo() {
        std::cout << "BaseForOverride::foo()" << std::endl;
    }

    virtual void bar(int x) {
        std::cout << "BaseForOverride::bar(int): " << x << std::endl;
    }

    void nonVirtual() {
        std::cout << "BaseForOverride::nonVirtual()" << std::endl;
    }

    virtual ~BaseForOverride() {}
};

class DerivedForOverride : public BaseForOverride {
public:
    // 重写（override）：参数相同，基类是 virtual
    void foo() override {
        std::cout << "DerivedForOverride::foo()" << std::endl;
    }

    // 这不是重写！参数不同，是隐藏（hide）
    // 不会动态绑定
    void bar(double x) {
        std::cout << "DerivedForOverride::bar(double): " << x << std::endl;
    }

    // 隐藏基类的 nonVirtual
    void nonVirtual() {
        std::cout << "DerivedForOverride::nonVirtual()" << std::endl;
    }
};

void demo_overload_override() {
    std::cout << "\n=== 4. 重载（overload）和重写（override）===" << std::endl;

    // 重载示例
    OverloadDemo od;
    od.func();
    od.func(10);
    od.func(3.14);
    od.func(10, 3.14);

    std::cout << std::endl;

    // 重写示例
    BaseForOverride* p = new DerivedForOverride();
    p->foo();        // 输出 DerivedForOverride::foo() - 动态绑定
    p->bar(10);      // 输出 BaseForOverride::bar(int) - 参数匹配基类版本
    // p->bar(3.14); // 调用 bar(int)，3.14 被截断

    // 隐藏示例
    DerivedForOverride d;
    d.bar(10);       // 输出 DerivedForOverride::bar(double) - int 被提升为 double
    d.bar(3.14);     // 输出 DerivedForOverride::bar(double)
    d.BaseForOverride::bar(10);  // 显式调用基类版本

    d.nonVirtual();  // 输出 DerivedForOverride::nonVirtual()
    p->nonVirtual(); // 输出 BaseForOverride::nonVirtual() - 非虚函数，静态绑定

    delete p;
}

// ========================
// 总结
// ========================
/*
 * 虚函数机制：
 *   - 运行时多态的基础
 *   - 通过虚表和虚表指针实现
 *   - 只能用于指针和引用
 *
 * 虚表（vtable）：
 *   - 每个类一个，存放虚函数地址
 *   - 编译期生成
 *
 * 虚表指针（vptr）：
 *   - 每个对象一个，指向类的虚表
 *   - 对象构造时设置
 *
 * 纯虚函数和抽象类：
 *   - 纯虚函数 = 0，必须重写
 *   - 抽象类不能实例化
 *   - 用于定义接口
 *
 * 重载 vs 重写：
 *   - 重载：同一作用域，参数不同，编译期
 *   - 重写：父子类，参数相同，基类 virtual，运行期
 */

int main() {
    demo_virtual_mechanism();
    demo_vtable_vptr();
    demo_pure_virtual();
    demo_overload_override();
    return 0;
}
