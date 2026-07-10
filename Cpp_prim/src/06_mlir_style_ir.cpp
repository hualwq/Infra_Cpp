#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// 一个很小的 MLIR 风格 IR 模型：
//   Context 负责 uniquing Type/Attribute
//   Operation 拥有 operands/results/attributes
//   Value 是 SSA handle，指向定义它的 Operation + result index
//   Block/Region 拥有 Operation 列表

class Context {
    std::unordered_map<std::string, std::unique_ptr<std::string>> interned_;

public:
    const std::string *intern(std::string text) {
        auto it = interned_.find(text);
        if (it != interned_.end()) return it->second.get();

        auto owned = std::make_unique<std::string>(std::move(text));
        const std::string *ptr = owned.get();
        interned_.emplace(*ptr, std::move(owned));
        return ptr;
    }
};

class Type {
    const std::string *name_ = nullptr;

public:
    Type() = default;
    explicit Type(const std::string *name) : name_(name) {}

    std::string str() const { return *name_; }
    bool operator==(Type other) const { return name_ == other.name_; }
};

class Attribute {
    const std::string *value_ = nullptr;

public:
    Attribute() = default;
    explicit Attribute(const std::string *value) : value_(value) {}

    std::string str() const { return *value_; }
};

class Operation;

struct Value {
    Operation *owner = nullptr;
    std::size_t resultIndex = 0;
    Type type;

    std::string name() const;
};

struct NamedAttribute {
    std::string name;
    Attribute value;
};

class Operation {
    std::string name_;
    std::vector<Value> operands_;
    std::vector<Value> results_;
    std::vector<NamedAttribute> attrs_;

public:
    Operation(std::string name,
              std::vector<Value> operands,
              std::vector<Type> resultTypes,
              std::vector<NamedAttribute> attrs)
        : name_(std::move(name)), operands_(std::move(operands)), attrs_(std::move(attrs)) {
        for (std::size_t i = 0; i < resultTypes.size(); ++i) {
            results_.push_back(Value{this, i, resultTypes[i]});
        }
    }

    const std::string &name() const { return name_; }
    const std::vector<Value> &operands() const { return operands_; }
    const std::vector<Value> &results() const { return results_; }
    const std::vector<NamedAttribute> &attrs() const { return attrs_; }

    Value result(std::size_t i = 0) {
        assert(i < results_.size());
        return results_[i];
    }

    void dump() const {
        if (!results_.empty()) {
            for (std::size_t i = 0; i < results_.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << results_[i].name();
            }
            std::cout << " = ";
        }

        std::cout << '"' << name_ << '"';

        if (!operands_.empty()) {
            std::cout << "(";
            for (std::size_t i = 0; i < operands_.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << operands_[i].name();
            }
            std::cout << ")";
        }

        if (!attrs_.empty()) {
            std::cout << " {";
            for (std::size_t i = 0; i < attrs_.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << attrs_[i].name << " = " << attrs_[i].value.str();
            }
            std::cout << "}";
        }

        if (!results_.empty()) {
            std::cout << " : ";
            for (std::size_t i = 0; i < results_.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << results_[i].type.str();
            }
        }

        std::cout << "\n";
    }
};

std::string Value::name() const {
    std::ostringstream os;
    os << "%";
    if (owner) {
        os << owner->name() << "." << resultIndex;
    } else {
        os << "arg" << resultIndex;
    }
    return os.str();
}

class Block {
    std::vector<Value> arguments_;
    std::vector<std::unique_ptr<Operation>> ops_;

public:
    Value addArgument(Type type) {
        Value arg{nullptr, arguments_.size(), type};
        arguments_.push_back(arg);
        return arg;
    }

    Operation *addOperation(std::unique_ptr<Operation> op) {
        Operation *raw = op.get();
        ops_.push_back(std::move(op));
        return raw;
    }

    const std::vector<std::unique_ptr<Operation>> &operations() const {
        return ops_;
    }

    void dump() const {
        std::cout << "^bb0(";
        for (std::size_t i = 0; i < arguments_.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << arguments_[i].name() << ": " << arguments_[i].type.str();
        }
        std::cout << "):\n";

        for (const auto &op : ops_) {
            std::cout << "  ";
            op->dump();
        }
    }
};

class OpBuilder {
    Context &ctx_;
    Block &block_;

public:
    OpBuilder(Context &ctx, Block &block) : ctx_(ctx), block_(block) {}

    Type getType(std::string name) {
        return Type(ctx_.intern(std::move(name)));
    }

    Attribute getAttr(std::string value) {
        return Attribute(ctx_.intern(std::move(value)));
    }

    Operation *create(std::string name,
                      std::vector<Value> operands,
                      std::vector<Type> resultTypes,
                      std::vector<NamedAttribute> attrs = {}) {
        return block_.addOperation(std::make_unique<Operation>(
            std::move(name), std::move(operands), std::move(resultTypes), std::move(attrs)));
    }
};

int main() {
    std::cout << "=== MLIR-style IR skeleton ===\n\n";

    // context全局共享，因为不同的op可以共享相同的SSA value，通俗说就是 opA 和 opB 操作的矩阵维度一样。
    Context ctx;

    // Block 和IR中的Block一样的作用，表示一个容器
    Block entry;
    OpBuilder builder(ctx, entry);

    Type tensor = builder.getType("tensor<4x4xf32>");
    Type scalar = builder.getType("f32");

    Value lhs = entry.addArgument(tensor);
    Value rhs = entry.addArgument(tensor);

    Operation *matmul = builder.create(
        "toy.matmul",
        {lhs, rhs},
        {tensor},
        {{"transpose_lhs", builder.getAttr("false")}});

    Operation *sum = builder.create(
        "toy.reduce_sum",
        {matmul->result()},
        {scalar},
        {{"axis", builder.getAttr("1")}});

    builder.create("toy.return", {sum->result()}, {});

    entry.dump();

    std::cout << "\n-- Type uniquing --\n";
    Type tensorAgain = builder.getType("tensor<4x4xf32>");
    std::cout << "tensor == tensorAgain ? " << (tensor == tensorAgain) << "\n";

    std::cout << "\n-- 读 MLIR 时的三个问题 --\n";
    std::cout << "1. Operation 拥有哪些 Value / Attribute / Region？\n";
    std::cout << "2. Value 是谁定义的？被哪些 Operation 使用？\n";
    std::cout << "3. Type/Attribute 是值语义 handle，底层通常由 Context uniquing。\n";

    return 0;
}
