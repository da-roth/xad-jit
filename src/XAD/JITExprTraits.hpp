#pragma once

#include <XAD/JITGraph.hpp>
#include <XAD/JITOpCodeTraits.hpp>
#include <type_traits>

namespace xad
{

// Helper to detect if Op has a scalar constant (b_ member)
template <class Op, class = void>
struct HasScalarConstant : std::false_type {};

template <class Op>
struct HasScalarConstant<Op, decltype(void(std::declval<Op>().b_))> : std::true_type {};

// Helper to get constant value from scalar ops
template <class Op>
typename std::enable_if<HasScalarConstant<Op>::value, double>::type
getScalarConstant(const Op& op) { return static_cast<double>(op.b_); }

// Detect if Op is scalar_sub1 or scalar_div1 (scalar is first operand)
template <class> struct IsScalarFirstOp : std::false_type {};
template <class S, class T> struct IsScalarFirstOp<scalar_sub1_op<S, T>> : std::true_type {};
template <class S, class T> struct IsScalarFirstOp<scalar_div1_op<S, T>> : std::true_type {};
template <class S, class T> struct IsScalarFirstOp<scalar_pow1_op<S, T>> : std::true_type {};

// Helper to record scalar value as constant
inline uint32_t recordJITConstant(JITGraph& graph, double value)
{
    uint32_t constIdx = graph.addConstant(value);
    return graph.addNode(JITOpCode::Constant, 0, 0, 0, static_cast<double>(constIdx));
}

}  // namespace xad
