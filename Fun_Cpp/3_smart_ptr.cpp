#include <iostream>
#include <memory>   // unique_ptr, shared_ptr, make_unique, make_shared
#include <vector>
#include <string>

// ========== 0. 前置知识：RAII ==========
// RAII (Resource Acquisition Is Initialization)：资源在构造函数获取，在析构函数释放
// 智能指针是 RAII 的典型应用——堆内存的生命周期绑定到栈对象的生命周期

// ========== 1. std::unique_ptr（独占所有权） ==========
// 不能被拷贝，只能被移动。指针被销毁时自动 delete 所管理的对象

class Node {
    std::string name_;
public:
    Node(std::string name) : name_(name) {
        std::cout << "  Node(" << name_ << ") 构造" << std::endl;
    }
    void greet() const {
        std::cout << "  Hello from " << name_ << std::endl;
    }
    ~Node() {
        std::cout << "  Node(" << name_ << ") 析构" << std::endl;
    }
};

void test_unique_ptr() {
    std::cout << "\n=== unique_ptr 演示 ===" << std::endl;

    // 创建方式（推荐用 make_unique，异常安全且少一次 new 调用）
    auto p1 = std::make_unique<Node>("A");
    p1->greet();

    // 所有权转移：std::move 将 p1 的资源转给 p2，p1 变为空
    std::unique_ptr<Node> p2 = std::move(p1);
    if (!p1) std::cout << "  p1 已为空" << std::endl;
    p2->greet();

    // unique_ptr 不能拷贝（编译错误）
    // auto p3 = p2;                    // 错误！unique_ptr 没有拷贝构造
    auto p3 = std::move(p2);            // 正确：移动语义
    if (!p2) std::cout << "  p2 已为空" << std::endl;

    // 放入容器（也必须移动，unique_ptr 是 move-only 类型）
    std::vector<std::unique_ptr<Node>> vec;
    vec.push_back(std::make_unique<Node>("B"));
    vec.push_back(std::make_unique<Node>("C"));
    for (auto &p : vec) p->greet();

    // 函数传参：通过引用传入（不转移所有权）
    auto print_name = [](const std::unique_ptr<Node> &p) {
        if (p) p->greet();
    };
    print_name(p3);

    // 函数返回：直接返回 unique_ptr（自动移动）
    auto create_node = [](const std::string &s) {
        return std::make_unique<Node>(s);
    };
    auto p4 = create_node("D");
    // p3 和 p4 在此作用域结束后自动析构
}

// ========== 2. std::shared_ptr（共享所有权） ==========
// 引用计数：多个 shared_ptr 共享同一对象，最后一个销毁时释放资源

class ExprNode {
    std::string op_;
public:
    ExprNode(std::string op) : op_(op) {
        std::cout << "  ExprNode(" << op_ << ") 构造" << std::endl;
    }
    std::string op() const { return op_; }
    ~ExprNode() {
        std::cout << "  ExprNode(" << op_ << ") 析构" << std::endl;
    }
};

void test_shared_ptr() {
    std::cout << "\n=== shared_ptr 演示 ===" << std::endl;

    // 推荐用 make_shared（一次分配控制块+对象，更高效）
    auto sp1 = std::make_shared<ExprNode>("Add");
    std::cout << "  use_count = " << sp1.use_count() << std::endl;  // 1

    // 拷贝：引用计数 +1
    auto sp2 = sp1;
    std::cout << "  use_count = " << sp1.use_count() << std::endl;  // 2

    auto sp3 = sp1;
    std::cout << "  use_count = " << sp1.use_count() << std::endl;  // 3

    // sp2 和 sp3 析构后引用计数递减
    sp2.reset();
    std::cout << "  after reset sp2, use_count = " << sp1.use_count() << std::endl;
    sp3.reset();
    std::cout << "  after reset sp3, use_count = " << sp1.use_count() << std::endl;

    // shared_ptr 可以拷贝（有别于 unique_ptr）
    // 模拟构建 IR 树：子树被多个父节点共享
    auto child = std::make_shared<ExprNode>("Const(1)");
    auto parent1 = std::make_shared<ExprNode>("Add");
    auto parent2 = std::make_shared<ExprNode>("Mul");
    // 实际 TVM 的 IR 树更复杂，这里只示意共享语义
    std::cout << "  child use_count = " << child.use_count() << std::endl;
    // sp1 和 child 在此结束后析构
}

// ========== 3. 模拟 TVM 的引用计数 IR 节点 ==========
// TVM 没有用 std::shared_ptr，而是自己实现了 Object/ObjectRef 机制
// 核心思想：引用计数，浅拷贝（拷贝只增加引用计数），CopyOnWrite 修改时深拷贝
// 这里简化模拟

class TVMObject {
    int ref_count_ = 0;
public:
    // TVM 中通过 Retain/Release 手动管理计数
    void Retain() { ref_count_++; }
    void Release() {
        if (--ref_count_ == 0) {
            std::cout << "  ref count = 0, 删除对象" << std::endl;
            delete this;
        }
    }
    virtual std::string type_key() const = 0;
    virtual ~TVMObject() = default;
protected:
    TVMObject() = default;
};

// 简单的模拟智能指针
template <typename T>
class TVMRef {
    T *ptr_ = nullptr;
public:
    explicit TVMRef(T *ptr) : ptr_(ptr) { if (ptr_) ptr_->Retain(); }
    TVMRef(const TVMRef &other) : ptr_(other.ptr_) { if (ptr_) ptr_->Retain(); }
    TVMRef(TVMRef &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    ~TVMRef() { if (ptr_) ptr_->Release(); }

    T *get() const { return ptr_; }
    T *operator->() const { return ptr_; }
    T &operator*() const { return *ptr_; }
    // 禁用拷贝赋值（简化），实际 TVM 的 ObjectRef 是可拷贝的
};

void test_tvm_style_rc() {
    std::cout << "\n=== 模拟 TVM 引用计数 IR 节点 ===\n" << std::endl;

    class IntConstant : public TVMObject {
        int value_;
    public:
        IntConstant(int v) : value_(v) {}
        std::string type_key() const override { return "IntConstant"; }
        int value() const { return value_; }
    };

    // 手动 new + 智能包装（实际 TVM 有更完善的 Allocation 机制）
    TVMRef<IntConstant> a(new IntConstant(42));
    std::cout << "  value = " << a->value() << ", type = " << a->type_key() << std::endl;

    // 拷贝：引用计数 +1
    TVMRef<IntConstant> b = a;
    std::cout << "  拷贝构造 b" << std::endl;
    // b 和 a 析构时 release 两次，计数到 0 时删除
}

// ========== 4. 右值引用 && 和 std::move ==========

void test_rvalue_and_move() {
    std::cout << "\n=== 右值引用 && 和 std::move 演示 ===" << std::endl;

    // 左值：有地址、可取名的表达式
    // 右值：临时对象、无名字、即将销毁的表达式（如 42, func() 返回值）

    // && 右值引用：专门绑定到右值，允许"偷"走其资源
    std::string a = "hello";
    std::string b = "world";

    // std::move 将左值强制转换为右值引用
    // 它不移动任何东西，只是"告诉编译器可以偷走这个资源"
    std::string c = std::move(a);   // a 的资源被"移动"给 c
    std::cout << "  after move: c = " << c << ", a = \"" << a << "\" (已空)" << std::endl;

    // 为什么 unique_ptr 用 move？
    // unique_ptr 禁止拷贝，但允许移动 —— 移动后原指针放弃所有权
    auto up1 = std::make_unique<std::string>("独占资源");
    auto up2 = std::move(up1);
    if (!up1) std::cout << "  unique_ptr 移动后原指针为空" << std::endl;
    std::cout << "  up2: " << *up2 << std::endl;

    // 右值引用在函数参数中的应用：移动构造函数
    class MyBuffer {
        int *data_;
        size_t size_;
    public:
        MyBuffer(size_t n) : size_(n), data_(new int[n]) {
            std::cout << "  构造 MyBuffer(" << n << ")" << std::endl;
        }
        // 移动构造函数：从 other 偷走资源，清空 other
        MyBuffer(MyBuffer &&other) noexcept
            : data_(other.data_), size_(other.size_) {
            other.data_ = nullptr;
            other.size_ = 0;
            std::cout << "  移动构造 MyBuffer" << std::endl;
        }
        ~MyBuffer() {
            delete[] data_;
            std::cout << "  析构 MyBuffer" << std::endl;
        }
        // 禁止拷贝（简化）
        MyBuffer(const MyBuffer &) = delete;
    };

    MyBuffer buf1(100);
    MyBuffer buf2 = std::move(buf1);  // 触发移动构造，无拷贝开销
}

// ========== 5. 综合：用智能指针构建小型 IR 树 ==========

// 简化版 IR 节点：用 unique_ptr 表达独占子节点所有权
class IRNode {
public:
    virtual std::string repr() const = 0;
    virtual ~IRNode() = default;
};

class IntConst : public IRNode {
    int val_;
public:
    IntConst(int v) : val_(v) {}
    std::string repr() const override { return std::to_string(val_); }
};

class BinaryOp : public IRNode {
    std::string op_;
    std::unique_ptr<IRNode> lhs_, rhs_;
public:
    BinaryOp(std::string op,
             std::unique_ptr<IRNode> lhs,
             std::unique_ptr<IRNode> rhs)
        : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    // 注意：这里用 std::move 将 unique_ptr 传入，所有权被转移到成员
    std::string repr() const override {
        return "(" + lhs_->repr() + " " + op_ + " " + rhs_->repr() + ")";
    }
};

void test_ir_tree() {
    std::cout << "\n=== 用 unique_ptr 构建 IR 树 ===" << std::endl;

    // 构造 (1 + (2 * 3))
    auto tree = std::make_unique<BinaryOp>(
        "+",
        std::make_unique<IntConst>(1),
        std::make_unique<BinaryOp>(
            "*",
            std::make_unique<IntConst>(2),
            std::make_unique<IntConst>(3)
        )
    );
    std::cout << "  tree = " << tree->repr() << std::endl;
    // 离开作用域自动释放整棵树
}

int main() {
    test_unique_ptr();
    test_shared_ptr();
    test_tvm_style_rc();
    test_rvalue_and_move();
    test_ir_tree();

    std::cout << "\nmain 结束，所有资源自动释放" << std::endl;
    return 0;
}
