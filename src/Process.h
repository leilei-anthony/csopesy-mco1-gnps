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
    Process(const std::string& processName, int pid, int memorySize);
    
    void generateRandomInstructions(int minIns, int maxIns);
    bool executeNextInstruction(int coreId);
    std::string getCurrentTimestamp() const;
    
private:
    uint16_t getValue(const std::string& param);
};

using ProcessPtr = std::shared_ptr<Process>;