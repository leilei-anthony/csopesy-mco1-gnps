#include "Console.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

void Console::run() {
    displayHeader();
    mainLoop();
}

void Console::displayHeader() {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "\nHello, Welcome to CSOPESY commandline!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
}

void Console::mainLoop() {
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

void Console::handleMainCommand(const std::string& command) {
    auto tokens = parseCommand(command);
    if (tokens.empty()) return;
    
    const std::string& cmd = tokens[0];
    
    if (cmd == "initialize") {
        scheduler.initialize();
    } else if (cmd == "exit") {
        scheduler.shutdown();
        std::exit(0);
    } else if (cmd == "clear") {
        clearScreen();
        displayHeader();
    } else if (!scheduler.isInitialized() && cmd != "exit") {
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

void Console::handleScreenCommand(const std::string& command) {
    if (currentScreen.empty()) {
        auto tokens = parseCommand(command);
        if (tokens.size() < 2 || tokens[0] != "screen") {
            std::cout << "Invalid screen command format." << std::endl;
            return;
        }
        
        const std::string& option = tokens[1];
        
        if (option == "-s" && tokens.size() >= 4) {
            const std::string& processName = tokens[2];
            int memSize = std::stoi(tokens[3]);

            std::cout << "Adding process: " << processName << " with memory size: " << memSize << std::endl;

            // Validate memory size: power of 2 and in [64, 65536]
            if (memSize < 64 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
                std::cout << "invalid memory allocation" << std::endl;
                return;
            }

            std::cout << !scheduler.checkExistingProcess(processName);

            if (!scheduler.checkExistingProcess(processName)) {
                std::cout << "here if";
                currentScreen = processName;
                displayProcessScreen();
                return;
            } else {
                std::cout << "here else";
                scheduler.addProcess(processName, memSize);
                std::cout << "Process " << processName << " added with memory size: " << memSize << " bytes." << std::endl;
                currentScreen = processName;
                displayProcessScreen();
            }

        } else if (option == "-r" && tokens.size() >= 3) {
            const std::string& processName = tokens[2];
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
    } else {
        if (command == "exit") {
            currentScreen.clear();
            clearScreen();
            displayHeader();
        } else if (command == "process-smi") {
            displayProcessInfo();
        } else {
            std::cout << "Command '" << command << "' not recognized in screen mode." << std::endl;
        }
    }
}

void Console::displayProcessScreen() {
    clearScreen();
    std::cout << "Process Screen: " << currentScreen << std::endl;
    std::cout << "===============================================" << std::endl;
    displayProcessInfo();
    std::cout << "===============================================" << std::endl;
    std::cout << "Type 'process-smi' for process info, 'exit' to return to main menu." << std::endl;
}

void Console::displayProcessInfo() {
    auto process = scheduler.getAllProcess(currentScreen);
    if (!process) {
        std::cout << "Process " << currentScreen << " not found." << std::endl;
        currentScreen.clear();
        return;
    }
    
    std::cout << "Process name: " << process->name << std::endl;
    std::cout << "ID: " << process->pid << std::endl;
    
    std::cout << "Logs:" << std::endl;
    for (const auto& log : process->printLogs) {
        std::cout << log << std::endl;
    }

    if (process->isFinished) {
        std::cout << "\nFinished!\n" << std::endl;
    } else {
        std::cout << "\nCurrent instruction line: " << process->currentInstruction << std::endl;
        std::cout << "Lines of code\n: " << process->totalInstructions << std::endl;
    }
    
    
}

void Console::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

std::vector<std::string> Console::parseCommand(const std::string& command) {
    
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}