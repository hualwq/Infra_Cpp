#include <cassert>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// 一个教学版 SmallVector：前 N 个元素存在对象内部，超过 N 后再搬到堆上。
// 真实 llvm::SmallVector 更完整，支持更多异常安全和算法接口。

template <typename T, std::size_t InlineN>
class SmallVector {
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

    std::size_t size_ = 0;
    std::size_t capacity_ = InlineN;
    T *data_ = inlineData();
    Storage inline_[InlineN == 0 ? 1 : InlineN];

    T *inlineData() {
        return reinterpret_cast<T *>(inline_);
    }

    const T *inlineData() const {
        return reinterpret_cast<const T *>(inline_);
    }

    bool isSmall() const {
        return data_ == inlineData();
    }

    void grow() {
        std::size_t newCapacity = capacity_ == 0 ? 1 : capacity_ * 2;
        T *newData = static_cast<T *>(::operator new(sizeof(T) * newCapacity));

        for (std::size_t i = 0; i < size_; ++i) {
            new (newData + i) T(std::move(data_[i]));
            data_[i].~T();
        }

        if (!isSmall()) {
            ::operator delete(data_);
        }

        data_ = newData;
        capacity_ = newCapacity;
    }

public:
    SmallVector() = default;

    SmallVector(const SmallVector &) = delete;
    SmallVector &operator=(const SmallVector &) = delete;

    ~SmallVector() {
        clear();
        if (!isSmall()) {
            ::operator delete(data_);
        }
    }

    void push_back(const T &value) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(value);
        ++size_;
    }

    void push_back(T &&value) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(std::move(value));
        ++size_;
    }

    template <typename... Args>
    T &emplace_back(Args &&...args) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(std::forward<Args>(args)...);
        return data_[size_++];
    }

    void clear() {
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }

    T &operator[](std::size_t i) {
        assert(i < size_);
        return data_[i];
    }

    const T &operator[](std::size_t i) const {
        assert(i < size_);
        return data_[i];
    }

    T *begin() { return data_; }
    T *end() { return data_ + size_; }
    const T *begin() const { return data_; }
    const T *end() const { return data_ + size_; }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool usingInlineStorage() const { return isSmall(); }
};

struct Operand {
    std::string name;

    explicit Operand(std::string n) : name(std::move(n)) {
        std::cout << "construct " << name << "\n";
    }

    Operand(Operand &&other) noexcept : name(std::move(other.name)) {
        std::cout << "move\n";
    }

    Operand(const Operand &other) : name(other.name) {
        std::cout << "copy " << name << "\n";
    }
};

void dump(const SmallVector<Operand, 4> &operands) {
    std::cout << "size=" << operands.size()
              << ", capacity=" << operands.capacity()
              << ", inline=" << operands.usingInlineStorage()
              << ", values=";
    for (const Operand &op : operands) {
        std::cout << op.name << " ";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "=== SmallVector ===\n\n";

    SmallVector<Operand, 4> operands;
    operands.emplace_back("%a");
    operands.emplace_back("%b");
    operands.emplace_back("%c");
    dump(operands);

    std::cout << "\n-- 第 4 个元素仍在 inline storage，第 5 个触发 grow --\n";
    operands.emplace_back("%d");
    dump(operands);
    operands.emplace_back("%e");
    dump(operands);

    std::cout << "\n-- 和 std::vector 的直觉对比 --\n";
    std::cout << "sizeof(std::vector<Operand>)       = "
              << sizeof(std::vector<Operand>) << "\n";
    std::cout << "sizeof(SmallVector<Operand, 4>)    = "
              << sizeof(SmallVector<Operand, 4>) << "\n";
    std::cout << "SmallVector 对象本身更大，但小规模时可以避免堆分配。\n";

    return 0;
}
