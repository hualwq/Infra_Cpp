#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// A deliberately small AI compiler pipeline for the expression:
//   y = relu(x * weight + bias)
// It has three layers:
//   Graph IR -> compiler passes -> executable kernels.

struct Tensor {
    std::vector<float> data;
    std::vector<int> shape;

    Tensor() = default;
    Tensor(std::vector<float> values, std::vector<int> dims)
        : data(std::move(values)), shape(std::move(dims)) {}

    std::size_t size() const { return data.size(); }
};

std::string tensorToString(const Tensor &tensor) {
    std::ostringstream stream;
    stream << "[";
    for (std::size_t i = 0; i < tensor.data.size(); ++i) {
        if (i != 0) stream << ", ";
        stream << std::fixed << std::setprecision(1) << tensor.data[i];
    }
    stream << "]";
    return stream.str();
}

enum class OpCode { Input, Constant, Mul, Add, Relu, FusedMulAddRelu };

const char *opName(OpCode op) {
    switch (op) {
        case OpCode::Input: return "input";
        case OpCode::Constant: return "constant";
        case OpCode::Mul: return "mul";
        case OpCode::Add: return "add";
        case OpCode::Relu: return "relu";
        case OpCode::FusedMulAddRelu: return "fused.mul_add_relu";
    }
    return "unknown";
}

struct Node {
    int id;
    OpCode op;
    std::string name;
    std::vector<int> inputs;
    Tensor constant;
};

class Graph {
public:
    int addInput(std::string name) { return addNode(OpCode::Input, std::move(name), {}); }

    int addConstant(std::string name, Tensor value) {
        const int id = addNode(OpCode::Constant, std::move(name), {});
        nodes_.back().constant = std::move(value);
        return id;
    }

    int addOp(OpCode op, std::string name, std::vector<int> inputs) {
        return addNode(op, std::move(name), std::move(inputs));
    }

    Node &node(int id) { return nodes_.at(static_cast<std::size_t>(id)); }
    const Node &node(int id) const { return nodes_.at(static_cast<std::size_t>(id)); }
    const std::vector<Node> &nodes() const { return nodes_; }

    void dump(const std::string &title) const {
        std::cout << "\n--- " << title << " ---\n";
        for (const Node &node : nodes_) {
            std::cout << "%" << node.id << " = " << opName(node.op) << "  # " << node.name;
            if (!node.inputs.empty()) {
                std::cout << " (%";
                for (std::size_t i = 0; i < node.inputs.size(); ++i) {
                    if (i != 0) std::cout << ", %";
                    std::cout << node.inputs[i];
                }
                std::cout << ")";
            }
            if (node.op == OpCode::Constant) std::cout << " = " << tensorToString(node.constant);
            std::cout << "\n";
        }
    }

private:
    int addNode(OpCode op, std::string name, std::vector<int> inputs) {
        const int id = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{id, op, std::move(name), std::move(inputs), {}});
        return id;
    }

    std::vector<Node> nodes_;
};

Tensor elementwiseBinary(const Tensor &left, const Tensor &right,
                         const std::function<float(float, float)> &operation) {
    if (left.size() != right.size()) throw std::runtime_error("demo only supports equal tensor sizes");
    Tensor result{{}, left.shape};
    result.data.resize(left.size());
    for (std::size_t i = 0; i < left.size(); ++i) result.data[i] = operation(left.data[i], right.data[i]);
    return result;
}

Tensor relu(const Tensor &input) {
    Tensor result = input;
    for (float &value : result.data) value = std::max(0.0F, value);
    return result;
}

// The reference interpreter is the traditional compiler analogy: execute one IR op at a time.
Tensor interpret(const Graph &graph, const Tensor &input) {
    std::map<int, Tensor> values;
    for (const Node &node : graph.nodes()) {
        switch (node.op) {
            case OpCode::Input: values[node.id] = input; break;
            case OpCode::Constant: values[node.id] = node.constant; break;
            case OpCode::Mul:
                values[node.id] = elementwiseBinary(values.at(node.inputs[0]), values.at(node.inputs[1]),
                                                      [](float a, float b) { return a * b; });
                break;
            case OpCode::Add:
                values[node.id] = elementwiseBinary(values.at(node.inputs[0]), values.at(node.inputs[1]),
                                                      [](float a, float b) { return a + b; });
                break;
            case OpCode::Relu: values[node.id] = relu(values.at(node.inputs[0])); break;
            case OpCode::FusedMulAddRelu: {
                const Tensor &x = values.at(node.inputs[0]);
                const Tensor &weight = values.at(node.inputs[1]);
                const Tensor &bias = values.at(node.inputs[2]);
                Tensor result{{}, x.shape};
                result.data.resize(x.size());
                for (std::size_t i = 0; i < x.size(); ++i) {
                    result.data[i] = std::max(0.0F, x.data[i] * weight.data[i] + bias.data[i]);
                }
                values[node.id] = std::move(result);
                break;
            }
        }
    }
    return values.at(static_cast<int>(graph.nodes().size() - 1));
}

Graph fuseMulAddRelu(const Graph &original) {
    Graph fused;
    std::map<int, int> remap;  // 用来存旧图和新图中节点的映射
    for (std::size_t i = 0; i < original.nodes().size(); ++i) {
        const Node &node = original.nodes()[i];
        // Match relu(add(mul(x, w), bias)). In real compilers this is a rewrite pattern.
        if (node.op == OpCode::Relu) {
            const Node &add = original.node(node.inputs.at(0));
            if (add.op == OpCode::Add) {
                const Node &mul = original.node(add.inputs.at(0));
                if (mul.op == OpCode::Mul) {
                    const int newId = fused.addOp(OpCode::FusedMulAddRelu, "linear_relu",
                        {remap.at(mul.inputs[0]), remap.at(mul.inputs[1]), remap.at(add.inputs[1])});
                    remap[node.id] = newId;
                    continue;
                }
            }
        }
        if (node.op == OpCode::Mul || node.op == OpCode::Add) {
            // They are internal to the matched chain and need no standalone emitted op.
            remap[node.id] = -1;
            continue;
        }
        if (node.op == OpCode::Input) remap[node.id] = fused.addInput(node.name);
        else if (node.op == OpCode::Constant) remap[node.id] = fused.addConstant(node.name, node.constant);
    }
    return fused;
}

using Kernel = std::function<Tensor(const Tensor &)>;

Kernel lowerToKernel(const Graph &graph) {
    // Lowering turns a graph-level fused op into a target-level loop kernel.
    const Node &weight = graph.node(1);
    const Node &bias = graph.node(2);
    return [weight, bias](const Tensor &x) {
        Tensor output{{}, x.shape};
        output.data.resize(x.size());
        for (std::size_t i = 0; i < x.size(); ++i) {
            output.data[i] = std::max(0.0F, x.data[i] * weight.constant.data[i] + bias.constant.data[i]);
        }
        return output;
    };
}

class AotCompiler {
public:
    explicit AotCompiler(const Graph &model) : kernel_(lowerToKernel(fuseMulAddRelu(model))) {
        std::cout << "AOT: startup compilation completed before serving requests.\n";
    }

    Tensor run(const Tensor &input) const { return kernel_(input); }

private:
    Kernel kernel_;
};

class JitCompiler {
public:
    explicit JitCompiler(const Graph &model) : model_(model) {}

    Tensor run(const Tensor &input) {
        const std::string signature = shapeSignature(input);
        auto found = cache_.find(signature);
        if (found == cache_.end()) {
            std::cout << "JIT: cache miss for " << signature << "; specialize and compile now.\n";
            found = cache_.emplace(signature, lowerToKernel(fuseMulAddRelu(model_))).first;
        } else {
            std::cout << "JIT: cache hit for " << signature << "; reuse compiled kernel.\n";
        }
        return found->second(input);
    }

private:
    static std::string shapeSignature(const Tensor &tensor) {
        std::ostringstream stream;
        stream << "shape=";
        for (int dim : tensor.shape) stream << dim << "x";
        return stream.str();
    }

    Graph model_;
    std::map<std::string, Kernel> cache_;
};

int main() {
    std::cout << "=== Fun Compiler: a tiny AI compiler pipeline ===\n";
    std::cout << "Model: y = relu(x * weight + bias)\n";

    Graph graph;
    const int x = graph.addInput("x");
    const int weight = graph.addConstant("weight", {{2.0F, -1.0F, 0.5F, 3.0F}, {4}});
    const int bias = graph.addConstant("bias", {{-1.0F, 2.0F, 1.0F, -4.0F}, {4}});
    const int scaled = graph.addOp(OpCode::Mul, "scaled", {x, weight});
    const int shifted = graph.addOp(OpCode::Add, "shifted", {scaled, bias});
    graph.addOp(OpCode::Relu, "y", {shifted});

    const Tensor input{{1.0F, 3.0F, -4.0F, 2.0F}, {4}};
    graph.dump("Graph IR (frontend output)");
    std::cout << "\nInterpreter result: " << tensorToString(interpret(graph, input)) << "\n";

    Graph optimized = fuseMulAddRelu(graph);
    optimized.dump("Graph IR after fusion pass");
    std::cout << "Fusion removes intermediate tensors: scaled and shifted.\n";

    std::cout << "\n--- AOT execution ---\n";
    AotCompiler aot(graph);
    std::cout << "AOT result: " << tensorToString(aot.run(input)) << "\n";

    std::cout << "\n--- JIT execution ---\n";
    JitCompiler jit(graph);
    const Tensor firstJitResult = jit.run(input);
    std::cout << "JIT result: " << tensorToString(firstJitResult) << "\n";
    const Tensor secondJitResult = jit.run(input);
    std::cout << "JIT result: " << tensorToString(secondJitResult) << "\n";

    std::cout << "\nTakeaway: a traditional compiler lowers source code once; an AI compiler also "
                 "uses tensor shapes, operators and hardware constraints to optimize a dataflow graph.\n";
    return 0;
}


    // 计算图（融合前）：
    //  x(0)   weight(1)   bias(2)
    //     \      /           |
    //      Mul(3)            |
    //         \              |
    //          Add(4)        |
    //             \          |
    //              Relu(5)
    // 注意，这个计算图的表示中，节点： SSA value definition，  边：值的依赖关系

    // 计算图(融合后):
    // x(0)    weight(1)    bias(2)
    //   \        |          /
    //    \       |         /
    //     FusedMulAddRelu(3)
    //           |
    //           y
