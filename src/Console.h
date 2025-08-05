#pragma once
#include "CPUScheduler.h"
#include <string>

class Console {
public:
    Console() = default;
    void run();
    
private:
    CPUScheduler scheduler;

    struct ScreenContext {
    std::string name;
    int pid = -1;

    bool isEmpty() const {
        return name.empty() && pid == -1;
    }

    void clear() {
        name.clear();
        pid = -1;
    }
    };
    ScreenContext currentScreen;

    
    // Display methods
    void displayHeader();
    void displayProcessScreen();
    void displayProcessInfo();
    
    // Command handling
    void mainLoop();
    void handleMainCommand(const std::string& command);
    void handleScreenCommand(const std::string& command);
    void handleVmstatCommand(); // new
    
    // Utility
    void clearScreen();
    std::vector<std::string> parseCommand(const std::string& command);
};