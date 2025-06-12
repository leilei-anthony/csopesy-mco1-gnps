#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <atomic>

struct Process {
    std::string name;
    int totalInstructions;
    int currentInstruction;
    std::string creationTimestamp;
    std::string finishTimestamp;
    int assignedCore;
    bool isFinished;
    std::vector<std::string> printLogs;
    
    Process(const std::string& processName, int instructions) 
        : name(processName), totalInstructions(instructions), currentInstruction(0),
          assignedCore(-1), isFinished(false) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y %H:%M:%S%p");
        creationTimestamp = ss.str();
    }
};

class ProcessScheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::vector<std::shared_ptr<Process>> runningProcesses;
    std::vector<std::shared_ptr<Process>> finishedProcesses;
    std::vector<std::thread> coreThreads;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> schedulerRunning{false};
    std::atomic<bool> testMode{false};
    int numCores;
    std::thread schedulerThread;
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y %H:%M:%S%p");
        return ss.str();
    }
    
    void coreWorker(int coreId) {
        while (schedulerRunning) {
            std::shared_ptr<Process> process = nullptr;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this] { return !readyQueue.empty() || !schedulerRunning; });
                
                if (!schedulerRunning) break;
                
                if (!readyQueue.empty()) {
                    process = readyQueue.front();
                    readyQueue.pop();
                    process->assignedCore = coreId;
                    runningProcesses.push_back(process);
                }
            }
            
            if (process) {
                // Execute process instructions
                while (process->currentInstruction < process->totalInstructions && schedulerRunning) {
                    process->currentInstruction++;
                    
                    // Add print log entry
                    std::stringstream logEntry;
                    logEntry << "(" << getCurrentTimestamp() << ") Core:" << coreId 
                            << " \"Hello world from " << process->name << "!\"";
                    process->printLogs.push_back(logEntry.str());
                    
                    // Simulate instruction execution time
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                // Mark process as finished
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    process->isFinished = true;
                    process->finishTimestamp = getCurrentTimestamp();
                    process->assignedCore = -1;
                    
                    // Remove from running processes
                    runningProcesses.erase(
                        std::remove(runningProcesses.begin(), runningProcesses.end(), process),
                        runningProcesses.end());
                    
                    finishedProcesses.push_back(process);
                }
                
                // Generate text file for process
                generateProcessFile(process);
            }
        }
    }
    
    void generateProcessFile(std::shared_ptr<Process> process) {
        std::string filename = process->name + ".txt";
        std::ofstream file(filename);
        
        if (file.is_open()) {
            file << "Process name: " << process->name << std::endl;
            file << "Logs:" << std::endl;
            file << std::endl;
            
            for (const auto& log : process->printLogs) {
                file << log << std::endl;
            }
            
            file.close();
        }
    }
    
public:
    ProcessScheduler(int cores = 4) : numCores(cores) {}
    
    void initialize() {
        if (schedulerRunning) {
            std::cout << "Scheduler is already running." << std::endl;
            return;
        }
        
        schedulerRunning = true;
        
        // Start core worker threads
        for (int i = 0; i < numCores; i++) {
            coreThreads.emplace_back(&ProcessScheduler::coreWorker, this, i);
        }
        
        std::cout << "Scheduler initialized with " << numCores << " cores." << std::endl;
    }
    
    void stop() {
        if (!schedulerRunning) {
            std::cout << "Scheduler is not running." << std::endl;
            return;
        }
        
        schedulerRunning = false;
        testMode = false;
        cv.notify_all();
        
        // Join all threads
        for (auto& thread : coreThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }
        
        coreThreads.clear();
        std::cout << "Scheduler stopped." << std::endl;
    }
    
    void startTest() {
        if (!schedulerRunning) {
            std::cout << "Please initialize the scheduler first." << std::endl;
            return;
        }
        
        if (testMode) {
            std::cout << "Test is already running." << std::endl;
            return;
        }
        
        testMode = true;
        
        // Start test thread that creates processes periodically  
        schedulerThread = std::thread([this]() {
            int processCounter = 1;
            while (testMode && schedulerRunning) {
                std::string processName = "process" + std::to_string(processCounter++);
                addProcess(processName, 100);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        
        std::cout << "Scheduler test started." << std::endl;
    }
    
    void stopTest() {
        testMode = false;
        std::cout << "Scheduler test stopped." << std::endl;
    }
    
    void addProcess(const std::string& name, int instructions) {
        auto process = std::make_shared<Process>(name, instructions);
        
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(process);
        cv.notify_one();
    }
    
    void reportUtil() {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        std::cout << "------------------------------------" << std::endl;
        std::cout << "Running processes:" << std::endl;
        
        for (const auto& process : runningProcesses) {
            std::cout << process->name << "\t(" 
                     << process->creationTimestamp << ")\tCore: " 
                     << process->assignedCore << "\t" 
                     << process->currentInstruction << " / " 
                     << process->totalInstructions << std::endl;
        }
        
        std::cout << std::endl << "Finished processes:" << std::endl;
        for (const auto& process : finishedProcesses) {
            std::cout << process->name << "\t(" 
                     << process->creationTimestamp << ")\tFinished\t" 
                     << process->totalInstructions << " / " 
                     << process->totalInstructions << std::endl;
        }
        std::cout << "------------------------------------" << std::endl;
    }
    
    std::shared_ptr<Process> getProcess(const std::string& name) {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        // Check running processes
        for (const auto& process : runningProcesses) {
            if (process->name == name) return process;
        }
        
        // Check finished processes  
        for (const auto& process : finishedProcesses) {
            if (process->name == name) return process;
        }
        
        return nullptr;
    }
    
    ~ProcessScheduler() {
        stop();
    }
};

struct ScreenInfo {
    std::string processName;
    int currentLine;
    int totalLines;
    std::string creationTimestamp;
};

class Console {
private: 
    std::map<std::string, ScreenInfo> screens;
    std::string activeScreenName;
    ProcessScheduler scheduler;

public:
    void run();

private:
    void displayHeader();
    std::string getCurrentTimestamp();
    void drawScreen(const ScreenInfo& info);
    void mainMenuLoop();
    void screenLoop();
    void handleMainCommand(const std::string& command);
    void handleScreenCommand(const std::string& command);
};

void Console::displayHeader() {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "\nHello, Welcome to CSOPESY commandline!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
}

std::string Console::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y, %I:%M:%S %p");
    return ss.str();
}

void Console::drawScreen(const ScreenInfo& info) {
    system("cls");
    std::cout << "------------------------------------------\n";
    std::cout << "SCREEN: " << info.processName << "\n";
    std::cout << "------------------------------------------\n";
    
    // Get actual process info from scheduler
    auto process = scheduler.getProcess(info.processName);
    if (process) {
        std::cout << "Process Name: " << process->name << "\n";
        std::cout << "Current Line of Instruction: " << process->currentInstruction << "\n";
        std::cout << "Total Line of Instruction: " << process->totalInstructions << "\n";
        std::cout << "Timestamp: " << process->creationTimestamp << "\n";
    } else {
        std::cout << "Process Name: " << info.processName << "\n";
        std::cout << "Current Line of Instruction: " << info.currentLine << "\n";
        std::cout << "Total Line of Instruction: " << info.totalLines << "\n";
        std::cout << "Timestamp: " << info.creationTimestamp << "\n";
    }
    
    std::cout << "------------------------------------------\n";
    std::cout << "Type 'exit' to return to main menu.\n";
}

void Console::run() {
    displayHeader();
    while (true) {
        if (activeScreenName.empty()) {
            mainMenuLoop();
        } else {
            screenLoop();
        }
    }
}

void Console::mainMenuLoop() {
    std::string command;
    std::cout << ">> ";
    std::getline(std::cin, command);
    handleMainCommand(command);
}

void Console::screenLoop() {
    std::string command;
    std::cout << "[" << activeScreenName << "]>> ";
    std::getline(std::cin, command);
    handleScreenCommand(command);
}

void Console::handleMainCommand(const std::string& command) {
    std::vector<std::string> validCommands = {"initialize", "screen", "scheduler-test", "scheduler-stop", "report-util", "clear", "exit"};

    if (command.rfind("screen ", 0) == 0) {
        std::string screen_command = command.substr(7);

        if (screen_command.rfind("-r ", 0) == 0) {
            std::string screen_name = screen_command.substr(3);
            if (screens.count(screen_name)) {
                activeScreenName = screen_name;
                drawScreen(screens[activeScreenName]);
            } else {
                std::cout << "Screen '" << screen_name << "' not found.\n";
            }
        } else if (screen_command.rfind("-s ", 0) == 0) {
            std::string screen_name = screen_command.substr(3);
            if (screens.count(screen_name)) {
                std::cout << "Screen '" << screen_name << "' already exists. Switching to it.\n";
            } else {
                ScreenInfo newScreen;
                newScreen.processName = screen_name;
                newScreen.currentLine = 0;
                newScreen.totalLines = 100;
                newScreen.creationTimestamp = getCurrentTimestamp();
                screens[screen_name] = newScreen;
                
                // Also add process to scheduler
                scheduler.addProcess(screen_name, 100);
                std::cout << "Created new screen: " << screen_name << "\n";
            }
            activeScreenName = screen_name;
            drawScreen(screens[activeScreenName]);
        } else {
            std::cout << "Invalid 'screen' command. Use 'screen -r <name>' or 'screen -s <name>'.\n";
        }
    } else if (command == "initialize") {
        scheduler.initialize();
    } else if (command == "scheduler-test") {
        scheduler.startTest();
    } else if (command == "scheduler-stop") {
        scheduler.stopTest();
    } else if (command == "report-util") {
        scheduler.reportUtil();
    } else if (command == "clear") {
        system("cls");
        displayHeader();
    } else if (command == "exit") {
        scheduler.stop();
        std::exit(0);
    } else {
        std::cout << "Unrecognized command. Try again.\n";
    }
}

void Console::handleScreenCommand(const std::string& command) {
    if (command == "exit") {
        activeScreenName.clear();
        system("cls");
        displayHeader();
    } else {
        std::cout << "Command '" << command << "' executed within screen '" << activeScreenName << "'.\n";
        if (screens.count(activeScreenName)) {
            screens[activeScreenName].currentLine++;
            if (screens[activeScreenName].currentLine > screens[activeScreenName].totalLines) {
                screens[activeScreenName].currentLine = 1;
            }
            drawScreen(screens[activeScreenName]);
        }
    }
}

int main() {
    Console console;
    console.run();
    return 0;
}