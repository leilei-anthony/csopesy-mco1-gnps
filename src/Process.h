#pragma once
#include "Instruction.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

class Process {
public:
    std::string name;
    int pid;
    int memorySize; // bytes allocated to this process
    int totalInstructions;
    int currentInstruction;
    std::string creationTime;
    std::string finishTime;
    int assignedCore;
    bool isFinished;

    // Memory and variables
    std::vector<uint8_t> memory;
    static constexpr size_t maxVariables = 32; // 32 variables * 2 bytes = 64 bytes

    std::vector<std::string> printLogs;
    std::vector<Instruction> instructions;
    std::map<std::string, uint16_t> variables; // symbol table: name -> value

    // Round-robin scheduling variables
    int remainingQuantum;
    int sleepCounter;
    bool isSleeping;
    
    // For loop handling
    std::vector<int> forLoopStack;
    std::vector<int> forLoopCounters;
    

    Process(const std::string& processName, int pid, int memorySize);
    
    void generateRandomInstructions(int minIns, int maxIns);
    bool executeNextInstruction(int coreId);
    std::string getCurrentTimestamp() const;
    
    // new
    bool parseUserInstructions(const std::string& instructionString);
    bool parseInstruction(const std::string& instrStr, Instruction& instr);
    
    // new Memory operations
    uint32_t parseHexAddress(const std::string& hexStr);
    bool isValidAddress(uint32_t address);
    uint16_t readFromMemory(uint32_t address);
    void writeToMemory(uint32_t address, uint16_t value);
    void handleMemoryAccessViolation(uint32_t address);
    
    // new Print processing
    std::string processPrintStatement(const std::string& statement);

private:
    uint16_t getValue(const std::string& param);
};

using ProcessPtr = std::shared_ptr<Process>;