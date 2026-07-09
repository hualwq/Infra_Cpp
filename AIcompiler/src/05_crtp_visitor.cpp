#include <iostream>
#include <memory>
#include <string>
#include <utility>

// CRTP: Curiously Recurring Template Pattern
// 基类模板拿到 Derived 类型，然后用 static_cast<Derived *>(this)
// 在编译期调用派生类能力。LLVM/MLIR 中 visitor、pass、类型工具经常用这种模式。

class Constant;
class Add;
class Mul;

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
};

class Constant : public Expr {
public:
    int value = 0;

    explicit Constant(int v) : Expr(Kind::Constant), value(v) {}
};

class Binary : public Expr {
public:
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    Binary(Kind kind, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Expr(kind), lhs(std::move(l)), rhs(std::move(r)) {}
};

class Add : public Binary {
public:
    Add(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Binary(Kind::Add, std::move(l), std::move(r)) {}
};

class Mul : public Binary {
public:
    Mul(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Binary(Kind::Mul, std::move(l), std::move(r)) {}
};

template <typename Derived>
class ExprVisitor {
public:
    void visit(Expr *expr) {
        switch (expr->kind()) {
        case Expr::Kind::Constant:
            derived().visitConstant(static_cast<Constant *>(expr));
            break;
        case Expr::Kind::Add:
            derived().visitAdd(static_cast<Add *>(expr));
            break;
        case Expr::Kind::Mul:
            derived().visitMul(static_cast<Mul *>(expr));
            break;
        }
    }

    void visitConstant(Constant *) {}

    void visitAdd(Add *op) {
        walkBinary(op);
    }

    void visitMul(Mul *op) {
        walkBinary(op);
    }

protected:
    Derived &derived() {
        return *static_cast<Derived *>(this);
    }

    void walkBinary(Binary *op) {
        derived().visit(op->lhs.get());
        derived().visit(op->rhs.get());
    }
};

class PrintVisitor : public ExprVisitor<PrintVisitor> {
    int depth_ = 0;

    std::string indent() const {
        return std::string(depth_ * 2, ' ');
    }

public:
    void visitConstant(Constant *op) {
        std::cout << indent() << "constant " << op->value << "\n";
    }

    void visitAdd(Add *op) {
        std::cout << indent() << "add\n";
        ++depth_;
        walkBinary(op);
        --depth_;
    }

    void visitMul(Mul *op) {
        std::cout << indent() << "mul\n";
        ++depth_;
        walkBinary(op);
        --depth_;
    }
};

class CostVisitor : public ExprVisitor<CostVisitor> {
public:
    int cost = 0;

    void visitConstant(Constant *) {
        cost += 1;
    }

    void visitAdd(Add *op) {
        cost += 2;
        walkBinary(op);
    }

    void visitMul(Mul *op) {
        cost += 4;
        walkBinary(op);
    }
};

std::unique_ptr<Expr> c(int v) {
    return std::make_unique<Constant>(v);
}

int main() {
    std::cout << "=== CRTP Visitor ===\n\n";

    auto expr = std::make_unique<Mul>(
        std::make_unique<Add>(c(1), c(2)),
        std::make_unique<Add>(c(3), c(4)));

    PrintVisitor printer;
    printer.visit(expr.get());

    CostVisitor cost;
    cost.visit(expr.get());
    std::cout << "\nestimated cost = " << cost.cost << "\n";

    std::cout << "\nCRTP visitor 的效果：基类提供统一分发和默认递归，派生类只覆盖关心的节点。\n";
    return 0;
}
