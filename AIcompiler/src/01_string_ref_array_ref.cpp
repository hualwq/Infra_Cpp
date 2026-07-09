#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// StringRef / ArrayRef 的核心：只保存 pointer + length，不拥有数据。
// 真实 LLVM 版本功能更多，这里只保留最常用的读法和生命周期警告。

class StringRef {
    const char *data_ = "";
    std::size_t size_ = 0;

public:
    StringRef() = default;
    StringRef(const char *s) : data_(s), size_(std::strlen(s)) {}
    StringRef(const std::string &s) : data_(s.data()), size_(s.size()) {}
    StringRef(const char *data, std::size_t size) : data_(data), size_(size) {}

    const char *data() const { return data_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    char operator[](std::size_t i) const {
        assert(i < size_);
        return data_[i];
    }

    std::string str() const { return std::string(data_, size_); }

    bool starts_with(StringRef prefix) const {
        if (prefix.size_ > size_) return false;
        return std::memcmp(data_, prefix.data_, prefix.size_) == 0;
    }

    StringRef drop_front(std::size_t n = 1) const {
        if (n > size_) n = size_;
        return StringRef(data_ + n, size_ - n);
    }
};

template <typename T>
class ArrayRef {
    const T *data_ = nullptr;
    std::size_t size_ = 0;

public:
    ArrayRef() = default;
    ArrayRef(const T *data, std::size_t size) : data_(data), size_(size) {}
    ArrayRef(const std::vector<T> &v) : data_(v.data()), size_(v.size()) {}

    const T *begin() const { return data_; }
    const T *end() const { return data_ + size_; }
    const T *data() const { return data_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    const T &operator[](std::size_t i) const {
        assert(i < size_);
        return data_[i];
    }

    ArrayRef<T> drop_front(std::size_t n = 1) const {
        if (n > size_) n = size_;
        return ArrayRef<T>(data_ + n, size_ - n);
    }
};

template <typename T>
void dumpArray(ArrayRef<T> values, StringRef title) {
    std::cout << title.str() << ":";
    for (const T &v : values) {
        std::cout << " " << v;
    }
    std::cout << "\n";
}

void parseOperationName(StringRef name) {
    std::cout << "op name = " << name.str() << "\n";
    if (name.starts_with("toy.")) {
        std::cout << "dialect = toy\n";
        std::cout << "op      = " << name.drop_front(4).str() << "\n";
    }
}

StringRef dangerousReturn() {
    std::string tmp = "local.buffer";
    return StringRef(tmp); // 错误示例：返回后 tmp 已经析构，StringRef 悬空。
}

int main() {
    std::cout << "=== StringRef / ArrayRef ===\n\n";

    std::string owned = "toy.matmul";
    StringRef opName(owned);
    parseOperationName(opName);

    std::cout << "\n-- StringRef 不拥有数据 --\n";
    std::cout << "before mutation: " << opName.str() << "\n";
    owned[4] = 'a';
    std::cout << "after mutation:  " << opName.str()
              << "  (view sees the same buffer)\n";

    std::cout << "\n-- ArrayRef 统一接收 vector / C array --\n";
    std::vector<int> dims = {1, 64, 128};
    int strides[] = {8192, 128, 1};
    dumpArray(ArrayRef<int>(dims), "shape");
    dumpArray(ArrayRef<int>(strides, 3), "strides");
    dumpArray(ArrayRef<int>(dims).drop_front(), "shape without batch");

    std::cout << "\n-- 生命周期规则 --\n";
    std::cout << "dangerousReturn() 编译能过，但返回的是悬空 view，真实代码不要这样写。\n";
    (void)dangerousReturn;

    std::string_view stdView = owned;
    std::cout << "std::string_view 和 StringRef 思想类似: " << stdView << "\n";
    return 0;
}
