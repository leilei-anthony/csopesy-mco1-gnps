#include "CPUScheduler.h"
#include "MemoryManager.h"  // new addition
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>



CPUScheduler::~CPUScheduler() {
    shutdown();
}

bool CPUScheduler::initialize() {
    if (initialized) {
        std::cout << "Scheduler already initialized." << std::endl;
        return false;
    }
    
    if (!config.loadFromFile()) {
        std::cout << "Error: Could not load config.txt" << std::endl;
        return false;
    }
    
    memoryManager.init(             // new addition
    config.getMaxOverallMem(),
    config.getMemPerFrame(),
    config.getMaxMemPerProc()
    );

    schedulerRunning = true;
    
    // Start CPU tick counter
    std::thread tickThread(&CPUScheduler::tickCounter, this);
    tickThread.detach();
    
    // Start core worker threads
    coreThreads.reserve(config.getNumCpu());
    for (int i = 0; i < config.getNumCpu(); i++) {
        coreThreads.emplace_back(&CPUScheduler::coreWorker, this, i);
    }
    
    initialized = true;
    std::cout << "Scheduler initialized with " << config.getNumCpu() << " CPU cores using " 
              << config.getScheduler() << " scheduling algorithm." << std::endl;
    return true;
}

void CPUScheduler::startBatchGeneration() {
    if (!initialized) {
        std::cout << "Please initialize the scheduler first." << std::endl;
        return;
    }
    
    if (batchGenerationRunning) {
        std::cout << "Batch process generation is already running." << std::endl;
        return;
    }
    
    batchGenerationRunning = true;
    batchGeneratorThread = std::thread(&CPUScheduler::batchGenerator, this);
    std::cout << "Batch process generation started." << std::endl;
}

void CPUScheduler::stopBatchGeneration() {
    if (!batchGenerationRunning) {
        std::cout << "Batch process generation is not running." << std::endl;
        return;
    }
    
    batchGenerationRunning = false;
    if (batchGeneratorThread.joinable()) {
        batchGeneratorThread.join();
    }
    std::cout << "Batch process generation stopped." << std::endl;
}

void CPUScheduler::addProcess(const std::string& name, int memSize) {
    if (!initialized) {
        std::cout << "Please initialize the scheduler first." << std::endl;
        return;
    }

    int actualMemSize = memSize;
    if (memSize == -1) {
        // For batch generation, roll random power of 2 between min and max
        int minMem = config.getMinMemPerProc();
        int maxMem = config.getMaxMemPerProc();
        std::vector<int> powers;
        for (int p = 6; p <= 16; ++p) {
            int val = 1 << p;
            if (val >= minMem && val <= maxMem) powers.push_back(val);
        }
        if (!powers.empty()) {
            actualMemSize = powers[rand() % powers.size()];
        } else {
            actualMemSize = minMem;
        }
    }

    auto process = std::make_shared<Process>(name, processCounter++, actualMemSize);
    process->generateRandomInstructions(config.getMinIns(), config.getMaxIns());

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        readyQueue.push(process);
    }
    cv.notify_one();
}

ProcessPtr CPUScheduler::getProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    // Check running processes
    auto it = std::find_if(runningProcesses.begin(), runningProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != runningProcesses.end()) return *it;
    
    /*
    // Check finished processes
    it = std::find_if(finishedProcesses.begin(), finishedProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != finishedProcesses.end()) return *it;
    */

    // Check ready queue
    std::queue<ProcessPtr> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        auto process = tempQueue.front();
        tempQueue.pop();
        if (process->name == name) return process;
    }
    
    return nullptr;
}

ProcessPtr CPUScheduler::getAllProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    // Check running processes
    auto it = std::find_if(runningProcesses.begin(), runningProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != runningProcesses.end()) return *it;
    
    
    // Check finished processes
    it = std::find_if(finishedProcesses.begin(), finishedProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != finishedProcesses.end()) return *it;
    

    // Check ready queue
    std::queue<ProcessPtr> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        auto process = tempQueue.front();
        tempQueue.pop();
        if (process->name == name) return process;
    }
    
    return nullptr;
}

bool CPUScheduler::checkExistingProcess(const std::string& name) {

// std::unique_lock<std::mutex> testLock(schedulerMutex, std::defer_lock);

//     if (!testLock.try_lock()) {
//     std::cout << "[DEBUG] schedulerMutex is already locked!" << std::endl;
// } else {
//     std::cout << "[DEBUG] schedulerMutex was free. Lock acquired." << std::endl;
//     testLock.unlock();
// }

    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    auto it = std::find_if(runningProcesses.begin(), runningProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != runningProcesses.end()) return false;
    
    /*
    // Check finished processes
    it = std::find_if(finishedProcesses.begin(), finishedProcesses.end(),
        [&name](const ProcessPtr& process) { return process->name == name; });
    if (it != finishedProcesses.end()) return false;
    */

    std::queue<ProcessPtr> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        auto process = tempQueue.front();
        tempQueue.pop();
        if (process->name == name) return false;
    }
    
    return true; 
}

void CPUScheduler::listProcesses() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    double cpuUtilization = getCpuUtilization();
    int coresUsed = getCoresUsed();
    int coresAvailable = getCoresAvailable();
    
    std::cout << "CPU utilization: " << std::fixed << std::setprecision(2) 
              << cpuUtilization << "%" << std::endl;
    std::cout << "Cores used: " << coresUsed << std::endl;
    std::cout << "Cores available: " << coresAvailable << std::endl;
    std::cout << std::endl;
    
    std::cout << "Running processes:" << std::endl;
    for (const auto& process : runningProcesses) {
        std::cout << process->name << "\t(" << process->creationTime 
                  << ")\tCore: " << process->assignedCore << "\t"
                  << process->currentInstruction << " / " 
                  << process->totalInstructions << std::endl;
    }
    
    std::cout << std::endl << "Finished processes:" << std::endl;
    for (const auto& process : finishedProcesses) {
        std::cout << process->name << "\t(" << process->creationTime 
                  << ")\tFinished\t" << process->finishTime << "\t"
                  << process->totalInstructions << " / " 
                  << process->totalInstructions << std::endl;
    }
}

void CPUScheduler::generateReport() {
    std::ofstream file("csopesy-log.txt");
    if (!file.is_open()) {
        std::cout << "Error: Could not create csopesy-log.txt" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    double cpuUtilization = getCpuUtilization();
    int coresUsed = getCoresUsed();
    int coresAvailable = getCoresAvailable();
    
    file << "CPU utilization: " << std::fixed << std::setprecision(2) 
         << cpuUtilization << "%" << std::endl;
    file << "Cores used: " << coresUsed << std::endl;
    file << "Cores available: " << coresAvailable << std::endl;
    file << std::endl;
    
    file << "Running processes:" << std::endl;
    for (const auto& process : runningProcesses) {
        file << process->name << "\t(" << process->creationTime 
             << ")\tCore: " << process->assignedCore << "\t"
             << process->currentInstruction << " / " 
             << process->totalInstructions << std::endl;
    }
    
    file << std::endl << "Finished processes:" << std::endl;
    for (const auto& process : finishedProcesses) {
        file << process->name << "\t(" << process->creationTime 
             << ")\tFinished\t" << process->finishTime << "\t"
             << process->totalInstructions << " / " 
             << process->totalInstructions << std::endl;
    }
    
    file.close();
    std::cout << "Report generated: csopesy-log.txt" << std::endl;
}

void CPUScheduler::shutdown() {
    if (!schedulerRunning) return;
    
    schedulerRunning = false;
    batchGenerationRunning = false;
    cv.notify_all();
    
    for (auto& thread : coreThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    if (batchGeneratorThread.joinable()) {
        batchGeneratorThread.join();
    }
    
    coreThreads.clear();
    initialized = false;
}

void CPUScheduler::tickCounter() {
    while (schedulerRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cpuTicks++;
    }
}

void CPUScheduler::batchGenerator() {
    int lastTick = cpuTicks;
    while (batchGenerationRunning && schedulerRunning) {
        if (static_cast<unsigned long>(cpuTicks - lastTick) >= config.getBatchProcessFreq()) {
            std::string processName = "p" + std::to_string(processCounter++);
            // Roll random power of 2 between min and max for memory size
            int minMem = config.getMinMemPerProc();
            int maxMem = config.getMaxMemPerProc();
            std::vector<int> powers;
            for (int p = 6; p <= 16; ++p) {
                int val = 1 << p;
                if (val >= minMem && val <= maxMem) powers.push_back(val);
            }
            int memSize = minMem;
            if (!powers.empty()) memSize = powers[rand() % powers.size()];
            auto process = std::make_shared<Process>(processName, processCounter - 1, memSize);
            process->generateRandomInstructions(config.getMinIns(), config.getMaxIns());
            {
                std::lock_guard<std::mutex> lock(schedulerMutex);
                readyQueue.push(process);
            }
            cv.notify_one();
            lastTick = cpuTicks;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void CPUScheduler::coreWorker(int coreId) {
    while (schedulerRunning) {
        ProcessPtr process = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(schedulerMutex);
            cv.wait(lock, [this] { return !readyQueue.empty() || !schedulerRunning; });
            
            if (!schedulerRunning) break;
            
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();    
                process->assignedCore = coreId;
                process->remainingQuantum = config.getQuantumCycles();
                runningProcesses.push_back(process);
            }
        }

        // new addition
        if (process) {  
            // Try to allocate memory for the process
            if (!memoryManager.allocate(process)) {
                // Memory allocation failed, try to make space
                bool spaceFreed = false;
                
                {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    
                    // Look for processes in ready queue that have memory allocated
                    // and deallocate the first one found (FIFO order)
                    std::queue<ProcessPtr> tempQueue;
                    
                    while (!readyQueue.empty() && !spaceFreed) {
                        ProcessPtr waitingProcess = readyQueue.front();
                        readyQueue.pop();
                        
                        if (memoryManager.isAllocated(waitingProcess)) {
                            // Deallocate memory from this waiting process
                            memoryManager.deallocate(waitingProcess);
                            spaceFreed = true;
                        }
                        
                        tempQueue.push(waitingProcess);
                    }
                    
                    // Put all processes back in ready queue
                    while (!tempQueue.empty()) {
                        readyQueue.push(tempQueue.front());
                        tempQueue.pop();
                    }
                }
                
                // Try to allocate again after freeing space
                if (spaceFreed && !memoryManager.allocate(process)) {
                    // Still couldn't allocate, put process back in ready queue
                    {
                        std::lock_guard<std::mutex> lock(schedulerMutex);
                        readyQueue.push(process);
                        runningProcesses.erase(
                            std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                            runningProcesses.end()
                        );
                    }
                    cv.notify_one();
                    continue;
                } else if (!spaceFreed) {
                    // No space could be freed, put process back in ready queue
                    {
                        std::lock_guard<std::mutex> lock(schedulerMutex);
                        readyQueue.push(process);
                        runningProcesses.erase(
                            std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                            runningProcesses.end()
                        );
                    }
                    cv.notify_one();
                    continue;
                }
            }
        } else {
            continue;
        }
        
        if (process) {
            bool processRunning = true;
            while (processRunning && schedulerRunning) {
                bool stillRunning = process->executeNextInstruction(coreId);
                
                // Memory dump logic
                int newQuantumCycle = cpuTicks / config.getQuantumCycles();
                if (newQuantumCycle > currentQuantumCycle) {
                    currentQuantumCycle = newQuantumCycle;
                    quantumCycleCount++;
                    memoryManager.dumpStatusToFile(quantumCycleCount);
                }

                if (!stillRunning || process->isFinished) {
                    processRunning = false;
                } else if (config.getScheduler() == "rr") {
                    process->remainingQuantum--;
                    
                    if (process->remainingQuantum <= 0 && !process->isSleeping) {
                        bool shouldPreempt = false;
                        {
                            std::lock_guard<std::mutex> lock(schedulerMutex);
                            shouldPreempt = !readyQueue.empty();
                        }
                        
                        if (shouldPreempt) {
                            // It will keep its memory allocation while waiting in ready queue
                            {
                                std::lock_guard<std::mutex> lock(schedulerMutex);
                                readyQueue.push(process);
                            }
                            cv.notify_one();
                            processRunning = false;
                        } else {
                            process->remainingQuantum = config.getQuantumCycles();
                        }
                    }
                }
                
                if (config.getDelaysPerExec() > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.getDelaysPerExec() * 10));
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            {
                std::lock_guard<std::mutex> lock(schedulerMutex);
                runningProcesses.erase(
                    std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                    runningProcesses.end());
                
                if (process->isFinished) {
                    finishedProcesses.push_back(process);
                    process->assignedCore = -1;
                    // Only deallocate memory when process is truly finished
                    memoryManager.deallocate(process);
                }
            }
        }
    }
}

double CPUScheduler::getCpuUtilization() const {
    return static_cast<double>(runningProcesses.size()) / config.getNumCpu() * 100.0;
}

int CPUScheduler::getCoresUsed() const {
    return static_cast<int>(runningProcesses.size());
}

int CPUScheduler::getCoresAvailable() const {
    return config.getNumCpu() - static_cast<int>(runningProcesses.size());
}