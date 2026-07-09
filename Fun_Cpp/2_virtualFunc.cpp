#include <iostream>
#include <vector>
#include <memory>

// ========== 前向声明 ==========
class IRVisitor;

// ========== 1. 虚函数与纯虚函数 ==========
// 基类定义接口，派生类提供具体实现

class Expr {
public:
    // virtual: 允许子类重写此方法，实现动态绑定
    virtual std::string type_name() const {
        return "Expr";
    }

    // 纯虚函数 = 0：当前类变为抽象类，不能实例化
    // 强制所有派生类必须实现此方法
    virtual int IsInstance() const = 0;

    // Visitor 模式：每个子类通过 Accept 调用 visitor 的 Visit 重载
    // 这是实现"双重分发"(double dispatch) 的关键
    virtual void Accept(IRVisitor *v) = 0;

    // 虚析构函数：确保通过基类指针删除时能正确调用派生类析构函数
    virtual ~Expr() {
        std::cout << "~Expr()" << std::endl;
    }
};

// 派生类 1
class IntImm : public Expr {
    int value_;
public:
    IntImm(int v) : value_(v) {}

    // override（C++11）：显式声明重写，编译器会检查是否真的覆盖了基类虚函数
    std::string type_name() const override {
        return "IntImm";
    }

    int IsInstance() const override {
        return value_;
    }

    int value() const { return value_; }

    void Accept(IRVisitor *v) override;

    ~IntImm() override {
        std::cout << "~IntImm(" << value_ << ")" << std::endl;
    }
};

// 派生类 2
class FloatImm : public Expr {
    float value_;
public:
    FloatImm(float v) : value_(v) {}

    std::string type_name() const override {
        return "FloatImm";
    }

    int IsInstance() const override {
        return static_cast<int>(value_);
    }

    float value() const { return value_; }

    void Accept(IRVisitor *v) override;

    ~FloatImm() override {
        std::cout << "~FloatImm(" << value_ << ")" << std::endl;
    }
};

// ========== 2. TVM 风格的 IRVisitor（访问器） ==========
// TVM 的 IR 遍历需要用 Visitor 模式，因为 C++ 不支持在运行时根据类型 dispatch
// 原理：每个子类调用 visitor 的对应重载版本（双重分发）

class Add;
class Mul;

class IRVisitor {
public:
    // 为每种具体类型提供重载
    virtual void Visit(const IntImm &node) {
        std::cout << "  [Visitor] IntImm(" << node.value() << ")" << std::endl;
    }
    virtual void Visit(const FloatImm &node) {
        std::cout << "  [Visitor] FloatImm(" << node.value() << ")" << std::endl;
    }
    virtual void Visit(const Add &node);
    virtual void Visit(const Mul &node);

    virtual ~IRVisitor() = default;
};

// 再定义两个二元运算节点（它们也继承 Expr）
class Add : public Expr {
public:
    Expr *a_, *b_;
    Add(Expr *a, Expr *b) : a_(a), b_(b) {}
    std::string type_name() const override { return "Add"; }
    int IsInstance() const override { return 0; }
    void Accept(IRVisitor *v) override { v->Visit(*this); }
};

class Mul : public Expr {
public:
    Expr *a_, *b_;
    Mul(Expr *a, Expr *b) : a_(a), b_(b) {}
    std::string type_name() const override { return "Mul"; }
    int IsInstance() const override { return 0; }
    void Accept(IRVisitor *v) override { v->Visit(*this); }
};

void IRVisitor::Visit(const Add &node) {
    std::cout << "  [Visitor] Add" << std::endl;
    node.a_->Accept(this);
    node.b_->Accept(this);
}
void IRVisitor::Visit(const Mul &node) {
    std::cout << "  [Visitor] Mul" << std::endl;
    node.a_->Accept(this);
    node.b_->Accept(this);
}

// 先把 IntImm / FloatImm 的 Accept 定义放在 IRVisitor 之后
void IntImm::Accept(IRVisitor *v) { v->Visit(*this); }
void FloatImm::Accept(IRVisitor *v) { v->Visit(*this); }

// TVM 中还有一个 MatchTag 机制：每个 IR 节点有一个 type_key 字符串
// 底层用 dynamic_cast 实现，TVM 封装成了 IsInstance<T>() / As<T>()

// ========== 3. 虚析构函数演示 ==========
void test_virtual_destructor() {
    std::cout << "\n--- 虚析构函数演示 ---" << std::endl;
    Expr *p = new IntImm(42);
    delete p;  // 如果 ~Expr() 不是虚函数，这里只会调用 ~Expr()，不会调用 ~IntImm()
}

// ========== 4. dynamic_cast 与多态 ==========
void test_dynamic_cast() {
    std::cout << "\n--- dynamic_cast 演示 ---" << std::endl;

    Expr *p1 = new IntImm(10);
    Expr *p2 = new FloatImm(3.14f);

    // 向下转型：基类指针 → 派生类指针，需要 dynamic_cast 保证安全
    if (IntImm *ip = dynamic_cast<IntImm *>(p1)) {
        std::cout << "  p1 is IntImm, value = " << ip->value() << std::endl;
    } else {
        std::cout << "  p1 is NOT IntImm" << std::endl;
    }

    if (IntImm *ip = dynamic_cast<IntImm *>(p2)) {
        std::cout << "  p2 is IntImm, value = " << ip->value() << std::endl;
    } else {
        std::cout << "  p2 is NOT IntImm" << std::endl;
    }

    // dynamic_cast 失败返回 nullptr（指针）或抛出 std::bad_cast（引用）
    try {
        FloatImm &rf = dynamic_cast<FloatImm &>(*p1);
        (void)rf;
    } catch (const std::bad_cast &e) {
        std::cout << "  p1 is not FloatImm: " << e.what() << std::endl;
    }

    delete p1;
    delete p2;
}

// ========== 5. 完整 Visitor 模拟 TVM IR 遍历 ==========
void test_visitor() {
    std::cout << "\n--- TVM 风格 Visitor 遍历 IR 树 ---" << std::endl;

    IntImm i1(1), i2(2), i3(3), i4(4);
    Add add1(&i1, &i2);   // 1 + 2
    Mul mul1(&i3, &i4);   // 3 * 4
    Add root(&add1, &mul1); // (1+2) + (3*4)

    IRVisitor visitor;
    root.Accept(&visitor);
}

int main() {
    // 1. 虚函数与多态
    std::cout << "--- 虚函数与多态 ---" << std::endl;
    // 通过基类指针/引用调用的方法会在运行时动态分派到实际类型
    Expr *arr[] = {new IntImm(1), new FloatImm(2.5f)};
    for (auto *e : arr) {
        std::cout << "  type_name: " << e->type_name()
                  << ", IsInstance: " << e->IsInstance() << std::endl;
    }
    for (auto *e : arr) delete e;

    // 2. 虚析构函数
    test_virtual_destructor();

    // 3. dynamic_cast
    test_dynamic_cast();

    // 4. Visitor 模拟 TVM IR 遍历
    test_visitor();

    return 0;
}



// 1. 函数定义中加上 const 的作用，承诺函数不会修改成员变量值
// 2. override，表明这是对基类函数的重写
// 3. 基类中的函数是const，重写的时候也必须加上const

// // 初始化列表写法
// Add(Expr *a, Expr *b) : a_(a), b_(b) {}
// // 等价于：
// Add(Expr *a, Expr *b) {
//     a_ = a;   // 先调用 a_ 的默认构造函数，再赋值
//     b_ = b;
// }