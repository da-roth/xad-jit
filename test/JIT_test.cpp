#include <XAD/XAD.hpp>
#include <gtest/gtest.h>
#include <cmath>
#include <iostream>

// A complex function that uses many math operations
// f(x) = sin(x) + cos(x) * 2
//      + exp(x/10) + log(x + 5)
//      + sqrt(x + 1) + pow(x, 1.5)
//      + tanh(x/3) + sinh(x/5) + cosh(x/5)
//      + abs(x - 1) + x*x (square via mul)
//      + 1/(x + 2) (division)
//      + cbrt(x + 1)
//      + log10(x + 1) + log2(x + 1)
//      + erf(x/2)
template <class T>
T bigFunction(const T& x)
{
    using std::sin; using std::cos; using std::exp; using std::log;
    using std::sqrt; using std::pow; using std::tanh; using std::sinh;
    using std::cosh; using std::abs; using std::cbrt;
    using std::log10; using std::log2; using std::erf;

    T result = sin(x) + cos(x) * 2.0;
    result = result + exp(x / 10.0) + log(x + 5.0);
    result = result + sqrt(x + 1.0) + pow(x, 1.5);
    result = result + tanh(x / 3.0) + sinh(x / 5.0) + cosh(x / 5.0);
    result = result + abs(x - 1.0) + x * x;
    result = result + 1.0 / (x + 2.0);
    result = result + cbrt(x + 1.0);
    result = result + log10(x + 1.0) + log2(x + 1.0);
    result = result + erf(x / 2.0);
    return result;
}

TEST(JIT, TapeBasedSimpleFunction)
{
    std::cout << "\n=== Tape-Based AD Test ===" << std::endl;
    std::cout << "Function: f(x) = x * 3 + 2" << std::endl;
    std::cout << "Derivative: f'(x) = 3" << std::endl;

    // Test with x = 1.0
    {
        double inputVal = 1.0;
        double expectedOutput = 5.0;   // f(1) = 1*3 + 2 = 5
        double expectedDeriv = 3.0;    // f'(x) = 3

        xad::Tape<double> tape;
        xad::AD x(inputVal);

        tape.registerInput(x);
        tape.newRecording();

        xad::AD y = x * 3.0 + 2.0;

        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- Run with x = 1.0 ---" << std::endl;
        std::cout << "  Input:               x = " << inputVal << std::endl;
        std::cout << "  Expected output:     f(x) = " << expectedOutput << std::endl;
        std::cout << "  Actual output:       f(x) = " << actualOutput << std::endl;
        std::cout << "  Expected derivative: f'(x) = " << expectedDeriv << std::endl;
        std::cout << "  Actual derivative:   f'(x) = " << actualDeriv << std::endl;

        EXPECT_DOUBLE_EQ(expectedOutput, actualOutput);
        EXPECT_DOUBLE_EQ(expectedDeriv, actualDeriv);
    }

    // Test with x = 2.0
    {
        double inputVal = 2.0;
        double expectedOutput = 8.0;   // f(2) = 2*3 + 2 = 8
        double expectedDeriv = 3.0;    // f'(x) = 3

        xad::Tape<double> tape;
        xad::AD x(inputVal);

        tape.registerInput(x);
        tape.newRecording();

        xad::AD y = x * 3.0 + 2.0;

        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- Run with x = 2.0 ---" << std::endl;
        std::cout << "  Input:               x = " << inputVal << std::endl;
        std::cout << "  Expected output:     f(x) = " << expectedOutput << std::endl;
        std::cout << "  Actual output:       f(x) = " << actualOutput << std::endl;
        std::cout << "  Expected derivative: f'(x) = " << expectedDeriv << std::endl;
        std::cout << "  Actual derivative:   f'(x) = " << actualDeriv << std::endl;

        EXPECT_DOUBLE_EQ(expectedOutput, actualOutput);
        EXPECT_DOUBLE_EQ(expectedDeriv, actualDeriv);
    }

    std::cout << std::endl;
}

TEST(JIT, TapeBasedBigFunction)
{
    std::cout << "\n=== Tape-Based Big Function Test ===" << std::endl;
    std::cout << "Testing many math operations: sin, cos, exp, log, sqrt, pow, tanh, sinh, cosh," << std::endl;
    std::cout << "                              abs, div, cbrt, log10, log2, erf" << std::endl;

    double inputVal = 2.0;

    // Compute expected output using plain doubles
    double expectedOutput = bigFunction(inputVal);

    // Compute using Tape-based AD
    xad::Tape<double> tape;
    xad::AD x(inputVal);

    tape.registerInput(x);
    tape.newRecording();

    xad::AD y = bigFunction(x);

    tape.registerOutput(y);
    derivative(y) = 1.0;
    tape.computeAdjoints();

    double actualOutput = value(y);
    double actualDeriv = derivative(x);

    std::cout << "\n--- Run with x = " << inputVal << " ---" << std::endl;
    std::cout << "  Expected output: f(x) = " << expectedOutput << std::endl;
    std::cout << "  Actual output:   f(x) = " << actualOutput << std::endl;
    std::cout << "  Derivative:      f'(x) = " << actualDeriv << std::endl;

    EXPECT_NEAR(expectedOutput, actualOutput, 1e-10);
    // We'll verify the derivative by comparing with JIT later
    // For now, just check it's a reasonable value (not NaN or Inf)
    EXPECT_TRUE(std::isfinite(actualDeriv));

    std::cout << std::endl;
}

TEST(JIT, JitBasedSimpleFunction)
{
    std::cout << "\n=== Jit-Based AD Test ===" << std::endl;
    std::cout << "Function: f(x) = x * 3 + 2" << std::endl;
    std::cout << "Derivative: f'(x) = 3" << std::endl;

    // Test with x = 1.0
    {
        double inputVal = 1.0;
        double expectedOutput = 5.0;   // f(1) = 1*3 + 2 = 5
        double expectedDeriv = 3.0;    // f'(x) = 3

        xad::JITCompiler<double> tape;
        xad::AD x(inputVal);

        tape.registerInput(x);
        tape.newRecording();

        xad::AD y = x * 3.0 + 2.0;

        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- Run with x = 1.0 ---" << std::endl;
        std::cout << "  Input:               x = " << inputVal << std::endl;
        std::cout << "  Expected output:     f(x) = " << expectedOutput << std::endl;
        std::cout << "  Actual output:       f(x) = " << actualOutput << std::endl;
        std::cout << "  Expected derivative: f'(x) = " << expectedDeriv << std::endl;
        std::cout << "  Actual derivative:   f'(x) = " << actualDeriv << std::endl;

        EXPECT_DOUBLE_EQ(expectedOutput, actualOutput);
        EXPECT_DOUBLE_EQ(expectedDeriv, actualDeriv);
    }

    // Test with x = 2.0
    {
        double inputVal = 2.0;
        double expectedOutput = 8.0;   // f(2) = 2*3 + 2 = 8
        double expectedDeriv = 3.0;    // f'(x) = 3

        xad::JITCompiler<double> tape;
        xad::AD x(inputVal);

        tape.registerInput(x);
        tape.newRecording();

        xad::AD y = x * 3.0 + 2.0;

        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();

        double actualOutput = value(y);
        double actualDeriv = derivative(x);

        std::cout << "\n--- Run with x = 2.0 ---" << std::endl;
        std::cout << "  Input:               x = " << inputVal << std::endl;
        std::cout << "  Expected output:     f(x) = " << expectedOutput << std::endl;
        std::cout << "  Actual output:       f(x) = " << actualOutput << std::endl;
        std::cout << "  Expected derivative: f'(x) = " << expectedDeriv << std::endl;
        std::cout << "  Actual derivative:   f'(x) = " << actualDeriv << std::endl;

        EXPECT_DOUBLE_EQ(expectedOutput, actualOutput);
        EXPECT_DOUBLE_EQ(expectedDeriv, actualDeriv);
    }

    std::cout << std::endl;
}

TEST(JIT, JitBasedBigFunction)
{
    std::cout << "\n=== JIT-Based Big Function Test ===" << std::endl;
    std::cout << "Testing many math operations: sin, cos, exp, log, sqrt, pow, tanh, sinh, cosh," << std::endl;
    std::cout << "                              abs, div, cbrt, log10, log2, erf" << std::endl;

    double inputVal = 2.0;

    // Compute expected output using plain doubles
    double expectedOutput = bigFunction(inputVal);

    // Also compute the Tape derivative for comparison
    double tapeDerivative;
    {
        xad::Tape<double> tape;
        xad::AD x(inputVal);
        tape.registerInput(x);
        tape.newRecording();
        xad::AD y = bigFunction(x);
        tape.registerOutput(y);
        derivative(y) = 1.0;
        tape.computeAdjoints();
        tapeDerivative = derivative(x);
    }

    // Compute using JIT-based AD
    xad::JITCompiler<double> jit;
    xad::AD x(inputVal);

    jit.registerInput(x);
    jit.newRecording();

    xad::AD y = bigFunction(x);

    jit.registerOutput(y);
    derivative(y) = 1.0;
    jit.computeAdjoints();

    double actualOutput = value(y);
    double actualDeriv = derivative(x);

    std::cout << "\n--- Run with x = " << inputVal << " ---" << std::endl;
    std::cout << "  Expected output:    f(x)  = " << expectedOutput << std::endl;
    std::cout << "  Actual output:      f(x)  = " << actualOutput << std::endl;
    std::cout << "  Tape derivative:    f'(x) = " << tapeDerivative << std::endl;
    std::cout << "  JIT derivative:     f'(x) = " << actualDeriv << std::endl;

    EXPECT_NEAR(expectedOutput, actualOutput, 1e-10);
    // Compare JIT derivative with Tape derivative - they should match!
    EXPECT_NEAR(tapeDerivative, actualDeriv, 1e-10);

    std::cout << std::endl;
}
