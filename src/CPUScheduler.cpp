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
    std::cout << cpuTicks << " CPU ticks accumulated." << std::endl;
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

ProcessPtr CPUScheduler::getProcessByPID(int pid) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    // Check running processes
    auto it = std::find_if(runningProcesses.begin(), runningProcesses.end(),
        [pid](const ProcessPtr& process) { return process->pid == pid; });
    if (it != runningProcesses.end()) return *it;

    // Check finished processes
    it = std::find_if(finishedProcesses.begin(), finishedProcesses.end(),
        [pid](const ProcessPtr& process) { return process->pid == pid; });
    if (it != finishedProcesses.end()) return *it;

    // Check ready queue
    std::queue<ProcessPtr> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        auto process = tempQueue.front();
        tempQueue.pop();
        if (process->pid == pid) return process;
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
        std::cout << process->name << " pid: " << process->pid << "\t(" << process->creationTime 
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
        cpuTicks += config.getNumCpu();
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
                // Mark as active tick
                {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    activeCpuTicks++;
                }
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


void CPUScheduler::printVmstat() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    // Snapshot values to avoid inconsistencies during computation
    const auto totalMem   = memoryManager.getTotalMemory();
    const auto usedMem    = memoryManager.getUsedMemory();
    const auto freeMem    = memoryManager.getFreeMemory();
    const auto totalTicks = cpuTicks.load();
    const auto active     = activeCpuTicks.load();
    const auto idle       = totalTicks - active;

    std::cout << "\n=== VMSTAT REPORT ===\n\n";
    std::cout << std::left << std::setw(20) << "Total memory:"      << totalMem << " bytes\n";
    std::cout << std::left << std::setw(20) << "Used memory:"       << usedMem  << " bytes\n";
    std::cout << std::left << std::setw(20) << "Free memory:"       << freeMem  << " bytes\n\n";

    std::cout << std::left << std::setw(20) << "Idle CPU ticks:"    << idle     << "\n";
    std::cout << std::left << std::setw(20) << "Active CPU ticks:"  << active   << "\n";
    std::cout << std::left << std::setw(20) << "Total CPU ticks:"   << totalTicks << "\n\n";

    std::cout << std::left << std::setw(20) << "Num paged in:"      << "0 (not implemented)\n";
    std::cout << std::left << std::setw(20) << "Num paged out:"     << "0 (not implemented)\n";

    std::cout << "\n======================\n";
}

std::vector<ProcessPtr> CPUScheduler::listAllProcesses() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    std::vector<ProcessPtr> all;

    all.insert(all.end(), runningProcesses.begin(), runningProcesses.end());
    all.insert(all.end(), finishedProcesses.begin(), finishedProcesses.end());

    std::queue<ProcessPtr> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        all.push_back(tempQueue.front());
        tempQueue.pop();
    }

    return all;
}




void CPUScheduler::printProcessSMI() {
    std::cout << "+-----------------------------------------------------------------------------+\n";
    std::cout << "|                          Process Memory Management                          |\n";
    std::cout << "+-------------------------------+----------------------+----------------------+\n";

    size_t totalMem = memoryManager.getTotalMemory();       // in bytes
    size_t usedMem = memoryManager.getUsedMemory();         // in bytes
    size_t freeMem = totalMem - usedMem;

    std::cout << "| Total Memory: " << std::setw(12) << totalMem
              << " B | Used Memory: " << std::setw(12) << usedMem
              << " B | Free Memory: " << std::setw(12) << freeMem << " B |\n";

    std::cout << "+-------------------------------+----------------------+----------------------+\n";
    std::cout << "| PID  | Process Name   | Pages | Mem Usage (B) | Status    | Start Time |\n";
    std::cout << "|------|----------------|-------|----------------|-----------|------------|\n";

    //  list of processes and memory occupied

    std::cout << "+-----------------------------------------------------------------------------+\n";
}



bool CPUScheduler::addProcessWithInstructions(const std::string& name, int memSize, const std::string& instructions) {
    if (!initialized) {
        std::cout << "Please initialize the scheduler first." << std::endl;
        return false;
    }

    // Check if process already exists
    if (!checkExistingProcess(name)) {
        std::cout << "Process " << name << " already exists." << std::endl;
        return false;
    }

    // Create new process with specified memory size
    auto process = std::make_shared<Process>(name, processCounter++, memSize);

    // Parse and set custom instructions
    if (!process->parseUserInstructions(instructions)) {
        std::cout << "Error parsing instructions for process " << name << std::endl;
        return false;
    }

    // Add to ready queue
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        readyQueue.push(process);
    }
    cv.notify_one();

    return true;
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