#include <iostream>
#include <atomic>
#include <string>
#include <typeinfo>
#include <vector>
#include <cassert>

// ============================================================
// 第0层：引用计数基类 Object
// TVM 的 Object 是所有 IR 节点的基类，管理引用计数
// ============================================================
class Object {
public:
    mutable std::atomic<int> ref_count_{0};

    Object() = default;
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    virtual ~Object() = default;

    // 运行时类型标识（TVM 用 TypeIndex 枚举，这里用 type_info）
    virtual const std::type_info& type_info() const = 0;
    virtual Object* copy() const = 0;

    void IncRef() const { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    void DecRef() const {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
    int use_count() const { return ref_count_.load(std::memory_order_relaxed); }
};

// ============================================================
// 第1层：ObjectRef 智能指针包装
// Pimpl 模式：接口（ObjectRef）持有实现（Object*）的指针
// ============================================================
class ObjectRef {
protected:
    Object* obj_{nullptr};

    // 模板 IsInstance：检查底层 Object 是否匹配 RefType 对应的 Object 子类
    // 例如 IsInstance<ConstantRef>() 检查 obj_ 是否是 ConstantObject
    template <typename RefType>
    friend bool IsInstance(const ObjectRef& ref);

public:
    ObjectRef() = default;
    explicit ObjectRef(Object* obj) : obj_(obj) {
        if (obj_) obj_->IncRef();
    }
    ObjectRef(const ObjectRef& other) : obj_(other.obj_) {
        if (obj_) obj_->IncRef();
    }
    ObjectRef(ObjectRef&& other) noexcept : obj_(other.obj_) {
        other.obj_ = nullptr;
    }
    ObjectRef& operator=(const ObjectRef& other) {
        if (this != &other) {
            if (obj_) obj_->DecRef();
            obj_ = other.obj_;
            if (obj_) obj_->IncRef();
        }
        return *this;
    }
    ObjectRef& operator=(ObjectRef&& other) noexcept {
        if (this != &other) {
            if (obj_) obj_->DecRef();
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }
    ~ObjectRef() {
        if (obj_) obj_->DecRef();
    }

    Object* operator->() const { return obj_; }
    Object* get() const { return obj_; }
    explicit operator bool() const { return obj_ != nullptr; }
    int use_count() const { return obj_ ? obj_->use_count() : 0; }

    // 统一类型检查接口
    template <typename RefType>
    bool IsInstance() const {
        if (!obj_) return false;
        // 用 RefType 对应的 Object 子类的 type_info 检查
        return obj_->type_info() == RefType::_object_type_info();
    }
};

// ============================================================
// 第2层：宏 —— 自动生成类型检查和向下转型样板代码
// 模拟 TVM_DEFINE_OBJECT_REF_METHODS
//
// 展开后生成：
//   1. static _object_type_info() 供基类 IsInstance<T> 使用
//   2. operator-> 向下转型为 Object 子类
//   3. IsInstance() 便捷版本
// ============================================================

#define TVM_DEFINE_OBJECT_REF_METHODS(RefType, ObjType, ParentRef)        \
    RefType() = default;                                                   \
    explicit RefType(Object* p) : ParentRef(p) {}                          \
    static const std::type_info& _object_type_info() {                     \
        return typeid(ObjType);                                            \
    }                                                                      \
    ObjType* operator->() const {                                          \
        auto* ptr = dynamic_cast<ObjType*>(this->get());                   \
        assert(ptr != nullptr && #RefType " downcast failed");             \
        return ptr;                                                        \
    }                                                                      \
    bool IsInstance() const {                                              \
        return this->get() != nullptr &&                                   \
               this->get()->type_info() == typeid(ObjType);                \
    }

// ============================================================
// 第3层：实际 IR 节点定义
// 模拟 TVM 的 Expr → Constant / Add
// ============================================================

// ---- 基类 ExprObject ----
class ExprObject : public Object {
public:
    const std::type_info& type_info() const override = 0;
    virtual std::string repr() const = 0;
};

// ---- 基类 ExprRef ----
class ExprRef : public ObjectRef {
public:
    using ObjectRef::ObjectRef;
    ExprRef() = default;

    std::string repr() const {
        assert(obj_ != nullptr);
        return static_cast<const ExprObject*>(obj_)->repr();
    }
};

// ---- Constant ----
class ConstantObject : public ExprObject {
public:
    int value;
    explicit ConstantObject(int v) : value(v) {}
    const std::type_info& type_info() const override { return typeid(ConstantObject); }
    Object* copy() const override { return new ConstantObject(value); }
    std::string repr() const override { return std::to_string(value); }
};

class ConstantRef : public ExprRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(ConstantRef, ConstantObject, ExprRef);

    // 便捷构造：从值创建 Object
    explicit ConstantRef(int v) : ExprRef(new ConstantObject(v)) {}
};

// ---- Add ----
class AddObject : public ExprObject {
public:
    ConstantRef lhs, rhs;
    AddObject(ConstantRef l, ConstantRef r) : lhs(std::move(l)), rhs(std::move(r)) {}
    const std::type_info& type_info() const override { return typeid(AddObject); }
    Object* copy() const override { return new AddObject(lhs, rhs); }
    std::string repr() const override {
        return "(" + lhs.repr() + " + " + rhs.repr() + ")";
    }
};

class AddRef : public ExprRef {
public:
    TVM_DEFINE_OBJECT_REF_METHODS(AddRef, AddObject, ExprRef);

    AddRef(ConstantRef l, ConstantRef r)
        : ExprRef(new AddObject(std::move(l), std::move(r))) {}
};

// ============================================================
// 第4层：使用示例
// ============================================================

int main() {
    std::cout << "=== TVM Object / ObjectRef 系统模拟 ===\n\n";

    // 1. 基本构造
    std::cout << "-- 1. 构造 ConstantRef（内部持有 ConstantObject）--\n";
    ConstantRef a(10), b(20);
    std::cout << "a = " << a.repr() << ", use_count = " << a.use_count() << "\n";
    std::cout << "b = " << b.repr() << ", use_count = " << b.use_count() << "\n";

    // 2. 引用计数：拷贝 Ref 共享同一 Object
    std::cout << "\n-- 2. 拷贝共享 / 引用计数 --\n";
    {
        ConstantRef c = a;
        std::cout << "c = a, a.use_count = " << a.use_count() << "\n";
        std::cout << "c->value = " << c->value << "\n";  // operator-> 向下转型
    }
    std::cout << "after c 析构, a.use_count = " << a.use_count() << "\n";

    // 3. IsInstance 类型检查
    std::cout << "\n-- 3. IsInstance<T>() 类型检查 --\n";
    ExprRef generic = a;
    std::cout << "generic is ConstantRef? " << generic.IsInstance<ConstantRef>() << "\n";
    std::cout << "generic is AddRef?      " << generic.IsInstance<AddRef>() << "\n";

    // 4. 构建 Add 节点
    std::cout << "\n-- 4. 构建 Add IR 节点 --\n";
    AddRef add(a, b);
    std::cout << "add = " << add.repr() << "\n";
    std::cout << "add.IsInstance() = " << add.IsInstance() << "\n";

    // 5. 多态容器
    std::cout << "\n-- 5. 多态容器 vector<ExprRef> --\n";
    std::vector<ExprRef> exprs;
    exprs.push_back(a);
    exprs.push_back(add);
    for (size_t i = 0; i < exprs.size(); i++) {
        std::cout << "  exprs[" << i << "] = " << exprs[i].repr()
                  << ", type=Constant? " << exprs[i].IsInstance<ConstantRef>()
                  << ", type=Add? "      << exprs[i].IsInstance<AddRef>()
                  << "\n";
    }

    // 6. Pimpl 效果展示
    std::cout << "\n-- 6. Pimpl 验证：Ref 拷贝只复制指针，不复制 Object --\n";
    ConstantRef d = a;
    std::cout << "a.get() = " << a.get() << "\n";
    std::cout << "d.get() = " << d.get() << "  (same pointer, shared Object)\n";
    std::cout << "a.use_count() = " << a.use_count() << "  (ref_count=2)\n";

    // 7. 宏展开预览
    std::cout << "\n-- 7. TVM_DEFINE_OBJECT_REF_METHODS 宏展开 --\n";
    std::cout << R"(
TVM_DEFINE_OBJECT_REF_METHODS(ConstantRef, ConstantObject, ExprRef)
展开为：

    ConstantRef() = default;
    explicit ConstantRef(Object* p) : ExprRef(p) {}

    static const std::type_info& _object_type_info() {
        return typeid(ConstantObject);
    }

    ConstantObject* operator->() const {
        auto* ptr = dynamic_cast<ConstantObject*>(this->get());
        assert(ptr != nullptr && "ConstantRef downcast failed");
        return ptr;
    }

    bool IsInstance() const {
        return this->get() != nullptr &&
               this->get()->type_info() == typeid(ConstantObject);
    }
)";

    return 0;
}
