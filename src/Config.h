
#pragma once
#include <string>

class Config {
private:
    // Configuration parameters
    int numCpu = 4;
    std::string scheduler = "rr";
    unsigned long quantumCycles = 5;
    unsigned long batchProcessFreq = 1;
    unsigned long minIns = 1000;
    unsigned long maxIns = 2000;
    unsigned long delaysPerExec = 0;
    
    // Validation methods
    bool validateNumCpu(int value) const;
    bool validateScheduler(const std::string& value) const;
    bool validateQuantumCycles(unsigned long value) const;
    bool validateBatchProcessFreq(unsigned long value) const;
    bool validateMinIns(unsigned long value) const;
    bool validateMaxIns(unsigned long value) const;
    bool validateDelaysPerExec(unsigned long value) const;
    
    void createDefaultFile(const std::string& filename = "config.txt") const;

public:
    bool loadFromFile(const std::string& filename = "config.txt");
    void printConfig() const;
    
    // Getters
    int getNumCpu() const { return numCpu; }
    std::string getScheduler() const { return scheduler; }
    unsigned long getQuantumCycles() const { return quantumCycles; }
    unsigned long getBatchProcessFreq() const { return batchProcessFreq; }
    unsigned long getMinIns() const { return minIns; }
    unsigned long getMaxIns() const { return maxIns; }
    unsigned long getDelaysPerExec() const { return delaysPerExec; }
    
    // Additional validation checks
    bool isRoundRobin() const { return scheduler == "rr"; }
    bool isValidConfig() const { return minIns <= maxIns; }
};

