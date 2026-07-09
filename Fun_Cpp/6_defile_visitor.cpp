#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>

// ============================================================
// 第0层：IR 节点基类（复用上一章的简化版本）
// ============================================================
class ExprNode {
public:
    virtual ~ExprNode() = default;
    virtual std::string repr() const = 0;
    // 每个子类需返回自己的类型名（用于注册表查找）
    virtual const char* type_key() const = 0;
    // 自我克隆（供 IRMutator 使用）
    virtual std::unique_ptr<ExprNode> clone() const = 0;
};

// ============================================================
// 第1层：具体 IR 节点
// ============================================================

class ConstantNode : public ExprNode {
public:
    int value{0};
    ConstantNode() = default;
    explicit ConstantNode(int v) : value(v) {}
    std::string repr() const override { return std::to_string(value); }
    const char* type_key() const override { return "ConstantNode"; }
    std::unique_ptr<ExprNode> clone() const override {
        return std::make_unique<ConstantNode>(value);
    }
};

class AddNode : public ExprNode {
public:
    std::unique_ptr<ExprNode> lhs, rhs;
    AddNode() = default;
    AddNode(std::unique_ptr<ExprNode> l, std::unique_ptr<ExprNode> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
    std::string repr() const override {
        return "(" + lhs->repr() + " + " + rhs->repr() + ")";
    }
    const char* type_key() const override { return "AddNode"; }
    std::unique_ptr<ExprNode> clone() const override {
        return std::make_unique<AddNode>(lhs->clone(), rhs->clone());
    }
};

class MulNode : public ExprNode {
public:
    std::unique_ptr<ExprNode> lhs, rhs;
    MulNode() = default;
    MulNode(std::unique_ptr<ExprNode> l, std::unique_ptr<ExprNode> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
    std::string repr() const override {
        return "(" + lhs->repr() + " * " + rhs->repr() + ")";
    }
    const char* type_key() const override { return "MulNode"; }
    std::unique_ptr<ExprNode> clone() const override {
        return std::make_unique<MulNode>(lhs->clone(), rhs->clone());
    }
};

// ============================================================
// 第2层：Registry —— 全局注册表 / 反射机制
// 模拟 TVM_REGISTER_NODE_TYPE / TVM_REGISTER_PASS
// ============================================================

// 工厂函数类型：无参构造 unique_ptr<ExprNode>
// 注意：TVM 实际更复杂（通过 TVMArgs 传参），这里简化
using NodeFactory = std::function<std::unique_ptr<ExprNode>()>;

class Registry {
    std::unordered_map<std::string, NodeFactory> factories_;
    Registry() = default;

public:
    static Registry& Global() {
        static Registry inst;
        return inst;
    }

    // 注册一个节点类型的工厂
    Registry& Register(const std::string& key, NodeFactory f) {
        factories_[key] = std::move(f);
        return *this;
    }

    // 运行时根据类型名创建节点
    std::unique_ptr<ExprNode> Create(const std::string& key) const {
        auto it = factories_.find(key);
        if (it == factories_.end()) {
            std::cerr << "[Registry] unknown type: " << key << "\n";
            return nullptr;
        }
        return it->second();
    }

    bool Has(const std::string& key) const {
        return factories_.count(key) > 0;
    }

    void ListAll() const {
        std::cout << "[Registry] registered types:\n";
        for (const auto& [k, _] : factories_)
            std::cout << "  " << k << "\n";
    }
};

// ---- 宏：自动注册节点类型 ----
// TVM_REGISTER_NODE_TYPE(ConstantNode) 展开后：
//   在文件加载时自动向全局 Registry 注册一个工厂
#define TVM_REGISTER_NODE_TYPE(NodeType)                         \
    static bool _reg_##NodeType = []() {                         \
        Registry::Global().Register(                             \
            #NodeType,                                           \
            []() { return std::make_unique<NodeType>(); }        \
        );                                                       \
        return true;                                             \
    }();

// ---- 注册各节点类型 ----
TVM_REGISTER_NODE_TYPE(ConstantNode);
TVM_REGISTER_NODE_TYPE(AddNode);
TVM_REGISTER_NODE_TYPE(MulNode);

// ---- 类似 TVM_REGISTER_PASS：注册一个 Pass 名 ----
using PassFunc = std::function<std::unique_ptr<ExprNode>(std::unique_ptr<ExprNode>)>;

class PassRegistry {
    std::unordered_map<std::string, PassFunc> passes_;
    PassRegistry() = default;

public:
    static PassRegistry& Global() {
        static PassRegistry inst;
        return inst;
    }

    PassRegistry& Register(const std::string& name, PassFunc f) {
        passes_[name] = std::move(f);
        return *this;
    }

    std::unique_ptr<ExprNode> Run(const std::string& name,
                                  std::unique_ptr<ExprNode> ir) const {
        auto it = passes_.find(name);
        if (it == passes_.end()) {
            std::cerr << "[PassRegistry] unknown pass: " << name << "\n";
            return ir;
        }
        return it->second(std::move(ir));
    }
};

#define TVM_REGISTER_PASS(PassName)                              \
    static bool _pass_##PassName = []() {                        \
        return true;                                              \
    }();                                                          \
    /* 使用方式见下方实际 Pass 定义 */                            \
    /* 这里简化：Register 在具体实现函数中手动调用 */

// ============================================================
// 第3层：Visitor —— IRVisitor（只读遍历）
// ============================================================

class IRVisitor {
public:
    virtual ~IRVisitor() = default;

    // 统一的访问入口 — 根据运行时类型分发
    void Visit(ExprNode* node) {
        if (!node) return;
        if (auto* c = dynamic_cast<ConstantNode*>(node)) {
            VisitConstant_(c);
        } else if (auto* a = dynamic_cast<AddNode*>(node)) {
            VisitAdd_(a);
        } else if (auto* m = dynamic_cast<MulNode*>(node)) {
            VisitMul_(m);
        } else {
            std::cerr << "[Visitor] unknown node type\n";
        }
    }

    // 每个节点类的 Visit 虚函数，子类可重写
    // 注意 TVM 这里用多级 CRTP + 宏避免重复写 dynamic_cast，
    // 这里简化展示核心逻辑
    virtual void VisitConstant_(ConstantNode* op) { DefaultVisit_(op); }
    virtual void VisitAdd_(AddNode* op)           { DefaultVisit_(op); }
    virtual void VisitMul_(MulNode* op)           { DefaultVisit_(op); }

    // 默认行为：递归访问子节点（如果有的话）
    virtual void DefaultVisit_(ExprNode* node) {
        if (auto* a = dynamic_cast<AddNode*>(node)) {
            Visit(a->lhs.get());
            Visit(a->rhs.get());
        } else if (auto* m = dynamic_cast<MulNode*>(node)) {
            Visit(m->lhs.get());
            Visit(m->rhs.get());
        }
        // leaf（ConstantNode）没有子节点，不做任何事
    }
};

// ---- 具体 Visitor：打印树的深度 + 类型 ----
class PrintVisitor : public IRVisitor {
    int depth_ = 0;
public:
    void VisitConstant_(ConstantNode* op) override {
        std::cout << std::string(depth_ * 2, ' ') << "Const[" << op->value << "]\n";
    }
    void VisitAdd_(AddNode* op) override {
        std::cout << std::string(depth_ * 2, ' ') << "Add:\n";
        ++depth_; Visit(op->lhs.get()); --depth_;
        ++depth_; Visit(op->rhs.get()); --depth_;
    }
    void VisitMul_(MulNode* op) override {
        std::cout << std::string(depth_ * 2, ' ') << "Mul:\n";
        ++depth_; Visit(op->lhs.get()); --depth_;
        ++depth_; Visit(op->rhs.get()); --depth_;
    }
};

// ============================================================
// 第4层：Mutator —— IRMutator（遍历并返回新节点）
// 关键区别：返回值，不修改原树，每条 Visit 都返回新节点
// ============================================================

class IRMutator {
public:
    virtual ~IRMutator() = default;

    // 统一入口
    std::unique_ptr<ExprNode> Visit(std::unique_ptr<ExprNode> node) {
        if (!node) return nullptr;
        if (dynamic_cast<ConstantNode*>(node.get())) {
            return VisitConstant_(std::move(node));
        } else if (dynamic_cast<AddNode*>(node.get())) {
            return VisitAdd_(std::move(node));
        } else if (dynamic_cast<MulNode*>(node.get())) {
            return VisitMul_(std::move(node));
        }
        std::cerr << "[Mutator] unknown node type\n";
        return node;
    }

    virtual std::unique_ptr<ExprNode> VisitConstant_(std::unique_ptr<ExprNode> op) {
        return op;  // leaf，原样返回
    }

    virtual std::unique_ptr<ExprNode> VisitAdd_(std::unique_ptr<ExprNode> op) {
        auto* node = static_cast<AddNode*>(op.get());
        auto new_lhs = Visit(std::move(node->lhs));
        auto new_rhs = Visit(std::move(node->rhs));
        // 只有子节点变化时才创建新节点（immutable 优化）
        if (new_lhs.get() == node->lhs.get() &&
            new_rhs.get() == node->rhs.get()) {
            return op;  // 没变化，复用原节点
        }
        return std::make_unique<AddNode>(std::move(new_lhs), std::move(new_rhs));
    }

    virtual std::unique_ptr<ExprNode> VisitMul_(std::unique_ptr<ExprNode> op) {
        auto* node = static_cast<MulNode*>(op.get());
        auto new_lhs = Visit(std::move(node->lhs));
        auto new_rhs = Visit(std::move(node->rhs));
        if (new_lhs.get() == node->lhs.get() &&
            new_rhs.get() == node->rhs.get()) {
            return op;
        }
        return std::make_unique<MulNode>(std::move(new_lhs), std::move(new_rhs));
    }
};

// ---- 具体 Mutator：常量折叠 (x + 0) → x, (x * 1) → x ----
class FoldConstantMutator : public IRMutator {
public:
    std::unique_ptr<ExprNode> VisitAdd_(std::unique_ptr<ExprNode> op) override {
        auto* node = static_cast<AddNode*>(op.get());
        auto new_lhs = Visit(std::move(node->lhs));
        auto new_rhs = Visit(std::move(node->rhs));

        // 检查是否是 x + 0
        if (auto* rc = dynamic_cast<ConstantNode*>(new_rhs.get())) {
            if (rc->value == 0) return new_lhs;  // x + 0 → x
        }
        if (auto* lc = dynamic_cast<ConstantNode*>(new_lhs.get())) {
            if (lc->value == 0) return new_rhs;  // 0 + x → x
        }

        // 两个常量直接计算
        if (auto* lc = dynamic_cast<ConstantNode*>(new_lhs.get())) {
            if (auto* rc = dynamic_cast<ConstantNode*>(new_rhs.get())) {
                return std::make_unique<ConstantNode>(lc->value + rc->value);
            }
        }

        return std::make_unique<AddNode>(std::move(new_lhs), std::move(new_rhs));
    }

    std::unique_ptr<ExprNode> VisitMul_(std::unique_ptr<ExprNode> op) override {
        auto* node = static_cast<MulNode*>(op.get());
        auto new_lhs = Visit(std::move(node->lhs));
        auto new_rhs = Visit(std::move(node->rhs));

        // x * 1 → x
        if (auto* rc = dynamic_cast<ConstantNode*>(new_rhs.get())) {
            if (rc->value == 1) return new_lhs;
        }
        if (auto* lc = dynamic_cast<ConstantNode*>(new_lhs.get())) {
            if (lc->value == 1) return new_rhs;
        }

        // 常量折叠
        if (auto* lc = dynamic_cast<ConstantNode*>(new_lhs.get())) {
            if (auto* rc = dynamic_cast<ConstantNode*>(new_rhs.get())) {
                return std::make_unique<ConstantNode>(lc->value * rc->value);
            }
        }

        return std::make_unique<MulNode>(std::move(new_lhs), std::move(new_rhs));
    }
};

// ---- 另一个 Pass：用 TVM_REGISTER_PASS 风格注册 ----
// 实际注册到 PassRegistry
static bool _reg_fold_constants = []() {
    PassRegistry::Global().Register("FoldConstants", [](std::unique_ptr<ExprNode> ir) {
        FoldConstantMutator mutator;
        return mutator.Visit(std::move(ir));
    });
    return true;
}();

static bool _reg_print_ir = []() {
    PassRegistry::Global().Register("PrintIR", [](std::unique_ptr<ExprNode> ir) {
        PrintVisitor visitor;
        std::cout << "[Pass::PrintIR]\n";
        visitor.Visit(ir.get());
        return ir;
    });
    return true;
}();

// ============================================================
// 第5层：使用示例
// ============================================================

int main() {
    std::cout << "=== 1. Registry：注册表与反射 ===\n";
    Registry::Global().ListAll();

    std::cout << "\n--- 运行时创建节点 ---\n";
    auto c = Registry::Global().Create("ConstantNode");
    if (c) {
        static_cast<ConstantNode*>(c.get())->value = 42;
        std::cout << "  created: " << c->repr() << "\n";
    }

    std::cout << "\n=== 2. IRVisitor：只读遍历 ===\n";
    // 构建树: (1 + 2) * (3 + 0)
    auto tree = std::make_unique<MulNode>(
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(1),
            std::make_unique<ConstantNode>(2)
        ),
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(3),
            std::make_unique<ConstantNode>(0)
        )
    );
    std::cout << "  tree = " << tree->repr() << "\n\n";

    std::cout << "--- PrintVisitor 遍历 ---\n";
    PrintVisitor printer;
    printer.Visit(tree.get());

    std::cout << "\n=== 3. IRMutator：常量折叠 ===\n";
    std::cout << "  before: " << tree->repr() << "\n";
    FoldConstantMutator fcm;
    auto folded = fcm.Visit(std::move(tree));
    std::cout << "  after:  " << folded->repr() << "  (3 + 0 → 3, 再与 x 无关)\n";

    // 更彻底的折叠：(1 + 2) * (3 + 0) → 3 * 3 → 9
    std::cout << "\n=== 4. 多层折叠 ===\n";
    auto tree2 = std::make_unique<MulNode>(
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(1),
            std::make_unique<ConstantNode>(2)
        ),
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(3),
            std::make_unique<ConstantNode>(0)
        )
    );
    std::cout << "  before: " << tree2->repr() << "\n";
    // 运行两次 Mutator 让折叠传到底（TVM 的 CanonicalSimplify 会循环到不动点）
    FoldConstantMutator fcm2;
    auto folded2 = fcm2.Visit(std::move(tree2));
    folded2 = fcm2.Visit(std::move(folded2));
    std::cout << "  after 2 passes: " << folded2->repr() << "\n";

    std::cout << "\n=== 5. PassRegistry：按名字运行 Pass ===\n";
    auto tree3 = std::unique_ptr<ExprNode>(std::make_unique<MulNode>(
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(1),
            std::make_unique<ConstantNode>(2)
        ),
        std::make_unique<AddNode>(
            std::make_unique<ConstantNode>(3),
            std::make_unique<ConstantNode>(0)
        )
    ));
    tree3 = PassRegistry::Global().Run("PrintIR", std::move(tree3));
    // 运行 FoldConstants pass
    auto result = PassRegistry::Global().Run("FoldConstants", std::move(tree3));
    std::cout << "  FoldConstants result: " << result->repr() << "\n";

    return 0;
}

// 一些问题：

/*
1. using 和 #define 的区别， #define 是纯粹的字符串替换，不做类型检查
2. std::function<std::unique_ptr<ExprNode>(std::unique_ptr<ExprNode>)> 尖括号里面是 function 的返回值，圆括号里面是传入参数
3. 下面这段代码是链式调用（Method Chaining）的设计模式。返回自身的引用后，可以这样连续注册
PassRegistry& Register(const std::string& name, PassFunc f) {
    passes_[name] = std::move(f);
    return *this;  // 返回当前对象的引用
}

例如：
PassRegistry()
    .Register("fold", somePass)
    .Register("propagate", anotherPass)
    .Register("eliminate", yetAnotherPass);

*/