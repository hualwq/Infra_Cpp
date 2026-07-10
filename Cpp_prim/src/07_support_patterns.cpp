#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// 这一章放几个 LLVM Support 里非常常见、但容易误用的模式：
//   function_ref: 非拥有 callable view
//   Twine:        临时字符串拼接表达式
//   Expected<T>:  返回值或错误，调用方必须显式处理

class FunctionRefInt {
    void *callback_ = nullptr;
    int (*call_)(void *, int) = nullptr;

public:
    FunctionRefInt() = default;

    template <typename Callable>
    FunctionRefInt(Callable &callable)
        : callback_(static_cast<void *>(&callable)),
          call_([](void *cb, int value) {
              return (*static_cast<Callable *>(cb))(value);
          }) {}

    int operator()(int value) const {
        return call_(callback_, value);
    }

    explicit operator bool() const {
        return call_ != nullptr;
    }
};

void walkNumbers(const std::vector<int> &numbers, FunctionRefInt visitor) {
    for (int n : numbers) {
        std::cout << visitor(n) << " ";
    }
    std::cout << "\n";
}

class TwineLike {
    std::string_view lhs_;
    std::string_view rhs_;

public:
    TwineLike(std::string_view lhs, std::string_view rhs) : lhs_(lhs), rhs_(rhs) {}

    std::string str() const {
        std::string out;
        out.reserve(lhs_.size() + rhs_.size());
        out.append(lhs_);
        out.append(rhs_);
        return out;
    }
};

template <typename T>
class Expected {
    std::optional<T> value_;
    std::string error_;

public:
    Expected(T value) : value_(std::move(value)) {}
    Expected(std::string error) : error_(std::move(error)) {}

    explicit operator bool() const {
        return value_.has_value();
    }

    T &get() {
        return *value_;
    }

    const std::string &error() const {
        return error_;
    }
};

Expected<int> parsePositiveInt(std::string_view text) {
    if (text.empty()) {
        return std::string("empty input");
    }

    int value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return std::string("non-digit character: ") + c;
        }
        value = value * 10 + (c - '0');
    }

    if (value == 0) {
        return std::string("expected positive integer");
    }
    return value;
}

int main() {
    std::cout << "=== LLVM Support patterns ===\n\n";

    std::cout << "-- function_ref: 非拥有回调 view --\n";
    std::vector<int> numbers = {1, 2, 3};
    int scale = 10;
    auto multiply = [&](int x) { return x * scale; };
    walkNumbers(numbers, multiply);
    std::cout << "FunctionRefInt 不拥有 lambda，所以不能比 multiply 活得更久。\n";

    std::cout << "\n-- Twine: 临时字符串拼接 --\n";
    std::string dialect = "toy.";
    std::string op = "matmul";
    TwineLike fullName(dialect, op);
    std::cout << fullName.str() << "\n";
    std::cout << "TwineLike 保存 string_view，只适合立即 .str()，不适合长期存储。\n";

    std::cout << "\n-- Expected<T>: 值或错误 --\n";
    for (std::string_view input : {"42", "0", "12x"}) {
        Expected<int> parsed = parsePositiveInt(input);
        if (parsed) {
            std::cout << input << " -> " << parsed.get() << "\n";
        } else {
            std::cout << input << " -> error: " << parsed.error() << "\n";
        }
    }

    return 0;
}
