#include <iostream>
#include <vector>
#include <type_traits>
#include <string>
#include <memory>
#include <functional>
#include <sstream>
#include <utility>
#include <vector>

// ===== 1. 函数模板 =====
template <typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

// ===== 2. 类模板 =====
template <typename T>
class Box {
    T value;
public:
    explicit Box(T v) : value(v) {}
    T get() const { return value; }
    void set(T v) { value = v; }
};

// ===== 3. 全特化 =====
template <>
class Box<const char*> {
    const char* value;
public:
    explicit Box(const char* v) : value(v) {}
    const char* get() const { return value; }
};

// ===== 4. 偏特化 =====
template <typename T>
class Box<std::vector<T>> {
    std::vector<T> data;
public:
    explicit Box(std::vector<T> v) : data(std::move(v)) {}
    typename std::vector<T>::size_type size() const { return data.size(); }
};

// ===== 5. auto & decltype =====
template <typename T, typename U>
auto mul(T a, U b) -> decltype(a * b) {
    return a * b;
}

// ===== 6. 可变参数模板：基础 =====
template <typename... Args>
void print(Args... args) {
    (std::cout << ... << args) << "\n";
}

// ===== 7. 参数包展开：递归求和 =====
auto sum() { return 0; }

template <typename T, typename... Args>
auto sum(T first, Args... rest) {
    return first + sum(rest...);
}

// ===== 8. 折叠表达式 (C++17) =====
template <typename... Args>
auto foldSum(Args... args) {
    return (... + args);  // 一元左折
}

template <typename... Args>
bool allTrue(Args... args) {
    return (... && args);  // 一元左折
}

template <typename... Args>
void printWithSep(const std::string& sep, Args... args) {
    ((std::cout << args << sep), ...) << "\n";  // 逗号表达式折
}

// ===== 9. TVM 风格：IR 节点构造器 =====
// 模拟 TVM 的 IR 节点：Relay 风格的 Expr
class Expr {
public:
    virtual ~Expr() = default;
    virtual std::string repr() const = 0;
};

class Constant : public Expr {
    int val_;
public:
    explicit Constant(int v) : val_(v) {}
    std::string repr() const override { return std::to_string(val_); }
};

// TVM::tir::Add::make(lhs, rhs) 风格：工厂方法 + 可变参数
class Add : public Expr {
    std::unique_ptr<Expr> lhs_, rhs_;
public:
    template <typename L, typename R>
    static auto make(L&& l, R&& r) {
        // 完美转发构造唯一指针
        return std::make_unique<Add>(
            std::make_unique<std::decay_t<L>>(std::forward<L>(l)),
            std::make_unique<std::decay_t<R>>(std::forward<R>(r))
        );
    }
    Add(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : lhs_(std::move(l)), rhs_(std::move(r)) {}
    std::string repr() const override {
        return "(" + lhs_->repr() + " + " + rhs_->repr() + ")";
    }
};

// ===== 10. TVM 风格：Visitor 的 Visit 函数 =====
// 模拟 TVM 的 NodeVisitor::Visit(const T& node) 用可变参数注册
template <typename... ExprTypes>
class Visitor;

// 基类：展开 ExprTypes 为每个类型生成 Visit 虚函数
template <typename... ExprTypes>
class Visitor {
public:
    virtual ~Visitor() = default;
    // 为每个类型生成一个 Visit 重载
    virtual void Visit(const ExprTypes&... args) = 0;
};

// 特化展开示例：Printer 访问多种表达式
class PrinterVisitor : public Visitor<Constant, Add> {
public:
    // 参数包展开同时处理所有类型
    void Visit(const Constant& c, const Add& a) override {
        std::cout << "[Visitor] Constant: " << c.repr()
                  << " | Add: " << a.repr() << "\n";
    }
};

// ===== 11. Lambda + 虚函数 + 可变参数合并 =====
// 用可变参数模板包装 lambda 到多态接口中
class FunctorBase {
public:
    virtual ~FunctorBase() = default;
    virtual int apply(int v) const = 0;
};

// 接受任意 lambda 的包装器
template <typename F>
class Functor : public FunctorBase {
    F f_;
public:
    explicit Functor(F f) : f_(std::move(f)) {}
    int apply(int v) const override { return f_(v); }
};

// 工厂函数：lambda → 多态对象
template <typename F>
auto makeFunctor(F&& f) {
    return std::make_unique<Functor<std::decay_t<F>>>(std::forward<F>(f));
}

// ===== 12. 高阶：可变参数 + lambda 构建 AST =====
// 模拟 TVM Relay IR 构建器的风格
class IRBuilder {
    std::vector<std::unique_ptr<Expr>> nodes;
public:
    template <typename NodeType, typename... Args>
    auto emplace(Args&&... args) {
        auto node = std::make_unique<NodeType>(std::forward<Args>(args)...);
        auto* ptr = node.get();
        nodes.push_back(std::move(node));
        return ptr;
    }
    void dump() const {
        for (const auto& n : nodes)
            std::cout << "  " << n->repr() << "\n";
    }
};

int main() {
    // 函数模板实例化
    std::cout << max(3, 7) << "\n";
    std::cout << max(3.14, 2.72) << "\n";

    // 类模板
    Box<int> ib(42);
    std::cout << ib.get() << "\n";

    // 全特化
    Box<const char*> sb("hello template");
    std::cout << sb.get() << "\n";

    // 偏特化
    Box<std::vector<int>> vb(std::vector{1, 2, 3});
    std::cout << vb.size() << "\n";

    // auto & decltype
    std::cout << mul(3, 4.5) << "\n";

    // 可变参数模板
    std::cout << "\n--- 6. 可变参数模板 print ---\n";
    print(1, " + ", 2.0, " = ", 3);

    std::cout << "\n--- 7. 递归包展开 sum ---\n";
    std::cout << "sum(1,2,3,4,5) = " << sum(1, 2, 3, 4, 5) << "\n";

    std::cout << "\n--- 8. 折叠表达式 ---\n";
    std::cout << "foldSum(1,2,3,4) = " << foldSum(1, 2, 3, 4) << "\n";
    std::cout << "allTrue(true,true,false) = " << allTrue(true, true, false) << "\n";
    printWithSep(" | ", "a", "b", "c");

    std::cout << "\n--- 9. TVM 风格 IR 构造 ---\n";
    auto expr = Add::make(Constant(1), Constant(2));
    std::cout << "expr = " << expr->repr() << "\n";

    std::cout << "\n--- 10. TVM 风格 Visitor ---\n";
    PrinterVisitor pv;
    Constant c10(10);
    Add a10(std::make_unique<Constant>(3), std::make_unique<Constant>(4));
    pv.Visit(c10, a10);

    std::cout << "\n--- 11. Lambda + 虚函数 ---\n";
    auto f1 = makeFunctor([](int x) { return x * x; });
    auto f2 = makeFunctor([](int x) { return x + 10; });
    std::cout << "f1(5) = " << f1->apply(5) << "\n";
    std::cout << "f2(5) = " << f2->apply(5) << "\n";

    std::cout << "\n--- 12. 可变参数 IRBuilder ---\n";
    IRBuilder builder;
    builder.emplace<Constant>(42);
    builder.emplace<Add>(
        std::make_unique<Constant>(1),
        std::make_unique<Constant>(2)
    );
    builder.dump();
}





// && 的作用是什么？
// 根据传入的是左值还是右值，自动折叠为左值引用还是右值引用



// template<typename T>
// void wrapper(T&& arg) { // 这里 T&& 是万能引用，不是右值引用
//     // 将 arg 完美转发给其他函数
//     other_func(std::forward<T>(arg));
// }

// int x = 10;
// wrapper(x);  // 传入左值，T 推导为 int&，函数实例化为 wrapper(int&)
// wrapper(10); // 传入右值，T 推导为 int，函数实例化为 wrapper(int&&)


// int asd(int& a){
//     std::cout << a << std::endl;
// }

// //普通的函数，如果是引用，则只能传左值，不能传右值