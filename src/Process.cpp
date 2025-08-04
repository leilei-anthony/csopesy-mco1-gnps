#include "MemoryManager.h"
#include "Process.h"
#include "Instruction.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <algorithm>
#include <iostream>

Process::Process(const std::string& processName, int pid, int memorySize)
    : name(processName), pid(pid), memorySize(memorySize), totalInstructions(0), currentInstruction(0),
      assignedCore(-1), isFinished(false), remainingQuantum(0), 
      sleepCounter(0), isSleeping(false) {
    creationTime = getCurrentTimestamp();
}

std::string Process::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y, %I:%M:%S %p");
    return ss.str();
}

void Process::generateRandomInstructions(int minIns, int maxIns) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instrDis(minIns, maxIns);
    std::uniform_int_distribution<> typeDis(0, 7); // include ACCESS_MEM
    std::uniform_int_distribution<> offsetDis(0, memorySize - 1);

    totalInstructions = instrDis(gen);
    instructions.clear();
    instructions.reserve(totalInstructions);

    for (int i = 0; i < totalInstructions; i++) {
        Instruction instr;
        int type = typeDis(gen);

        switch (type) {
            case 0: // PRINT
                instr.type = InstructionType::PRINT;
                instr.params.emplace_back("Hello world from " + name + "!");
                break;
            case 1: // DECLARE
                instr.type = InstructionType::DECLARE;
                instr.params.emplace_back("var" + std::to_string(i));
                instr.params.emplace_back(std::to_string(gen() % 100));
                break;
            case 2: // ADD
                instr.type = InstructionType::ADD;
                instr.params.emplace_back("result" + std::to_string(i));
                instr.params.emplace_back("var1");
                instr.params.emplace_back("var2");
                break;
            case 3: // SUBTRACT
                instr.type = InstructionType::SUBTRACT;
                instr.params.emplace_back("result" + std::to_string(i));
                instr.params.emplace_back("var1");
                instr.params.emplace_back("var2");
                break;
            case 4: // SLEEP
                instr.type = InstructionType::SLEEP;
                instr.sleepCycles = (gen() % 10) + 1;
                break;
            case 5: // FOR_START
                instr.type = InstructionType::FOR_START;
                instr.forRepeats = (gen() % 5) + 1;
                break;
            case 6: // FOR_END
                instr.type = InstructionType::FOR_END;
                break;
            case 7: // ACCESS_MEM
                instr.type = InstructionType::ACCESS_MEM;
                instr.params.emplace_back(std::to_string(offsetDis(gen)));
                break;
        }

        instructions.push_back(std::move(instr));
    }
}

bool Process::executeNextInstruction(int coreId) {
    if (currentInstruction >= totalInstructions) {
        if (!isFinished) {
            isFinished = true;
            finishTime = getCurrentTimestamp();
        }
        return false;
    }

    if (isSleeping) {
        sleepCounter--;
        if (sleepCounter <= 0) {
            isSleeping = false;
        }
        return true;
    }

    const Instruction& instr = instructions[currentInstruction];

    switch (instr.type) {
        case InstructionType::PRINT: {
            std::stringstream logEntry;
            logEntry << "(" << getCurrentTimestamp() << ") Core:" << coreId 
                     << " \"" << instr.params[0] << "\"";
            printLogs.push_back(logEntry.str());
            break;
        }
        case InstructionType::DECLARE:
            if (instr.params.size() >= 2 && canDeclareVariable(instr.params[0])) {
                variables[instr.params[0]] = static_cast<uint16_t>(std::stoi(instr.params[1]));
            }
            break;
        case InstructionType::ADD:
            if (instr.params.size() >= 3) {
                uint16_t val1 = getValue(instr.params[1]);
                uint16_t val2 = getValue(instr.params[2]);
                variables[instr.params[0]] = val1 + val2;
            }
            break;
        case InstructionType::SUBTRACT:
            if (instr.params.size() >= 3) {
                uint16_t val1 = getValue(instr.params[1]);
                uint16_t val2 = getValue(instr.params[2]);
                variables[instr.params[0]] = val1 - val2;
            }
            break;
        case InstructionType::SLEEP:
            isSleeping = true;
            sleepCounter = instr.sleepCycles;
            break;
        case InstructionType::FOR_START:
            if (forLoopStack.size() < 3) {
                forLoopStack.push_back(currentInstruction);
                forLoopCounters.push_back(0);
            }
            break;
        case InstructionType::FOR_END:
            if (!forLoopStack.empty()) {
                int& counter = forLoopCounters.back();
                int startPos = forLoopStack.back();
                counter++;

                if (counter < instructions[startPos].forRepeats) {
                    currentInstruction = startPos;
                    return true;
                } else {
                    forLoopStack.pop_back();
                    forLoopCounters.pop_back();
                }
            }
            break;
        case InstructionType::ACCESS_MEM:
            if (instr.params.size() >= 1 && memoryManager) {
                uint16_t offset = static_cast<uint16_t>(std::stoi(instr.params[0]));
                if (offset < memorySize) {
                    uint16_t dummy;
                    std::string err;
                    if (!memoryManager->readMemory(pid, offset, dummy, err)) {
                        crashedDueToMemoryViolation = true;
                        memoryErrorDetails = err;
                        isFinished = true;
                        return false;
                    }
                } else {
                    crashedDueToMemoryViolation = true;
                    memoryErrorDetails = "Accessed offset beyond allocated memory.";
                    isFinished = true;
                    return false;
                }
            }
            break;
    }

    currentInstruction++;
    return true;
}

uint16_t Process::getValue(const std::string& param) {
    if (std::isdigit(param[0])) {
        return static_cast<uint16_t>(std::stoi(param));
    }

    auto it = variables.find(param);
    if (it == variables.end()) {
        variables[param] = 0;
        return 0;
    }
    return it->second;
}

bool Process::canDeclareVariable(const std::string& var) {
    return variables.size() < memoryVariableLimit || variables.count(var);
}

void Process::setMemoryManager(std::shared_ptr<FirstFitMemoryAllocator> allocator) {
    memoryManager = allocator;
}