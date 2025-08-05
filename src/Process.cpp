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
      sleepCounter(0), isSleeping(false), maxVariables(32) {
    creationTime = getCurrentTimestamp();
    // Initialize memory space (simulated)
    memory.resize(memorySize, 0);
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
    std::uniform_int_distribution<> typeDis(0, 8); // Updated to include READ and WRITE
    
    totalInstructions = instrDis(gen);
    instructions.clear();
    instructions.reserve(totalInstructions);
    
    for (int i = 0; i < totalInstructions; i++) {
        Instruction instr;
        int type = typeDis(gen);
        
        switch (type) {
            case 0: // PRINT
                instr.type = InstructionType::PRINT;
                instr.params.emplace_back("\"Hello world from " + name + "!\"");
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
            case 7: // READ
                instr.type = InstructionType::READ;
                instr.params.emplace_back("readVar" + std::to_string(i));
                {
                    if (memorySize <= 64) break;  // Prevent invalid access if memory too small
                    int addr = 64 + (gen() % (memorySize - 64));
                    std::stringstream ss;
                    ss << "0x" << std::hex << addr;
                    instr.params.emplace_back(ss.str());
                }
                break;

            case 8: // WRITE
                instr.type = InstructionType::WRITE;
                {
                    if (memorySize <= 64) break;
                    int addr = 64 + (gen() % (memorySize - 64));
                    std::stringstream ss;
                    ss << "0x" << std::hex << addr;
                    instr.params.emplace_back(ss.str());
                }
                instr.params.emplace_back(std::to_string(gen() % 256));
                break;

        }
        
        instructions.push_back(std::move(instr));
    }
}

bool Process::parseUserInstructions(const std::string& instructionString) {
    instructions.clear();
    variables.clear();
    
    // Split by semicolon
    std::vector<std::string> instructionTokens;
    std::stringstream ss(instructionString);
    std::string token;
    
    while (std::getline(ss, token, ';')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        
        if (!token.empty()) {
            instructionTokens.push_back(token);
        }
    }
    
    totalInstructions = instructionTokens.size();
    
    for (const auto& instrStr : instructionTokens) {
        Instruction instr;
        if (!parseInstruction(instrStr, instr)) {
            std::cout << "Error parsing instruction: " << instrStr << std::endl;
            return false;
        }
        instructions.push_back(instr);
    }
    
    return true;
}

bool Process::parseInstruction(const std::string& instrStr, Instruction& instr) {
    std::istringstream iss(instrStr);
    std::string command;
    iss >> command;
    
    // Convert to uppercase for comparison
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    
    if (command == "DECLARE") {
        instr.type = InstructionType::DECLARE;
        std::string varName, value;
        if (iss >> varName >> value) {
            instr.params.push_back(varName);
            instr.params.push_back(value);
            return true;
        }
    } else if (command == "ADD") {
        instr.type = InstructionType::ADD;
        std::string result, var1, var2;
        if (iss >> result >> var1 >> var2) {
            instr.params.push_back(result);
            instr.params.push_back(var1);
            instr.params.push_back(var2);
            return true;
        }
    } else if (command == "SUBTRACT") {
        instr.type = InstructionType::SUBTRACT;
        std::string result, var1, var2;
        if (iss >> result >> var1 >> var2) {
            instr.params.push_back(result);
            instr.params.push_back(var1);
            instr.params.push_back(var2);
            return true;
        }
    } else if (command == "READ") {
        instr.type = InstructionType::READ;
        std::string varName, address;
        if (iss >> varName >> address) {
            instr.params.push_back(varName);
            instr.params.push_back(address);
            return true;
        }
    } else if (command == "WRITE") {
        instr.type = InstructionType::WRITE;
        std::string address, value;
        if (iss >> address >> value) {
            instr.params.push_back(address);
            instr.params.push_back(value);
            return true;
        }
    } else if (command.find("PRINT") == 0) {
        instr.type = InstructionType::PRINT;

        // Remove escape characters (like \")
        std::string cleaned = instrStr;
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '\\'), cleaned.end());

        size_t parenPos = cleaned.find('(');
        size_t endParen = cleaned.rfind(')');
        if (parenPos != std::string::npos && endParen != std::string::npos && endParen > parenPos) {
            std::string content = cleaned.substr(parenPos + 1, endParen - parenPos - 1);
            instr.params.push_back(content);
            return true;
        }
    } else if (command == "SLEEP") {
        instr.type = InstructionType::SLEEP;
        int cycles;
        if (iss >> cycles) {
            instr.sleepCycles = cycles;
            return true;
        }
    }
    
    return false;
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
    
    try {
        switch (instr.type) {
            case InstructionType::PRINT: {
                std::string output = processPrintStatement(instr.params[0]);
                std::stringstream logEntry;
                logEntry << "(" << getCurrentTimestamp() << ") Core:" << coreId 
                        << " \"" << output << "\"";
                printLogs.push_back(logEntry.str());
                break;
            }
            case InstructionType::DECLARE:
                if (instr.params.size() >= 2) {
                    if (variables.size() >= maxVariables) {
                        // Ignore instruction if variable limit reached
                        break;
                    }
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
                if (forLoopStack.size() >= 3) {
                    break;
                }
                forLoopStack.push_back(currentInstruction);
                forLoopCounters.push_back(0);
                break;
            case InstructionType::FOR_END:
                if (!forLoopStack.empty()) {
                    int& counter = forLoopCounters.back();
                    int startPos = forLoopStack.back();
                    counter++;
                    
                    if (counter < instructions[startPos].forRepeats) {
                        currentInstruction = startPos;
                    } else {
                        forLoopStack.pop_back();
                        forLoopCounters.pop_back();
                    }
                }
                break;
            case InstructionType::READ:
                if (instr.params.size() >= 2) {
                    if (variables.size() >= maxVariables) {
                        // Ignore instruction if variable limit reached
                        break;
                    }
                    uint32_t address = parseHexAddress(instr.params[1]);
                    if (!isValidAddress(address)) {
                        handleMemoryAccessViolation(address);
                        return false;
                    }
                    uint16_t value = readFromMemory(address);
                    variables[instr.params[0]] = value;
                }
                break;
            case InstructionType::WRITE:
                if (instr.params.size() >= 2) {
                    uint32_t address = parseHexAddress(instr.params[0]);
                    if (!isValidAddress(address)) {
                        handleMemoryAccessViolation(address);
                        return false;
                    }
                    uint16_t value = getValue(instr.params[1]);
                    writeToMemory(address, value);
                }
                break;
        }
    } catch (const std::exception& e) {
        std::stringstream logEntry;
        logEntry << "(" << getCurrentTimestamp() << ") Core:" << coreId 
                << " ERROR: " << e.what();
        printLogs.push_back(logEntry.str());
        isFinished = true;
        finishTime = getCurrentTimestamp();
        return false;
    }
    
    currentInstruction++;
    return true;
}

std::string Process::processPrintStatement(const std::string& statement) {
    std::string result = statement;

    // Trim whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);

    // Handle case: "Result: " + var
    size_t plusPos = result.find(" + ");
    if (plusPos != std::string::npos) {
        std::string leftPart = result.substr(0, plusPos);
        std::string rightPart = result.substr(plusPos + 3);

        // Remove quotes from left part if present
        if (!leftPart.empty() && leftPart.front() == '"' && leftPart.back() == '"') {
            leftPart = leftPart.substr(1, leftPart.length() - 2);
        }

        // Get value of right part (assume it's a variable)
        uint16_t varValue = getValue(rightPart);
        return leftPart + std::to_string(varValue);
    }

    // Handle case: just a string literal
    if (!result.empty() && result.front() == '"' && result.back() == '"') {
        return result.substr(1, result.length() - 2);
    }

    // Handle case: just a variable
    try {
        uint16_t value = getValue(result);
        return std::to_string(value);
    } catch (...) {
        return "[error: unknown variable or format]";
    }
}


uint32_t Process::parseHexAddress(const std::string& hexStr) {
    std::string cleanHex = hexStr;
    if (cleanHex.substr(0, 2) == "0x" || cleanHex.substr(0, 2) == "0X") {
        cleanHex = cleanHex.substr(2);
    }
    
    return static_cast<uint32_t>(std::stoul(cleanHex, nullptr, 16));
}

bool Process::isValidAddress(uint32_t address) {
    return address < memory.size();
}

uint16_t Process::readFromMemory(uint32_t address) {
    if (address + 1 < memory.size()) {
        // Read uint16 from memory (little endian)
        return static_cast<uint16_t>(memory[address]) | 
               (static_cast<uint16_t>(memory[address + 1]) << 8);
    }
    return 0;
}

void Process::writeToMemory(uint32_t address, uint16_t value) {
    if (address + 1 < memory.size()) {
        // Write uint16 to memory (little endian)
        memory[address] = static_cast<uint8_t>(value & 0xFF);
        memory[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }
}

void Process::handleMemoryAccessViolation(uint32_t address) {
    std::stringstream logEntry;
    logEntry << "(" << getCurrentTimestamp() << ") MEMORY ACCESS VIOLATION: "
             << "Attempted to access address 0x" << std::hex << address
             << " outside allocated memory space (0x0 - 0x" << std::hex << (memory.size() - 1) << ")";
    printLogs.push_back(logEntry.str());

    std::stringstream ss;
    ss << std::hex << std::uppercase << address;
    invalidAccess = ss.str();  // Store as hex string (e.g., "0x500")
    
    isFinished = true;
    accessViolation = true;
    finishTime = getCurrentTimestamp();
}

uint16_t Process::getValue(const std::string& param) {
    if (std::isdigit(param[0])) {
        return static_cast<uint16_t>(std::stoi(param));
    }
    
    auto it = variables.find(param);
    if (it == variables.end()) {
        if (variables.size() < maxVariables) {
            variables[param] = 0;
        }
        return 0;
    }
    return it->second;
}