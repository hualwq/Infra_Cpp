#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// 教学版 BumpPtrAllocator：只向前 bump 指针，不支持单个对象 delete。
// 适合 AST/IR/字符串驻留这类“一批对象一起释放”的场景。

class BumpPtrAllocator {
    static constexpr std::size_t BlockSize = 4096;

    struct Block {
        std::unique_ptr<char[]> bytes;
        std::size_t used = 0;

        explicit Block(std::size_t size) : bytes(std::make_unique<char[]>(size)) {}
    };

    std::vector<Block> blocks_;

    static std::size_t alignTo(std::size_t value, std::size_t align) {
        return (value + align - 1) & ~(align - 1);
    }

public:
    void *allocate(std::size_t bytes, std::size_t align = alignof(std::max_align_t)) {
        assert((align & (align - 1)) == 0 && "alignment must be power of two");

        if (blocks_.empty() || alignTo(blocks_.back().used, align) + bytes > BlockSize) {
            std::size_t newBlockSize = bytes > BlockSize ? bytes : BlockSize;
            blocks_.emplace_back(newBlockSize);
        }

        Block &block = blocks_.back();
        std::size_t offset = alignTo(block.used, align);
        block.used = offset + bytes;
        return block.bytes.get() + offset;
    }

    template <typename T, typename... Args>
    T *make(Args &&...args) {
        void *mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    std::size_t blockCount() const { return blocks_.size(); }

    void reset() {
        blocks_.clear();
    }
};

class StringSaver {
    BumpPtrAllocator &alloc_;

public:
    explicit StringSaver(BumpPtrAllocator &alloc) : alloc_(alloc) {}

    const char *save(const std::string &s) {
        char *mem = static_cast<char *>(alloc_.allocate(s.size() + 1, alignof(char)));
        std::memcpy(mem, s.c_str(), s.size() + 1);
        return mem;
    }
};

struct Type {
    const char *name;

    explicit Type(const char *n) : name(n) {}
};

struct Operation {
    const char *name;
    Type *resultType = nullptr;

    Operation(const char *n, Type *t) : name(n), resultType(t) {}

    void dump() const {
        std::cout << name << " : " << resultType->name << "\n";
    }
};

int main() {
    std::cout << "=== BumpPtrAllocator ===\n\n";

    BumpPtrAllocator arena;
    StringSaver saver(arena);

    const char *i32 = saver.save("i32");
    const char *f32 = saver.save("f32");
    Type *i32Type = arena.make<Type>(i32);
    Type *f32Type = arena.make<Type>(f32);

    Operation *c0 = arena.make<Operation>(saver.save("toy.constant"), i32Type);
    Operation *add = arena.make<Operation>(saver.save("toy.add"), f32Type);

    c0->dump();
    add->dump();
    std::cout << "block count = " << arena.blockCount() << "\n";

    std::cout << "\n-- 关键取舍 --\n";
    std::cout << "1. 分配很快：只是指针向前移动。\n";
    std::cout << "2. 不单独 delete：arena reset 时整批释放。\n";
    std::cout << "3. placement new 构造对象，但 reset 不会逐个调用析构函数。\n";
    std::cout << "   所以放进 arena 的对象最好析构很轻，或由外层机制统一管理。\n";

    arena.reset();
    std::cout << "\nafter reset, block count = " << arena.blockCount() << "\n";
    std::cout << "旧指针已经全部失效，不能再访问 c0/add/i32Type。\n";
    return 0;
}
