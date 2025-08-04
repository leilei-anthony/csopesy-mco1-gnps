#pragma once
#include "Instruction.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// Forward declaration
class FirstFitMemoryAllocator;
class Process;

// === Pointer alias for shared pointer to Process ===
using ProcessPtr = std::shared_ptr<Process>;

class Process {
public:
    std::string name;
    int pid;
    int totalInstructions;
    int currentInstruction;
    std::string creationTime;
    std::string finishTime;
    int assignedCore;
    bool isFinished;
    std::vector<std::string> printLogs;
    std::vector<Instruction> instructions;
    std::map<std::string, uint16_t> variables;

    // Round-robin scheduling variables
    int remainingQuantum;
    int sleepCounter;
    bool isSleeping;

    // For loop handling
    std::vector<int> forLoopStack;
    std::vector<int> forLoopCounters;

    int memorySize; // bytes allocated to this process

    // === MO2 additions ===
    bool crashedDueToMemoryViolation = false;
    std::string memoryErrorDetails;
    static const int memoryVariableLimit = 32;

    Process(const std::string& processName, int pid, int memorySize);

    void generateRandomInstructions(int minIns, int maxIns);
    bool executeNextInstruction(int coreId);  // needs definition in .cpp

    std::string getCurrentTimestamp() const;

    // === Memory access ===
    void setMemoryManager(std::shared_ptr<FirstFitMemoryAllocator> allocator);
    std::shared_ptr<FirstFitMemoryAllocator> memoryManager;

private:
    uint16_t getValue(const std::string& param);
    bool canDeclareVariable(const std::string& var);
};
