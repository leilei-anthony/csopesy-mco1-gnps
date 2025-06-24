#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <climits>
#include <cstdint>

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        createDefaultFile(filename);
        file.open(filename);
    }
    
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        return false;
    }
    
    std::string line;
    bool hasErrors = false;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string key;
        if (iss >> key) {
            std::string value;
            // Read the rest of the line for the value
            std::getline(iss, value);
            // Remove leading whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            
            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            if (key == "num-cpu") {
                int val = std::stoi(value);
                if (validateNumCpu(val)) {
                    numCpu = val;
                } else {
                    hasErrors = true;
                }
            } else if (key == "scheduler") {
                if (validateScheduler(value)) {
                    scheduler = value;
                } else {
                    hasErrors = true;
                }
            } else if (key == "quantum-cycles") {
                unsigned long val = std::stoul(value);
                if (validateQuantumCycles(val)) {
                    quantumCycles = val;
                } else {
                    hasErrors = true;
                }
            } else if (key == "batch-process-freq") {
                unsigned long val = std::stoul(value);
                if (validateBatchProcessFreq(val)) {
                    batchProcessFreq = val;
                } else {
                    hasErrors = true;
                }
            } else if (key == "min-ins") {
                unsigned long val = std::stoul(value);
                if (validateMinIns(val)) {
                    minIns = val;
                } else {
                    hasErrors = true;
                }
            } else if (key == "max-ins") {
                unsigned long val = std::stoul(value);
                if (validateMaxIns(val)) {
                    maxIns = val;
                } else {
                    hasErrors = true;
                }
            } else if (key == "delays-per-exec") {
                unsigned long val = std::stoul(value);
                if (validateDelaysPerExec(val)) {
                    delaysPerExec = val;
                } else {
                    hasErrors = true;
                }
            } else {
                std::cerr << "Warning: Unknown parameter '" << key << "' in config file" << std::endl;
            }
        }
    }
    
    file.close();
    
    // Validate min-ins <= max-ins
    if (minIns > maxIns) {
        std::cerr << "Error: min-ins (" << minIns << ") cannot be greater than max-ins (" << maxIns << ")" << std::endl;
        hasErrors = true;
    }
    
    if (hasErrors) {
        std::cerr << "Configuration file contains errors. Please check the values." << std::endl;
        return false;
    }
    
    return true;
}

bool Config::validateNumCpu(int value) const {
    if (value < 1 || value > 128) {
        std::cerr << "Error: num-cpu must be in range [1, 128]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateScheduler(const std::string& value) const {
    if (value != "fcfs" && value != "rr") {
        std::cerr << "Error: scheduler must be 'fcfs' or 'rr'. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateQuantumCycles(unsigned long value) const {
    if (value < 1 || value > UINT32_MAX) {
        std::cerr << "Error: quantum-cycles must be in range [1, 2^32]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateBatchProcessFreq(unsigned long value) const {
    if (value < 1 || value > UINT32_MAX) {
        std::cerr << "Error: batch-process-freq must be in range [1, 2^32]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateMinIns(unsigned long value) const {
    if (value < 1 || value > UINT32_MAX) {
        std::cerr << "Error: min-ins must be in range [1, 2^32]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateMaxIns(unsigned long value) const {
    if (value < 1 || value > UINT32_MAX) {
        std::cerr << "Error: max-ins must be in range [1, 2^32]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

bool Config::validateDelaysPerExec(unsigned long value) const {
    if (value > UINT32_MAX) {
        std::cerr << "Error: delays-per-exec must be in range [0, 2^32]. Got: " << value << std::endl;
        return false;
    }
    return true;
}

void Config::createDefaultFile(const std::string& filename) const {
    std::ofstream defaultFile(filename);
    if (defaultFile.is_open()) {
        defaultFile << "num-cpu 4\n";
        defaultFile << "scheduler \"rr\"\n";
        defaultFile << "quantum-cycles 5\n";
        defaultFile << "batch-process-freq 1\n";
        defaultFile << "min-ins 1000\n";
        defaultFile << "max-ins 2000\n";
        defaultFile << "delays-per-exec 0\n";
        defaultFile.close();
        std::cout << "Created default " << filename << " file." << std::endl;
    }
}