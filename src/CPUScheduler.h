#pragma once
#include "MemoryManager.h" // new addition
#include "Process.h"
#include "Config.h"
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

class CPUScheduler {
public:
    CPUScheduler() = default;
    ~CPUScheduler();
    
    // Core functionality
    bool initialize();
    void shutdown();
    
    // Process management
    void addProcess(const std::string& name, int memSize = -1);
    ProcessPtr getProcess(const std::string& name);
    ProcessPtr getAllProcess(const std::string& name);
    bool checkExistingProcess(const std::string& name);

    // Batch processing
    void startBatchGeneration();
    void stopBatchGeneration();
    
    // Reporting
    void listProcesses();
    void generateReport();
    void printVmstat() const; // vmstat command
    
    // Getters
    bool isInitialized() const { return initialized; }
    bool isBatchRunning() const { return batchGenerationRunning; }
    
private:
    Config config;
    std::queue<ProcessPtr> readyQueue;
    std::vector<ProcessPtr> runningProcesses;
    std::vector<ProcessPtr> finishedProcesses;
    std::vector<std::thread> coreThreads;
    std::thread batchGeneratorThread;
    int currentQuantumCycle = 0;
    int quantumCycleCount = 0;

    mutable std::mutex schedulerMutex;
    
    std::condition_variable cv;
    std::atomic<bool> schedulerRunning{false};
    std::atomic<bool> batchGenerationRunning{false};
    std::atomic<uint64_t> cpuTicks{0};
    std::atomic<uint64_t> processCounter{1};
    std::atomic<uint64_t> idleCpuTicks{0};
    std::atomic<uint64_t> activeCpuTicks{0};

    
    bool initialized = false;
    
    FirstFitMemoryAllocator memoryManager; // new addition
    
    // Private methods
    bool loadConfig();
    void coreWorker(int coreId);
    void batchGenerator();
    void tickCounter();
    
    // Statistics helpers
    double getCpuUtilization() const;
    int getCoresUsed() const;
    int getCoresAvailable() const;
};