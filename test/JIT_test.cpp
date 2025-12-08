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

// Compute derivative using Tape-based AD
double computeTapeDerivative(const TestCase& tc)
{
    xad::Tape<double> tape;
    xad::AD x(tc.input);
    tape.registerInput(x);
    tape.newRecording();
    xad::AD y = tc.func_ad(x);
    tape.registerOutput(y);
    derivative(y) = 1.0;
    tape.computeAdjoints();
    return derivative(x);
}

// Compute derivative using JIT-based AD
double computeJITDerivative(const TestCase& tc)
{
    xad::JITCompiler<double> jit;
    xad::AD x(tc.input);
    jit.registerInput(x);
    jit.newRecording();
    xad::AD y = tc.func_ad(x);
    jit.registerOutput(y);
    derivative(y) = 1.0;
    jit.computeAdjoints();
    return derivative(x);
}

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

TEST_F(JITTest, TapeBasedAD)
{
    std::cout << "\n=== Tape-Based AD Tests ===" << std::endl;

    for (const auto& tc : testCases)
    {
        double expectedOutput = tc.func_double(tc.input);

        xad::Tape<double> tape;
        xad::AD x(tc.input);
        tape.registerInput(x);
        tape.newRecording();
        xad::AD y = tc.func_ad(x);
        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- " << tc.name << " (x=" << tc.input << ") ---" << std::endl;
        std::cout << "  Output:     " << actualOutput << std::endl;
        std::cout << "  Derivative: " << actualDeriv << std::endl;

        EXPECT_NEAR(expectedOutput, actualOutput, 1e-10) << "Function: " << tc.name;
        EXPECT_TRUE(std::isfinite(actualDeriv)) << "Function: " << tc.name;
    }
}

TEST_F(JITTest, JITBasedAD)
{
    std::cout << "\n=== JIT-Based AD Tests ===" << std::endl;

    for (const auto& tc : testCases)
    {
        double expectedOutput = tc.func_double(tc.input);
        double tapeDerivative = computeTapeDerivative(tc);

        xad::JITCompiler<double> jit;
        xad::AD x(tc.input);
        jit.registerInput(x);
        jit.newRecording();
        xad::AD y = tc.func_ad(x);
        jit.registerOutput(y);
        derivative(y) = 1.0;
        jit.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- " << tc.name << " (x=" << tc.input << ") ---" << std::endl;
        std::cout << "  Output:          " << actualOutput << std::endl;
        std::cout << "  Tape derivative: " << tapeDerivative << std::endl;
        std::cout << "  JIT derivative:  " << actualDeriv << std::endl;

        EXPECT_NEAR(expectedOutput, actualOutput, 1e-10) << "Function: " << tc.name;
        EXPECT_NEAR(tapeDerivative, actualDeriv, 1e-10) << "Function: " << tc.name;
    }
}

TEST_F(JITTest, TapeVsJITConsistency)
{
    std::cout << "\n=== Tape vs JIT Consistency Tests ===" << std::endl;

    for (const auto& tc : testCases)
    {
        double tapeDerivative = computeTapeDerivative(tc);
        double jitDerivative = computeJITDerivative(tc);

        std::cout << "\n--- " << tc.name << " ---" << std::endl;
        std::cout << "  Tape: " << tapeDerivative << std::endl;
        std::cout << "  JIT:  " << jitDerivative << std::endl;

        EXPECT_NEAR(tapeDerivative, jitDerivative, 1e-10)
            << "Derivatives should match for: " << tc.name;
    }
}
