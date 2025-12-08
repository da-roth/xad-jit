#pragma once

#include <XAD/Config.hpp>
#include <XAD/Exceptions.hpp>
#include <XAD/JITGraph.hpp>
#include <XAD/JITGraphInterpreter.hpp>
#include <XAD/Macros.hpp>
#include <XAD/Traits.hpp>
#include <complex>
#include <iostream>
#include <vector>

#define JIT_DEBUG(msg) std::cout << "[JIT] " << msg << std::endl

namespace xad
{

template <class Scalar, std::size_t M>
struct AReal;

template <class Real, std::size_t N = 1, class Backend = JITGraphInterpreter>
class JITCompiler
{
  public:
    typedef unsigned int size_type;
    typedef unsigned int slot_type;
    typedef slot_type position_type;
    typedef AReal<Real, N> active_type;
    typedef Real value_type;
    typedef JITCompiler<Real, N, Backend> jit_type;
    typedef typename DerivativesTraits<Real, N>::type derivative_type;
    typedef Backend backend_type;

    static constexpr slot_type INVALID_SLOT = slot_type(-1);

    explicit JITCompiler(bool activate = true) : backend_()
    {
        JIT_DEBUG("Constructor called, activate=" << activate);
        if (activate)
            setActive(this);
        JIT_DEBUG("Constructor done, active_jit_=" << active_jit_);
    }

    ~JITCompiler() { deactivate(); }

    JITCompiler(JITCompiler&& other) noexcept
        : graph_(std::move(other.graph_)),
          backend_(std::move(other.backend_)),
          inputValues_(std::move(other.inputValues_)),
          derivatives_(std::move(other.derivatives_))
    {
        if (other.isActive())
        {
            other.deactivate();
            setActive(this);
        }
    }

    JITCompiler& operator=(JITCompiler&& other) noexcept
    {
        if (this != &other)
        {
            deactivate();
            graph_ = std::move(other.graph_);
            backend_ = std::move(other.backend_);
            inputValues_ = std::move(other.inputValues_);
            derivatives_ = std::move(other.derivatives_);
            if (other.isActive())
            {
                other.deactivate();
                setActive(this);
            }
        }
        return *this;
    }

    JITCompiler(const JITCompiler&) = delete;
    JITCompiler& operator=(const JITCompiler&) = delete;

    XAD_INLINE void activate() { setActive(this); }

    XAD_INLINE void deactivate()
    {
        if (active_jit_ == this)
            active_jit_ = nullptr;
    }

    XAD_INLINE bool isActive() const { return active_jit_ == this; }
    XAD_INLINE static JITCompiler* getActive()
    {
        JIT_DEBUG("getActive() called, returning " << active_jit_);
        return active_jit_;
    }

    XAD_INLINE static void setActive(JITCompiler* j)
    {
        if (active_jit_ != nullptr)
            throw OutOfRange("JIT Compiler already active");
        active_jit_ = j;
    }

    XAD_INLINE static void deactivateAll() { active_jit_ = nullptr; }

    const JITGraph& getGraph() const { return graph_; }
    JITGraph& getGraph() { return graph_; }

    void newRecording()
    {
        JIT_DEBUG("newRecording() called");
        graph_.clear();
        inputValues_.clear();
        derivatives_.clear();
        backend_.reset();
        JIT_DEBUG("newRecording() done");
    }

    XAD_INLINE void registerInput(active_type& inp)
    {
        JIT_DEBUG("registerInput() called, shouldRecord=" << inp.shouldRecord());
        if (!inp.shouldRecord())
        {
            inp.slot_ = graph_.addInput();
            inputValues_.push_back(&inp.value());
            JIT_DEBUG("registerInput() assigned slot=" << inp.slot_);
        }
    }

    XAD_INLINE void registerInput(std::complex<active_type>& inp)
    {
        auto reim_ptr = reinterpret_cast<active_type*>(&inp);
        registerInput(reim_ptr[0]);
        registerInput(reim_ptr[1]);
    }

    XAD_INLINE void registerOutput(active_type& outp)
    {
        JIT_DEBUG("registerOutput() called, shouldRecord=" << outp.shouldRecord() << ", slot=" << outp.slot_);
        if (outp.shouldRecord())
            graph_.markOutput(outp.slot_);
    }

    XAD_INLINE void registerOutput(std::complex<active_type>& outp)
    {
        auto reim_ptr = reinterpret_cast<active_type*>(&outp);
        registerOutput(reim_ptr[0]);
        registerOutput(reim_ptr[1]);
    }

    template <class Inner>
    XAD_INLINE void registerInputs(std::vector<Inner>& v)
    {
        registerInputs(v.begin(), v.end());
    }

    template <class It>
    XAD_INLINE void registerInputs(It first, It last)
    {
        while (first != last)
            registerInput(*first++);
    }

    template <class Inner>
    XAD_INLINE void registerOutputs(std::vector<Inner>& v)
    {
        for (auto& x : v)
            registerOutput(x);
    }

    template <class It>
    XAD_INLINE void registerOutputs(It first, It last)
    {
        while (first != last)
            registerOutput(*first++);
    }

    slot_type registerVariable() { return static_cast<slot_type>(graph_.nodeCount()); }

    uint32_t recordNode(JITOpCode op, uint32_t a = 0, uint32_t b = 0, uint32_t c = 0)
    {
        return graph_.addNode(op, a, b, c);
    }

    uint32_t recordConstant(double value) { return graph_.addConstant(value); }

    void computeAdjoints()
    {
        JIT_DEBUG("computeAdjoints() called");
        std::size_t numInputs = graph_.input_ids.size();
        std::size_t numOutputs = graph_.output_ids.size();
        JIT_DEBUG("  numInputs=" << numInputs << ", numOutputs=" << numOutputs << ", nodeCount=" << graph_.nodeCount());

        std::vector<double> inputs(numInputs);
        for (std::size_t i = 0; i < numInputs; ++i)
            inputs[i] = *inputValues_[i];

        std::vector<double> outputAdjoints(numOutputs, 0.0);
        for (std::size_t i = 0; i < numOutputs; ++i)
        {
            uint32_t outId = graph_.output_ids[i];
            if (outId < derivatives_.size())
                outputAdjoints[i] = derivatives_[outId];
        }

        backend_.compile(graph_);
        std::vector<double> inputAdjoints(numInputs);
        backend_.computeAdjoints(graph_, inputs.data(), numInputs,
                                 outputAdjoints.data(), numOutputs,
                                 inputAdjoints.data());

        derivatives_.resize(graph_.nodeCount(), derivative_type());
        for (std::size_t i = 0; i < numInputs; ++i)
            derivatives_[graph_.input_ids[i]] = inputAdjoints[i];
    }

    derivative_type& derivative(slot_type s)
    {
        if (s >= derivatives_.size())
            derivatives_.resize(s + 1, derivative_type());
        return derivatives_[s];
    }

    const derivative_type& derivative(slot_type s) const
    {
        static derivative_type zero = derivative_type();
        return (s < derivatives_.size()) ? derivatives_[s] : zero;
    }

    derivative_type getDerivative(slot_type s) const { return derivative(s); }

    void setDerivative(slot_type s, const derivative_type& d)
    {
        if (s >= derivatives_.size())
            derivatives_.resize(s + 1, derivative_type());
        derivatives_[s] = d;
    }

    void clearDerivatives()
    {
        std::fill(derivatives_.begin(), derivatives_.end(), derivative_type());
    }

    void clearAll()
    {
        graph_.clear();
        inputValues_.clear();
        derivatives_.clear();
        backend_.reset();
    }

    void printStatus() const {}
    std::size_t getMemory() const { return graph_.nodeCount() * 32 + derivatives_.size() * sizeof(derivative_type); }
    position_type getPosition() const { return static_cast<position_type>(graph_.nodeCount()); }
    void clearDerivativesAfter(position_type) {}
    void resetTo(position_type) {}
    void computeAdjointsTo(position_type) {}

    // Compatibility with old interface
    XAD_INLINE void pushLhs(slot_type) {}

    template <class MulIt, class SlotIt>
    XAD_FORCE_INLINE void pushAll(MulIt, SlotIt, unsigned) {}

  private:
    static XAD_THREAD_LOCAL JITCompiler* active_jit_;
    JITGraph graph_;
    Backend backend_;
    std::vector<const Real*> inputValues_;
    std::vector<derivative_type> derivatives_;
};

template <class Real, std::size_t N, class Backend>
XAD_THREAD_LOCAL JITCompiler<Real, N, Backend>* JITCompiler<Real, N, Backend>::active_jit_ = nullptr;

}  // namespace xad
