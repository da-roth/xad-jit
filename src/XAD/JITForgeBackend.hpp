#pragma once

#include <XAD/JITGraph.hpp>
#include <XAD/JITGraphInterpreter.hpp>

// Forge library (https://github.com/da-roth/forge)
#include <src/graph/graph.hpp>
#include <src/compiler/forge_engine.hpp>
#include <src/compiler/compiler_config.hpp>
#include <src/compiler/node_value_buffers/node_value_buffer.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

namespace xad
{

/**
 * JIT Backend using Forge for native code generation.
 * https://github.com/da-roth/forge
 *
 * Uses Forge's JIT compiler for fast forward pass execution.
 * Falls back to JITGraphInterpreter for adjoint computation.
 */
class JITForgeBackend
{
  public:
    JITForgeBackend() = default;
    ~JITForgeBackend() = default;

    JITForgeBackend(JITForgeBackend&&) noexcept = default;
    JITForgeBackend& operator=(JITForgeBackend&&) noexcept = default;

    // No copy
    JITForgeBackend(const JITForgeBackend&) = delete;
    JITForgeBackend& operator=(const JITForgeBackend&) = delete;

    void compile(const JITGraph& graph)
    {
        // Build forge::Graph from JITGraph
        forgeGraph_ = forge::Graph();
        forgeGraph_.nodes.reserve(graph.nodeCount());

        for (std::size_t i = 0; i < graph.nodeCount(); ++i)
        {
            forge::Node n;
            n.op = static_cast<forge::OpCode>(graph.opcodes[i]);
            n.dst = static_cast<uint32_t>(i);
            n.a = graph.operand_a[i];
            n.b = graph.operand_b[i];
            n.c = graph.operand_c[i];
            n.imm = graph.immediates[i];
            n.isActive = (graph.flags[i] & JITNodeFlags::IsActive) != 0;
            n.isDead = (graph.flags[i] & JITNodeFlags::IsDead) != 0;
            n.needsGradient = (graph.flags[i] & JITNodeFlags::NeedsGradient) != 0;
            forgeGraph_.nodes.push_back(n);
        }

        // Copy constant pool and outputs
        forgeGraph_.constPool = graph.const_pool;
        forgeGraph_.outputs.assign(graph.output_ids.begin(), graph.output_ids.end());
        forgeGraph_.diff_inputs.assign(graph.input_ids.begin(), graph.input_ids.end());

        // Extract input node IDs
        inputIds_.clear();
        for (std::size_t i = 0; i < forgeGraph_.nodes.size(); ++i)
        {
            if (forgeGraph_.nodes[i].op == forge::OpCode::Input)
                inputIds_.push_back(static_cast<uint32_t>(i));
        }
        outputIds_.assign(graph.output_ids.begin(), graph.output_ids.end());

        // Configure compiler
        forge::CompilerConfig config;
        config.instructionSet = forge::CompilerConfig::InstructionSet::SSE2_SCALAR;
        config.enableOptimizations = true;
        config.enableCSE = true;
        config.enableAlgebraicSimplification = true;

        // Compile to native code
        forge::ForgeEngine compiler(config);
        kernel_ = compiler.compile(forgeGraph_);

        if (!kernel_)
            throw std::runtime_error("Forge kernel compilation failed");

        // Create node value buffer
        buffer_ = forge::NodeValueBufferFactory::create(forgeGraph_, *kernel_);

        if (!buffer_)
            throw std::runtime_error("Forge buffer creation failed");

        // Initialize interpreter for adjoint computation
        interpreter_.compile(graph);
    }

    void forward(const JITGraph& graph,
                 const double* inputs, std::size_t numInputs,
                 double* outputs, std::size_t numOutputs)
    {
        if (!kernel_ || !buffer_)
            throw std::runtime_error("Backend not compiled");

        if (numInputs != inputIds_.size())
            throw std::runtime_error("Input count mismatch");
        if (numOutputs != outputIds_.size())
            throw std::runtime_error("Output count mismatch");

        // Set input values
        for (std::size_t i = 0; i < numInputs; ++i)
            buffer_->setValue(inputIds_[i], inputs[i]);

        // Execute kernel
        kernel_->executeDirect(
            buffer_->getValuesPtr(),
            buffer_->getGradientsPtr(),
            buffer_->getNumNodes());

        // Read output values
        for (std::size_t i = 0; i < numOutputs; ++i)
            outputs[i] = buffer_->getValue(outputIds_[i]);
    }

    void computeAdjoints(const JITGraph& graph,
                         const double* inputValues, std::size_t numInputs,
                         const double* outputAdjoints, std::size_t numOutputs,
                         double* inputAdjoints)
    {
        // Use interpreter for adjoint computation
        interpreter_.computeAdjoints(graph, inputValues, numInputs,
                                     outputAdjoints, numOutputs, inputAdjoints);
    }

    void reset()
    {
        kernel_.reset();
        buffer_.reset();
        forgeGraph_ = forge::Graph();
        inputIds_.clear();
        outputIds_.clear();
        interpreter_.reset();
    }

  private:
    forge::Graph forgeGraph_;
    std::unique_ptr<forge::StitchedKernel> kernel_;
    std::unique_ptr<forge::INodeValueBuffer> buffer_;
    std::vector<uint32_t> inputIds_;
    std::vector<uint32_t> outputIds_;
    JITGraphInterpreter interpreter_;
};

}  // namespace xad
