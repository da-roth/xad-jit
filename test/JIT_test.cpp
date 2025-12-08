#include <XAD/XAD.hpp>
#include <gtest/gtest.h>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ============================================================================
// Test Functions
// ============================================================================

// f1: Simple linear function
// f(x) = x * 3 + 2, f'(x) = 3
template <class T>
T f1(const T& x)
{
    return x * 3.0 + 2.0;
}

// f2: Complex function with many math operations
// Uses: sin, cos, exp, log, sqrt, tanh, sinh, cosh, abs, cbrt, log10, log2, erf
// Note: pow with scalar exponent not yet supported in JIT
template <class T>
T f2(const T& x)
{
    using std::sin; using std::cos; using std::exp; using std::log;
    using std::sqrt; using std::tanh; using std::sinh;
    using std::cosh; using std::abs; using std::cbrt;
    using std::log10; using std::log2; using std::erf;

    T result = sin(x) + cos(x) * 2.0;
    result = result + exp(x / 10.0) + log(x + 5.0);
    result = result + sqrt(x + 1.0);
    result = result + tanh(x / 3.0) + sinh(x / 5.0) + cosh(x / 5.0);
    result = result + abs(x - 1.0) + x * x;
    result = result + 1.0 / (x + 2.0);
    result = result + cbrt(x + 1.0);
    result = result + log10(x + 1.0) + log2(x + 1.0);
    result = result + erf(x / 2.0);
    return result;
}

// ============================================================================
// Test Infrastructure
// ============================================================================

struct TestCase
{
    std::string name;
    std::string formula;
    std::function<double(double)> func_double;
    std::function<xad::AD(const xad::AD&)> func_ad;
    std::vector<double> inputs;
};

// ============================================================================
// Tests
// ============================================================================

class JITTest : public ::testing::Test
{
  protected:
    std::vector<TestCase> testCases;

    void SetUp() override
    {
        testCases = {
            {"f1", "x * 3 + 2", f1<double>, f1<xad::AD>, {2.0, 0.5, -1.0}},
            {"f2", "sin(x) + cos(x)*2 + exp(x/10) + log(x+5) + sqrt(x+1) + ...", f2<double>, f2<xad::AD>, {2.0, 0.5}},
        };
    }
};

TEST_F(JITTest, TapeVsJIT)
{
    for (const auto& tc : testCases)
    {
        std::cout << tc.name << "(x) = " << tc.formula << std::endl;

        std::vector<double> tapeOutputs, tapeDerivatives;
        std::vector<double> jitOutputs, jitDerivatives;

        // Compute all with Tape
        {
            xad::Tape<double> tape;
            for (double input : tc.inputs)
            {
                xad::AD x(input);
                tape.registerInput(x);
                tape.newRecording();
                xad::AD y = tc.func_ad(x);
                tape.registerOutput(y);
                derivative(y) = 1.0;
                tape.computeAdjoints();
                tapeOutputs.push_back(value(y));
                tapeDerivatives.push_back(derivative(x));
                tape.clearAll();
            }
        }

        // Compute all with JIT
        {
            xad::JITCompiler<double> jit;
            for (double input : tc.inputs)
            {
                xad::AD x(input);
                jit.registerInput(x);
                jit.newRecording();
                xad::AD y = tc.func_ad(x);
                jit.registerOutput(y);
                derivative(y) = 1.0;
                jit.computeAdjoints();
                jitOutputs.push_back(value(y));
                jitDerivatives.push_back(derivative(x));
                jit.clearAll();
            }
        }

        // Compare and print results
        for (std::size_t i = 0; i < tc.inputs.size(); ++i)
        {
            double input = tc.inputs[i];
            double expectedOutput = tc.func_double(input);

            std::cout << "  x=" << input << ": "
                      << "output=" << tapeOutputs[i] << ", "
                      << "derivTape=" << tapeDerivatives[i] << ", "
                      << "derivJIT=" << jitDerivatives[i] << std::endl;

            EXPECT_NEAR(expectedOutput, tapeOutputs[i], 1e-10) << tc.name << " tape output at x=" << input;
            EXPECT_NEAR(expectedOutput, jitOutputs[i], 1e-10) << tc.name << " JIT output at x=" << input;
            EXPECT_NEAR(tapeDerivatives[i], jitDerivatives[i], 1e-10) << tc.name << " derivatives at x=" << input;
        }
        std::cout << std::endl;
    }
}
