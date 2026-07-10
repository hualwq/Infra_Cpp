/*
 * 1_ptr.cpp - shared_ptr & unique_ptr 核心用法示例
 *
 * 编译: g++ -std=c++17 -o 1_ptr 1_ptr.cpp
 * 运行: ./1_ptr
 */

#include <iostream>
#include <memory>
#include <vector>

// ==================== 辅助类 ====================
struct Widget {
    int id;
    explicit Widget(int n) : id(n) { std::cout << "  Widget(" << id << ") constructed\n"; }
    ~Widget() { std::cout << "  Widget(" << id << ") destroyed\n"; }
    void greet() const { std::cout << "  Hello from Widget " << id << "\n"; }
};

// ==================== unique_ptr 示例 ====================
void demo_unique_ptr() {
    std::cout << "\n========== unique_ptr 示例 ==========\n";

    // --- 1. 基本创建 ---
    std::unique_ptr<Widget> p1 = std::make_unique<Widget>(1);
    p1->greet();                     // operator->
    (*p1).greet();                   // operator*

    // std::unique_ptr<Widget> p2 = std::make_unique<Widget>(2);

    // --- 2. 所有权转移 (move only) ---
    std::unique_ptr<Widget> p2 = std::move(p1);   // p1 变 null
    // p1->greet();
    if (!p1) std::cout << "  p1 is null after move\n";
    p2->greet();

    // std::unique_ptr<Widget> p3 = std::move(p2);

    // --- 3. release() / reset() ---
    auto p3 = std::make_unique<Widget>(2);
    Widget* raw = p3.release();           // 释放所有权，释放之后， raw 指的是这个裸指针
    delete raw;

    auto p4 = std::make_unique<Widget>(3);
    p4.reset();                            // 销毁对象，p4 变 null

    // --- 4. 数组支持 ---
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * i;
    std::cout << "  array[3] = " << arr[3] << "\n";

    // --- 5. 函数返回值 ---
    auto make_widget = [](int n) -> std::unique_ptr<Widget> {
        return std::make_unique<Widget>(n);
    };
    auto p5 = make_widget(4);
    p5->greet();

    // --- 6. 存入容器 ---
    std::vector<std::unique_ptr<Widget>> vec;
    vec.push_back(std::make_unique<Widget>(10));
    vec.push_back(std::make_unique<Widget>(11));
    for (auto& wp : vec) wp->greet();
}

// ==================== 循环引用前向声明 ====================
struct Child;

struct Parent {
    std::shared_ptr<Child> child;
    ~Parent() { std::cout << "  Parent destroyed\n"; }
};

struct Child {
    std::weak_ptr<Parent> parent;   // weak_ptr 打破循环
    ~Child() { std::cout << "  Child destroyed\n"; }
};

// ==================== shared_ptr + weak_ptr 示例 ====================
void demo_shared_weak_ptr() {
    std::cout << "\n========== shared_ptr 示例 ==========\n";

    // --- 1. 基本创建与引用计数 ---
    auto sp1 = std::make_shared<Widget>(100);
    std::cout << "  use_count: " << sp1.use_count() << "\n";  // 1

    {
        auto sp2 = sp1;   // 拷贝，引用计数 +1
        std::cout << "  use_count after copy: " << sp1.use_count() << "\n";  // 2
    }  // sp2 析构，-1
    std::cout << "  use_count final: " << sp1.use_count() << "\n";  // 1

    // --- 2. get() / reset() ---
    Widget* raw = sp1.get();
    (void)raw;

    auto sp3 = std::make_shared<Widget>(101);
    sp3.reset();   // 销毁对象
    std::cout << "  after reset, sp3 is " << (sp3 ? "alive" : "null") << "\n";

    // --- 3. 自定义删除器 ---
    std::cout << "  --- custom deleter ---\n";
    std::shared_ptr<Widget> sp4(new Widget(200), [](Widget* p) {
        std::cout << "  [custom deleter] deleting Widget " << p->id << "\n";
        delete p;
    });

    // --- 4. weak_ptr 打破循环引用 ---
    std::cout << "\n========== weak_ptr 示例 ==========\n";
    auto father = std::make_shared<Parent>();
    auto son   = std::make_shared<Child>();

    father->child = son;
    son->parent   = father;

    std::cout << "  father use_count: " << father.use_count() << "\n";  // 1（weak不计入）
    std::cout << "  son use_count: "    << son.use_count()   << "\n";  // 1

    // weak_ptr 用法
    if (auto sp = son->parent.lock()) {          // lock() 获取 shared_ptr
        std::cout << "  lock() succeeded\n";
    }
    if (!son->parent.expired()) {                 // expired() 检查是否存活
        std::cout << "  parent is alive\n";
    }

    std::cout << "  leaving scope, both destroyed correctly\n";
    // father & son 会正确析构，无内存泄漏
}

// ==================== enable_shared_from_this ====================
class SharedNode : public std::enable_shared_from_this<SharedNode> {
public:
    int id;
    explicit SharedNode(int n) : id(n) {}
    std::shared_ptr<SharedNode> shared() { return shared_from_this(); }
};

void demo_shared_from_this() {
    std::cout << "\n========== enable_shared_from_this ==========\n";
    auto a = std::make_shared<SharedNode>(1);
    auto self_a = a->shared();
    std::cout << "  shared_from_this() ok, use_count = " << a.use_count() << "\n";
}

// ==================== 选择指南 ====================
void demo_guidelines() {
    std::cout << "\n========== 选择指南 ==========\n";
    std::cout << "  unique_ptr  : 独占所有权，零开销\n";
    std::cout << "  shared_ptr  : 共享所有权，引用计数（原子操作有开销）\n";
    std::cout << "  weak_ptr    : 观察者，不控制生命周期\n";
    std::cout << "  最佳实践:\n";
    std::cout << "    ✅ 优先 make_unique / make_shared\n";
    std::cout << "    ✅ 能 unique 就别 shared\n";
    std::cout << "    ⚠️ 不要混用裸指针和智能指针管理同一对象\n";
}

// ==================== main ====================
int main() {
    demo_unique_ptr();
    demo_shared_weak_ptr();
    demo_shared_from_this();
    demo_guidelines();

    std::cout << "\nAll demos completed.\n";
    return 0;
}
