#pragma once
#include <vector>
#include <string>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> params;
    int sleepCycles = 0;
    int forRepeats = 0;
    
    Instruction() = default;
    Instruction(InstructionType t) : type(t) {}
};