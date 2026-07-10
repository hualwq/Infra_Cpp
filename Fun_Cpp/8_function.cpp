#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

// ========== 前置知识：函数指针的局限 ==========
// 函数指针只能指向"无状态"的可调用实体（普通函数、静态成员函数）
// 无法持有捕获了变量的 lambda、函数对象等"有状态"的可调用对象

int add(int a, int b) { return a + b; }

void demo_function_pointer_limit() {
    std::cout << "\n=== 函数指针的局限 ===" << std::endl;

    // 函数指针：只能指向普通函数
    int (*fp)(int, int) = add;
    std::cout << "函数指针调用 add(3, 4) = " << fp(3, 4) << std::endl;

    // 下面这行编译错误：捕获了变量的 lambda 不能转成函数指针
    // int (*fp2)(int) = [factor](int x) { return x * factor; };  // 错误！
    std::cout << "函数指针无法持有状态（捕获变量的 lambda）" << std::endl;
}

// ========== 1. std::function 基础：统一包装各种可调用对象 ==========
// std::function<返回类型(参数类型...)> 是一个通用多态函数包装器
// 它可以装：普通函数、lambda、函数对象、成员函数指针

class Multiply {
public:
    int operator()(int a, int b) const { return a * b; }
};

void demo_basic() {
    std::cout << "\n=== 1. std::function 基础 ===" << std::endl;

    // 1a. 包装普通函数
    std::function<int(int, int)> f1 = add;
    std::cout << "包装普通函数: f1(3, 4) = " << f1(3, 4) << std::endl;

    // 1b. 包装无捕获 lambda
    f1 = [](int a, int b) { return a - b; };
    std::cout << "包装无捕获 lambda: f1(10, 3) = " << f1(10, 3) << std::endl;

    // 1c. 包装有捕获 lambda（函数指针做不到的事）
    int factor = 3;
    std::function<int(int)> f2 = [factor](int x) { return x * factor; };
    std::cout << "包装有捕获 lambda (factor=" << factor << "): f2(7) = " << f2(7) << std::endl;

    // 1d. 包装函数对象
    std::function<int(int, int)> f3 = Multiply{};
    std::cout << "包装函数对象: f3(6, 7) = " << f3(6, 7) << std::endl;

    // 1e. 可以重新绑定（只要签名匹配）
    f1 = [](int a, int b) { return a * b; };
    std::cout << "重新绑定后: f1(6, 7) = " << f1(6, 7) << std::endl;
}

// ========== 2. std::function 拥有性：深拷贝可调用对象 ==========
// std::function 会拷贝/移动它包装的可调用对象，因此拥有其生命周期

void demo_ownership() {
    std::cout << "\n=== 2. std::function 的拥有性 ===" << std::endl;

    // lambda 捕获的变量会被 std::function 拷贝进去
    int base = 100;
    std::function<int(int)> f = [base](int x) { return x + base; };

    base = 999;  // 修改外部变量，不影响 std::function 内部的副本
    std::cout << "外部 base 改为 " << base
              << "，但 f(5) = " << f(5) << "（内部副本仍是 100）" << std::endl;

    // std::function 可以安全地返回，捕获的状态跟着走
    auto make_adder = [](int n) -> std::function<int(int)> {
        return [n](int x) { return x + n; };  // n 被捕获进 std::function
    };
    auto add10 = make_adder(10);
    auto add20 = make_adder(20);
    std::cout << "add10(5) = " << add10(5) << std::endl;
    std::cout << "add20(5) = " << add20(5) << std::endl;
}

// ========== 3. std::function 作为函数参数：回调 / 策略模式 ==========

void process_numbers(const std::vector<int> &nums,
                    std::function<void(int)> callback) {
    for (int n : nums) {
        callback(n);
    }
}

void demo_as_callback() {
    std::cout << "\n=== 3. std::function 作为回调参数 ===" << std::endl;

    std::vector<int> nums = {1, 2, 3, 4, 5};

    // 传 lambda
    std::cout << "打印每个数: ";
    process_numbers(nums, [](int n) { std::cout << n << " "; });
    std::cout << std::endl;

    // 传有捕获的 lambda
    int total = 0;
    process_numbers(nums, [&total](int n) { total += n; });
    std::cout << "求和结果: " << total << std::endl;

    // 传普通函数
    process_numbers(nums, [](int n) {
        if (n % 2 == 0) std::cout << n << " ";
    });
    std::cout << "← 偶数" << std::endl;
}

// ========== 4. std::function 存入容器：策略模式 ==========

void demo_in_container() {
    std::cout << "\n=== 4. std::function 存入容器（策略模式）===" << std::endl;

    // 把不同的操作存进 vector，统一调用
    std::vector<std::function<int(int, int)>> ops = {
        [](int a, int b) { return a + b; },
        [](int a, int b) { return a - b; },
        [](int a, int b) { return a * b; },
        [](int a, int b) { return b != 0 ? a / b : 0; },
    };

    int x = 10, y = 3;
    std::string op_names[] = {"加", "减", "乘", "除"};
    for (size_t i = 0; i < ops.size(); i++) {
        std::cout << op_names[i] << "(" << x << ", " << y
                  << ") = " << ops[i](x, y) << std::endl;
    }
}

// ========== 5. std::function 的空状态与 bool 检查 ==========

void demo_empty_check() {
    std::cout << "\n=== 5. std::function 的空状态 ===" << std::endl;

    std::function<int(int, int)> f;  // 默认构造：空

    if (!f) {
        std::cout << "f 当前为空（未绑定任何可调用对象）" << std::endl;
    }

    f = add;
    if (f) {
        std::cout << "f 绑定后调用: " << f(3, 4) << std::endl;
    }

    f = nullptr;  // 重置为空
    if (!f) {
        std::cout << "f 被重置为空" << std::endl;
    }

    // 警告：对空 std::function 调用会抛 std::bad_function_call
    // f(1, 2);  // 运行时异常！
}

// ========== 6. std::function vs function_ref（对比） ==========
// std::function：拥有可调用对象，可长期存储，有拷贝/堆分配开销
// function_ref：  不拥有，只保存指针/引用，零开销，不能长期存储

// 模拟一个简单的 function_ref（参考 AIcompiler 07 的实现思路）
class FuncRef {
    void *ptr_ = nullptr;
    int (*call_)(void *, int) = nullptr;

public:
    FuncRef() = default;

    template <typename Callable>
    FuncRef(Callable &c)
        : ptr_(static_cast<void *>(&c)),
          call_([](void *p, int v) { return (*static_cast<Callable *>(p))(v); }) {}

    int operator()(int v) const {
        return call_(ptr_, v);
    }

    explicit operator bool() const { return call_ != nullptr; }
};

void apply_twice(FuncRef f, int value) {
    std::cout << "apply_twice(" << value << ") = "
              << f(f(value)) << std::endl;
}

void demo_vs_function_ref() {
    std::cout << "\n=== 6. std::function vs function_ref 对比 ===" << std::endl;

    // std::function：拥有对象，可以存储
    std::function<int(int)> owned;
    {
        int k = 5;
        owned = [k](int x) { return x * k; };  // std::function 拷贝了 k
    }
    std::cout << "std::function 拥有状态，离开作用域仍可用: "
              << owned(3) << std::endl;

    // function_ref：不拥有，只能短期使用
    int scale = 10;
    auto lambda = [&scale](int x) { return x * scale; };
    std::cout << "function_ref 不拥有状态，需保证生命周期: ";
    apply_twice(lambda, 3);
    // 注意：不能把 lambda 的临时对象传给 apply_twice，会悬空
}

// ========== 7. 编译器场景：用 std::function 做 IR 遍历回调 ==========
// 在 LLVM / TVM 等编译器中，遍历 IR 节点时常用回调模式

class IRNode {
public:
    virtual std::string name() const = 0;
    virtual ~IRNode() = default;
};

class AddNode : public IRNode {
    std::string name() const override { return "Add"; }
};

class MulNode : public IRNode {
    std::string name() const override { return "Mul"; }
};

void walk_ir(const std::vector<std::unique_ptr<IRNode>> &nodes,
             std::function<void(const IRNode *)> visitor) {
    for (const auto &node : nodes) {
        visitor(node.get());
    }
}

void demo_compiler_use_case() {
    std::cout << "\n=== 7. 编译器场景：IR 遍历回调 ===" << std::endl;

    std::vector<std::unique_ptr<IRNode>> ir;
    ir.push_back(std::make_unique<AddNode>());
    ir.push_back(std::make_unique<MulNode>());
    ir.push_back(std::make_unique<AddNode>());

    // 用 lambda 做 visitor，捕获外部变量记录信息
    int add_count = 0, mul_count = 0;
    walk_ir(ir, [&](const IRNode *node) {
        std::cout << "访问节点: " << node->name() << std::endl;
        if (node->name() == "Add") add_count++;
        else if (node->name() == "Mul") mul_count++;
    });
    std::cout << "统计: Add=" << add_count << ", Mul=" << mul_count << std::endl;
}

int main() {
    demo_function_pointer_limit();
    demo_basic();
    demo_ownership();
    demo_as_callback();
    demo_in_container();
    demo_empty_check();
    demo_vs_function_ref();
    demo_compiler_use_case();

    return 0;
}
