#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <random>
#include <memory>

// Configuration structure
struct Config {
    int numCpu = 4;
    std::string scheduler = "fcfs";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delaysPerExec = 0;
};

// Process instruction types
enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> params;
    int sleepCycles = 0;
    int forRepeats = 0;
};

// Process representation
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
    std::vector<int> forLoopStack; // Stack for nested for loops
    std::vector<int> forLoopCounters; // Current iteration counters
    
    Process(const std::string& processName, int pid) 
        : name(processName), pid(pid), totalInstructions(0), currentInstruction(0),
          assignedCore(-1), isFinished(false), remainingQuantum(0), 
          sleepCounter(0), isSleeping(false) {
        
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y, %I:%M:%S %p");
        creationTime = ss.str();
    }
    
    void generateRandomInstructions(int minIns, int maxIns) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> instrDis(minIns, maxIns);
        std::uniform_int_distribution<> typeDis(0, 6);
        
        totalInstructions = instrDis(gen);
        instructions.clear();
        
        for (int i = 0; i < totalInstructions; i++) {
            Instruction instr;
            int type = typeDis(gen);
            
            switch (type) {
                case 0: // PRINT
                    instr.type = InstructionType::PRINT;
                    instr.params.push_back("Hello world from " + name + "!");
                    break;
                case 1: // DECLARE
                    instr.type = InstructionType::DECLARE;
                    instr.params.push_back("var" + std::to_string(i));
                    instr.params.push_back(std::to_string(gen() % 100));
                    break;
                case 2: // ADD
                    instr.type = InstructionType::ADD;
                    instr.params.push_back("result" + std::to_string(i));
                    instr.params.push_back("var1");
                    instr.params.push_back("var2");
                    break;
                case 3: // SUBTRACT
                    instr.type = InstructionType::SUBTRACT;
                    instr.params.push_back("result" + std::to_string(i));
                    instr.params.push_back("var1");
                    instr.params.push_back("var2");
                    break;
                case 4: // SLEEP
                    instr.type = InstructionType::SLEEP;
                    instr.sleepCycles = (gen() % 10) + 1;
                    break;
                case 5: // FOR_START (simplified)
                    instr.type = InstructionType::FOR_START;
                    instr.forRepeats = (gen() % 5) + 1;
                    break;
                case 6: // FOR_END
                    instr.type = InstructionType::FOR_END;
                    break;
            }
            
            instructions.push_back(instr);
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y, %I:%M:%S %p");
        return ss.str();
    }
    
    bool executeNextInstruction(int coreId) {
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
            return true; // Still running but sleeping
        }
        
        Instruction& instr = instructions[currentInstruction];
        
        switch (instr.type) {
            case InstructionType::PRINT: {
                std::stringstream logEntry;
                logEntry << "(" << getCurrentTimestamp() << ") Core:" << coreId 
                        << " \"" << instr.params[0] << "\"";
                printLogs.push_back(logEntry.str());
                break;
            }
            case InstructionType::DECLARE:
                if (instr.params.size() >= 2) {
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
        }
        
        currentInstruction++;
        return true;
    }
    
private:
    uint16_t getValue(const std::string& param) {
        // Check if it's a number
        if (std::isdigit(param[0])) {
            return static_cast<uint16_t>(std::stoi(param));
        }
        // Otherwise, it's a variable
        if (variables.find(param) == variables.end()) {
            variables[param] = 0; // Auto-declare with value 0
        }
        return variables[param];
    }
};

// CPU Scheduler implementation
class CPUScheduler {
private:
    Config config;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::vector<std::shared_ptr<Process>> runningProcesses;
    std::vector<std::shared_ptr<Process>> finishedProcesses;
    std::vector<std::thread> coreThreads;
    std::thread batchGeneratorThread;
    
    std::mutex schedulerMutex;
    std::condition_variable cv;
    std::atomic<bool> schedulerRunning{false};
    std::atomic<bool> batchGenerationRunning{false};
    std::atomic<int> cpuTicks{0};
    std::atomic<int> processCounter{1};
    
    bool initialized = false;
    
public:
    bool initialize() {
        if (initialized) {
            std::cout << "Scheduler already initialized." << std::endl;
            return false;
        }
        
        // Read config file
        if (!loadConfig()) {
            std::cout << "Error: Could not load config.txt" << std::endl;
            return false;
        }
        
        schedulerRunning = true;
        
        // Start CPU tick counter
        std::thread tickThread([this]() {
            while (schedulerRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                cpuTicks++;
            }
        });
        tickThread.detach();
        
        // Start core worker threads
        for (int i = 0; i < config.numCpu; i++) {
            coreThreads.emplace_back(&CPUScheduler::coreWorker, this, i);
        }
        
        initialized = true;
        std::cout << "Scheduler initialized with " << config.numCpu << " CPU cores using " 
                  << config.scheduler << " scheduling algorithm." << std::endl;
        return true;
    }
    
    void startBatchGeneration() {
        if (!initialized) {
            std::cout << "Please initialize the scheduler first." << std::endl;
            return;
        }
        
        if (batchGenerationRunning) {
            std::cout << "Batch process generation is already running." << std::endl;
            return;
        }
        
        batchGenerationRunning = true;
        batchGeneratorThread = std::thread([this]() {
            int lastTick = cpuTicks;
            while (batchGenerationRunning && schedulerRunning) {
                if (cpuTicks - lastTick >= config.batchProcessFreq) {
                    std::string processName = "p" + std::to_string(processCounter++);
                    auto process = std::make_shared<Process>(processName, processCounter - 1);
                    process->generateRandomInstructions(config.minIns, config.maxIns);
                    
                    {
                        std::lock_guard<std::mutex> lock(schedulerMutex);
                        readyQueue.push(process);
                    }
                    cv.notify_one();
                    lastTick = cpuTicks;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        std::cout << "Batch process generation started." << std::endl;
    }
    
    void stopBatchGeneration() {
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
    
    void addProcess(const std::string& name) {
        if (!initialized) {
            std::cout << "Please initialize the scheduler first." << std::endl;
            return;
        }
        
        auto process = std::make_shared<Process>(name, processCounter++);
        process->generateRandomInstructions(config.minIns, config.maxIns);
        
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);
            readyQueue.push(process);
        }
        cv.notify_one();
    }
    
    std::shared_ptr<Process> getProcess(const std::string& name) {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        
        // Check running processes
        for (const auto& process : runningProcesses) {
            if (process->name == name) return process;
        }
        
        // Check finished processes
        for (const auto& process : finishedProcesses) {
            if (process->name == name) return process;
        }
        
        // Check ready queue
        std::queue<std::shared_ptr<Process>> tempQueue = readyQueue;
        while (!tempQueue.empty()) {
            auto process = tempQueue.front();
            tempQueue.pop();
            if (process->name == name) return process;
        }
        
        return nullptr;
    }
    
    void listProcesses() {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        
        int coresUsed = runningProcesses.size();
        int coresAvailable = config.numCpu - coresUsed;
        double cpuUtilization = (double)coresUsed / config.numCpu * 100.0;
        
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
    
    void generateReport() {
        std::ofstream file("csopesy-log.txt");
        if (!file.is_open()) {
            std::cout << "Error: Could not create csopesy-log.txt" << std::endl;
            return;
        }
        
        std::lock_guard<std::mutex> lock(schedulerMutex);
        
        int coresUsed = runningProcesses.size();
        int coresAvailable = config.numCpu - coresUsed;
        double cpuUtilization = (double)coresUsed / config.numCpu * 100.0;
        
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
    
    void shutdown() {
        if (!schedulerRunning) return;
        
        schedulerRunning = false;
        batchGenerationRunning = false;
        cv.notify_all();
        
        // Join all threads
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
    
    ~CPUScheduler() {
        shutdown();
    }
    
private:
    bool loadConfig() {
        std::ifstream file("config.txt");
        if (!file.is_open()) {
            // Create default config file
            std::ofstream defaultFile("config.txt");
            if (defaultFile.is_open()) {
                defaultFile << "num-cpu 4\n";
                defaultFile << "scheduler fcfs\n";
                defaultFile << "quantum-cycles 5\n";
                defaultFile << "batch-process-freq 1\n";
                defaultFile << "min-ins 1000\n";
                defaultFile << "max-ins 2000\n";
                defaultFile << "delays-per-exec 0\n";
                defaultFile.close();
                std::cout << "Created default config.txt file." << std::endl;
            }
            file.open("config.txt");
        }
        
        if (!file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (iss >> key >> value) {
                if (key == "num-cpu") {
                    config.numCpu = std::stoi(value);
                } else if (key == "scheduler") {
                    config.scheduler = value;
                } else if (key == "quantum-cycles") {
                    config.quantumCycles = std::stoi(value);
                } else if (key == "batch-process-freq") {
                    config.batchProcessFreq = std::stoi(value);
                } else if (key == "min-ins") {
                    config.minIns = std::stoi(value);
                } else if (key == "max-ins") {
                    config.maxIns = std::stoi(value);
                } else if (key == "delays-per-exec") {
                    config.delaysPerExec = std::stoi(value);
                }
            }
        }
        
        file.close();
        return true;
    }
    
    void coreWorker(int coreId) {
        while (schedulerRunning) {
            std::shared_ptr<Process> process = nullptr;
            
            {
                std::unique_lock<std::mutex> lock(schedulerMutex);
                cv.wait(lock, [this] { return !readyQueue.empty() || !schedulerRunning; });
                
                if (!schedulerRunning) break;
                
                if (!readyQueue.empty()) {
                    process = readyQueue.front();
                    readyQueue.pop();
                    process->assignedCore = coreId;
                    process->remainingQuantum = config.quantumCycles;
                    runningProcesses.push_back(process);
                }
            }
            
            if (process) {
                // Execute process
                bool processRunning = true;
                while (processRunning && schedulerRunning) {
                    // Execute instruction
                    bool stillRunning = process->executeNextInstruction(coreId);
                    
                    if (!stillRunning || process->isFinished) {
                        processRunning = false;
                    } else if (config.scheduler == "rr") {
                        // Round-robin quantum check
                        process->remainingQuantum--;
                        if (process->remainingQuantum <= 0 && !process->isSleeping) {
                            // Time slice expired, preempt
                            {
                                std::lock_guard<std::mutex> lock(schedulerMutex);
                                readyQueue.push(process);
                            }
                            cv.notify_one();
                            processRunning = false;
                        }
                    }
                    
                    // Simulate execution delay
                    if (config.delaysPerExec > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(config.delaysPerExec * 10));
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
                
                // Remove from running processes
                {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    runningProcesses.erase(
                        std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                        runningProcesses.end());
                    
                    if (process->isFinished) {
                        finishedProcesses.push_back(process);
                        process->assignedCore = -1;
                    }
                }
            }
        }
    }
};

// Console implementation
class Console {
private:
    CPUScheduler scheduler;
    std::string currentScreen;
    bool initialized = false;
    
public:
    void run() {
        displayHeader();
        mainLoop();
    }
    
private:
    void displayHeader() {
        std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
        std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
        std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
        std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
        std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
        std::cout << "\nHello, Welcome to CSOPESY commandline!\n";
        std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
    }
    
    void mainLoop() {
        std::string input;
        
        while (true) {
            if (currentScreen.empty()) {
                std::cout << "root:\\> ";
            } else {
                std::cout << currentScreen << ":\\> ";
            }
            
            std::getline(std::cin, input);
            
            if (currentScreen.empty()) {
                handleMainCommand(input);
            } else {
                handleScreenCommand(input);
            }
        }
    }
    
    void handleMainCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "initialize") {
            if (scheduler.initialize()) {
                initialized = true;
            }
        } else if (cmd == "exit") {
            scheduler.shutdown();
            std::exit(0);
        } else if (cmd == "clear") {
            system("cls");
            displayHeader();
        } else if (!initialized && cmd != "exit") {
            std::cout << "Please run 'initialize' first." << std::endl;
        } else if (cmd == "screen") {
            handleScreenCommand(command);
        } else if (cmd == "scheduler-start") {
            scheduler.startBatchGeneration();
        } else if (cmd == "scheduler-stop") {
            scheduler.stopBatchGeneration();
        } else if (cmd == "report-util") {
            scheduler.generateReport();
        } else {
            std::cout << "Command not recognized." << std::endl;
        }
    }
    
    void handleScreenCommand(const std::string& command) {
        if (currentScreen.empty()) {
            // Parse screen command from main menu
            std::istringstream iss(command);
            std::string screen, option, processName;
            iss >> screen >> option >> processName;
            
            if (screen == "screen") {
                if (option == "-s") {
                    // Create new screen/process
                    scheduler.addProcess(processName);
                    currentScreen = processName;
                    displayProcessScreen();
                } else if (option == "-r") {
                    // Resume existing screen
                    auto process = scheduler.getProcess(processName);
                    if (process) {
                        currentScreen = processName;
                        displayProcessScreen();
                    } else {
                        std::cout << "Process " << processName << " not found." << std::endl;
                    }
                } else if (option == "-ls") {
                    scheduler.listProcesses();
                } else {
                    std::cout << "Invalid screen option. Use -s, -r, or -ls." << std::endl;
                }
            }
        } else {
            // Handle commands within screen
            if (command == "exit") {
                currentScreen.clear();
                system("cls");
                displayHeader();
            } else if (command == "process-smi") {
                displayProcessInfo();
            } else {
                std::cout << "Command '" << command << "' not recognized in screen mode." << std::endl;
            }
        }
    }
    
    void displayProcessScreen() {
        system("cls");
        std::cout << "Process Screen: " << currentScreen << std::endl;
        std::cout << "===============================================" << std::endl;
        displayProcessInfo();
        std::cout << "===============================================" << std::endl;
        std::cout << "Type 'process-smi' for process info, 'exit' to return to main menu." << std::endl;
    }
    
    void displayProcessInfo() {
        auto process = scheduler.getProcess(currentScreen);
        if (!process) {
            std::cout << "Process " << currentScreen << " not found." << std::endl;
            currentScreen.clear();
            return;
        }
        
        std::cout << "Process name: " << process->name << std::endl;
        std::cout << "ID: " << process->pid << std::endl;
        
        if (process->isFinished) {
            std::cout << "Finished!" << std::endl;
        } else {
            std::cout << "Current instruction line: " << process->currentInstruction << std::endl;
            std::cout << "Lines of code: " << process->totalInstructions << std::endl;
        }
        
        std::cout << std::endl << "Logs:" << std::endl;
        for (const auto& log : process->printLogs) {
            std::cout << log << std::endl;
        }
    }
};

int main() {
    Console console;
    console.run();
    return 0;
}