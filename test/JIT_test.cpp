#include <XAD/XAD.hpp>
#include <gtest/gtest.h>
#include <iostream>

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


TEST(JIT, JitBasedSimpleFunction)
{
    std::cout << "\n=== Tape-Based AD Test ===" << std::endl;
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
