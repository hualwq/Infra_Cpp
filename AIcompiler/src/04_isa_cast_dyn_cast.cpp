#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// LLVM 风格 RTTI：
//   isa<T>(x)      -> bool
//   cast<T>(x)     -> 断言式向下转型，失败说明调用者逻辑错
//   dyn_cast<T>(x) -> 尝试向下转型，失败返回 nullptr
//
// 真实 LLVM 通过 classof + 模板重载支持指针、引用、智能指针等多种形式。

class Expr {
public:
    enum class Kind { Constant, Add, Mul };

private:
    Kind kind_;

protected:
    explicit Expr(Kind kind) : kind_(kind) {}

public:
    virtual ~Expr() = default;
    Kind kind() const { return kind_; }
    virtual std::string repr() const = 0;
};

class ConstantExpr : public Expr {
    int value_ = 0;

public:
    explicit ConstantExpr(int value) : Expr(Kind::Constant), value_(value) {}

    int value() const { return value_; }
    std::string repr() const override { return std::to_string(value_); }

    static bool classof(const Expr *expr) {
        return expr->kind() == Kind::Constant;
    }
};

class BinaryExpr : public Expr {
protected:
    std::unique_ptr<Expr> lhs_;
    std::unique_ptr<Expr> rhs_;

    BinaryExpr(Kind kind, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : Expr(kind), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

public:
    const Expr *lhs() const { return lhs_.get(); }
    const Expr *rhs() const { return rhs_.get(); }

    static bool classof(const Expr *expr) {
        return expr->kind() == Kind::Add || expr->kind() == Kind::Mul;
    }
};

class AddExpr : public BinaryExpr {
public:
    AddExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : BinaryExpr(Kind::Add, std::move(lhs), std::move(rhs)) {}

    std::string repr() const override {
        return "(" + lhs_->repr() + " + " + rhs_->repr() + ")";
    }

    static bool classof(const Expr *expr) {
        return expr->kind() == Kind::Add;
    }
};

class MulExpr : public BinaryExpr {
public:
    MulExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : BinaryExpr(Kind::Mul, std::move(lhs), std::move(rhs)) {}

    std::string repr() const override {
        return "(" + lhs_->repr() + " * " + rhs_->repr() + ")";
    }

    static bool classof(const Expr *expr) {
        return expr->kind() == Kind::Mul;
    }
};

template <typename To, typename From>
bool isa(const From *value) {
    return value != nullptr && To::classof(value);
}

template <typename To, typename From>
To *dyn_cast(From *value) {
    return isa<To>(value) ? static_cast<To *>(value) : nullptr;
}

template <typename To, typename From>
const To *dyn_cast(const From *value) {
    return isa<To>(value) ? static_cast<const To *>(value) : nullptr;
}

template <typename To, typename From>
To *cast(From *value) {
    assert(isa<To>(value) && "cast<T>() argument has wrong dynamic type");
    return static_cast<To *>(value);
}

template <typename To, typename From>
const To *cast(const From *value) {
    assert(isa<To>(value) && "cast<T>() argument has wrong dynamic type");
    return static_cast<const To *>(value);
}

std::unique_ptr<Expr> constant(int v) {
    return std::make_unique<ConstantExpr>(v);
}

void inspect(const Expr *expr) {
    std::cout << expr->repr();

    if (const auto *c = dyn_cast<ConstantExpr>(expr)) {
        std::cout << "  => constant value=" << c->value();
    } else if (isa<BinaryExpr>(expr)) {
        std::cout << "  => binary expression";
    }

    std::cout << "\n";
}

int main() {
    std::cout << "=== isa / cast / dyn_cast ===\n\n";

    std::vector<std::unique_ptr<Expr>> exprs;
    exprs.push_back(constant(42));
    exprs.push_back(std::make_unique<AddExpr>(constant(1), constant(2)));
    exprs.push_back(std::make_unique<MulExpr>(constant(3), constant(4)));

    for (const auto &expr : exprs) {
        inspect(expr.get());
    }

    std::cout << "\n-- cast<T> 用在你确信类型正确的时候 --\n";
    const Expr *first = exprs.front().get();
    const auto *c = cast<ConstantExpr>(first);
    std::cout << "cast<ConstantExpr>(first)->value() = " << c->value() << "\n";

    std::cout << "\n-- dyn_cast<T> 用在分支判断 --\n";
    const Expr *second = exprs[1].get();
    if (const auto *maybeConst = dyn_cast<ConstantExpr>(second)) {
        std::cout << maybeConst->value() << "\n";
    } else {
        std::cout << "second is not ConstantExpr, no crash\n";
    }

    return 0;
}
