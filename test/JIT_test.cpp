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
    std::function<double(double)> func_double;
    std::function<xad::AD(const xad::AD&)> func_ad;
    double input;
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
            {"f1 (linear)", f1<double>, f1<xad::AD>, 2.0},
            {"f2 (complex)", f2<double>, f2<xad::AD>, 2.0},
        };
    }
};

TEST_F(JITTest, TapeVsJIT)
{
    for (const auto& tc : testCases)
    {
        double expectedOutput = tc.func_double(tc.input);

        // Compute with Tape
        double tapeOutput, tapeDerivative;
        {
            xad::Tape<double> tape;
            xad::AD x(tc.input);
            tape.registerInput(x);
            tape.newRecording();
            xad::AD y = tc.func_ad(x);
            tape.registerOutput(y);
            derivative(y) = 1.0;
            tape.computeAdjoints();
            tapeOutput = value(y);
            tapeDerivative = derivative(x);
        }

        // Compute with JIT
        double jitOutput, jitDerivative;
        {
            xad::JITCompiler<double> jit;
            xad::AD x(tc.input);
            jit.registerInput(x);
            jit.newRecording();
            xad::AD y = tc.func_ad(x);
            jit.registerOutput(y);
            derivative(y) = 1.0;
            jit.computeAdjoints();
            jitOutput = value(y);
            jitDerivative = derivative(x);
        }

        std::cout << tc.name << " (x=" << tc.input << "): "
                  << "output=" << expectedOutput << ", "
                  << "tape=" << tapeDerivative << ", "
                  << "jit=" << jitDerivative << std::endl;

        EXPECT_NEAR(expectedOutput, tapeOutput, 1e-10) << "Tape output: " << tc.name;
        EXPECT_NEAR(expectedOutput, jitOutput, 1e-10) << "JIT output: " << tc.name;
        EXPECT_NEAR(tapeDerivative, jitDerivative, 1e-10) << "Derivatives: " << tc.name;
    }
}
