// ============================================================
// C++ 宏学习 —— 循序渐进，直到看懂 6 中的宏定义
// ============================================================
// 编译 & 运行:
//   g++ -std=c++17 -Wall 7_macros.cpp && ./a.out

#include <iostream>
#include <string>

// ============================================================
// 第0层：最基础的宏 —— 文本替换
// ============================================================
// 预处理器（在编译前运行）把代码中的宏名字直接替换成定义的内容

#define PI 3.1415926
#define GREETING "Hello, Macros!"

void layer0() {
    std::cout << "=== 第0层：基础文本替换 ===\n";
    std::cout << "PI = " << PI << "\n";          // → 3.1415926
    std::cout << GREETING << "\n";                // → "Hello, Macros!"
    // 反例：没有分号，展开后只是纯文本替换
}

// ============================================================
// 第1层：带参数的宏（函数式宏）
// ============================================================
// 看起来像函数，但本质还是文本替换
// 警告：参数会被多次求值！这里故意简化

#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void layer1() {
    std::cout << "\n=== 第1层：带参数的宏 ===\n";
    std::cout << "SQUARE(3+1) = " << SQUARE(3+1) << "\n";  // ((3+1)*(3+1)) = 16
    std::cout << "MAX(10, 20) = " << MAX(10, 20) << "\n";  // 20

    // 注意：如果没加外层括号，SQUARE(1+2) → 1+2*1+2 = 5 而非 9
    // 所以宏参数必须每处都加括号！
}

// ============================================================
// 第2层：Stringification —— # 运算符
// ============================================================
// # 把参数变成字符串字面量（加引号）

#define PRINT_EXPR(x) std::cout << #x " = " << (x) << "\n"

void layer2() {
    std::cout << "\n=== 第2层：# 运算符（字符串化）===\n";
    PRINT_EXPR(42);                    // → std::cout << "42" " = " << (42) << "\n";
    PRINT_EXPR(1 + 2 * 3);             // → 输出: 1 + 2 * 3 = 7
    PRINT_EXPR(std::string("hello").size());

    // #x 展开为 "1 + 2 * 3" —— 保留你写的表达式原文！
    // 这就是 6 里 #NodeType 的作用："ConstantNode"
}

// ============================================================
// 第3层：Token Concatenation —— ## 运算符
// ============================================================
// ## 把左右两边的 token（标识符）拼接成一个新 token  这里是 ## 这个标识符的作用

#define MAKE_UNIQUE(name) func_##name

int MAKE_UNIQUE(add)(int a, int b) { return a + b; }  // → int func_add(int a, int b)...
int MAKE_UNIQUE(mul)(int a, int b) { return a * b; }

void layer3() {
    std::cout << "\n=== 第3层：## 运算符（token 拼接）===\n";
    std::cout << "func_add(3, 4) = " << func_add(3, 4) << "\n";
    std::cout << "func_mul(3, 4) = " << func_mul(3, 4) << "\n";

    // 这就是 6 里 _reg_##NodeType 的作用：
    // TVM_REGISTER_NODE_TYPE(AddNode) 展开后产生变量名 _reg_AddNode
    // TVM_REGISTER_NODE_TYPE(MulNode) 展开后产生变量名 _reg_MulNode
    // 每个宏调用生成不同的变量名，避免重定义冲突
}


// ============================================================
// 第4层：多语句宏的陷阱 —— do { ... } while(0)
// ============================================================

#define BAD_SWAP(a, b) \
    int tmp = a;       \
    a = b;             \
    b = tmp;

#define GOOD_SWAP(a, b)  \
    do {                 \
        int tmp = (a);   \
        (a) = (b);       \
        (b) = (tmp);     \
    } while (0)

void layer4() {
    std::cout << "\n=== 第4层：多语句宏与 do-while(0) ===\n";

    int x = 1, y = 2;
    // if (false) BAD_SWAP(x, y);  // 编译错误！展开后 if 只跟了 int tmp...
    // 展开为:
    // if (false) int tmp = x;  ← 只有这一行在 if 里
    //     x = y;               ← 这些在外面！
    //     y = tmp;

    // GOOD_SWAP 安全：
    if (true) GOOD_SWAP(x, y);
    std::cout << "after swap: x=" << x << " y=" << y << "\n";
    // 展开为 do { ... } while(0) — 只执行一次，分号结尾安全
}

// ============================================================
// 第5层：Static 变量初始化技巧 —— 宏的核心"魔法"
// ============================================================
// 这是 6 里宏的关键: static bool _reg_XXX = [](){...}();
// C++ 中，static 变量在程序加载时（main 之前）初始化
// 结合 Immediately Invoked Lambda，可以在 main 前执行任意代码

int counter = 0;

// 一个"注册"宏：在 main 之前自动向全局注册一个名字
#define AUTO_REGISTER(name)                                              \
    static int _reg_token_##name = []() {                                \
        std::cout << "  [AUTO_REGISTER] registering \"" #name "\"\n";    \
        counter++;                                                       \
        return counter; /* 作为 static 变量的值，无所谓 */                \
    }()

void layer5() {
    std::cout << "\n=== 第5层：static 变量 + lambda 自动注册 ===\n";
    // 注意：AUTO_REGISTER 写在函数外面（全局作用域），
    // 但这个例子写在函数内部便于演示

    auto dummy = []() {
        // 宏展开示例：
        // static int _reg_token_hello = [](){ ... return 1; }();
        // 函数内的 static 变量在第一次调用时初始化（不是 main 之前）
        // 全局 static 变量在 main 之前初始化
        return 0;
    };
    (void)dummy;
    std::cout << "  (具体调用见下面全局作用域的示例)\n";
}

// ---- 全局作用域的自动注册 ----
// 这两行在 main 调用之前就会执行！

static int _reg_token_Pear = []() {
    std::cout << "  [GLOBAL] registering \"Pear\"\n";
    return 0;
}();

static int _reg_token_Apple = []() {
    std::cout << "  [GLOBAL] registering \"Apple\"\n";
    return 0;
}();

// ============================================================
// 第6层：组合所有技术 —— 理解 6 中的 TVM_REGISTER_NODE_TYPE
// ============================================================

// 我们先模拟一个简单的 Registry + 宏
#include <unordered_map>
#include <memory>

// 模拟的工厂注册表
class SimpleRegistry {
    std::unordered_map<std::string, std::string> names_;
    SimpleRegistry() = default;
public:
    static SimpleRegistry& Global() {
        static SimpleRegistry inst;
        return inst;
    }
    void Register(const std::string& key, const std::string& /*dummy*/) {
        std::cout << "  [SimpleRegistry] registered: " << key << "\n";
        names_[key] = key;
    }
    bool Has(const std::string& key) const {
        return names_.count(key) > 0;
    }
};

// ---------- 目标宏（完全和 6 中一致）----------
// 解释每一步：
//
// 宏定义（反斜杠续行会触发 -Wcomment，故用文字描述）：
//   define  TVM_REGISTER_NODE_TYPE(NodeType)
//     static bool _reg_##NodeType = []() {
//       Registry::Global().Register(#NodeType,
//         []() { return std::make_unique<NodeType>(); });
//       return true;
//     }()
//
// 展开示例：TVM_REGISTER_NODE_TYPE(AddNode)
//   static bool _reg_AddNode = []() {
//     Registry::Global().Register("AddNode",
//       []() { return std::make_unique<AddNode>(); });
//     return true;
//   }();
//
// 效果：在 main() 之前初始化 static bool _reg_AddNode，
//       执行 lambda 向全局 Registry 注册 AddNode 的工厂函数
// ----------

// 简化版，使用 SimpleRegistry
#define MY_REGISTER_TYPE(TypeName)                             \
    static bool _reg_##TypeName = []() {                       \
        SimpleRegistry::Global().Register(                     \
            #TypeName,                                         \
            #TypeName                                          \
        );                                                     \
        return true;                                           \
    }()

// ---- 在全局作用域调用宏，完成注册 ----
MY_REGISTER_TYPE(IntNode);
MY_REGISTER_TYPE(FloatNode);
MY_REGISTER_TYPE(StringNode);

// 这就等价于 6 中的:
// TVM_REGISTER_NODE_TYPE(ConstantNode);
// TVM_REGISTER_NODE_TYPE(AddNode);
// TVM_REGISTER_NODE_TYPE(MulNode);

// ============================================================
// 第7层（Bonus）：# 和 ## 配合使用的复杂例子
// ============================================================

// 生成 getter 函数
#define DEFINE_GETTER(Type, Field)                     \
    const auto& get_##Field(const Type& obj) {         \
        return obj.Field;                              \
    }

struct Point { int x, y; };
DEFINE_GETTER(Point, x)   // → const auto& get_x(const Point& obj) { return obj.x; }
DEFINE_GETTER(Point, y)   // → const auto& get_y(const Point& obj) { return obj.y; }

// ============================================================
// main：验证一切
// ============================================================

int main() {
    layer0();
    layer1();
    layer2();
    layer3();
    layer4();
    layer5();

    std::cout << "\n=== 第6层：MY_REGISTER_TYPE 注册结果验证 ===\n";
    std::cout << "  Has IntNode?   " << SimpleRegistry::Global().Has("IntNode") << "\n";
    std::cout << "  Has FloatNode? " << SimpleRegistry::Global().Has("FloatNode") << "\n";
    std::cout << "  Has StringNode? " << SimpleRegistry::Global().Has("StringNode") << "\n";
    std::cout << "  Has WrongNode? " << SimpleRegistry::Global().Has("WrongNode") << "\n";

    std::cout << "\n=== 第7层：DEFINE_GETTER ===\n";
    Point p{10, 20};
    std::cout << "  get_x(p) = " << get_x(p) << "\n";
    std::cout << "  get_y(p) = " << get_y(p) << "\n";

    std::cout << "\n全部完成！回头看 6 中的宏定义：\n";
    std::cout << "  #define TVM_REGISTER_NODE_TYPE(NodeType) ...\n";
    std::cout << "你应该能看懂每一部分了。\n";

    return 0;
}
