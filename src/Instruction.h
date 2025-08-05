#pragma once
#include <vector>
#include <string>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    READ,
    WRITE,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END,
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> params;
    int sleepCycles = 0;
    int forRepeats = 0;
    int memoryAddress = -1; 

    Instruction() = default;
    Instruction(InstructionType t) : type(t) {}
};