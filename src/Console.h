#pragma once
#include "CPUScheduler.h"
#include <string>

class Console {
public:
    Console() = default;
    void run();
    
private:
    CPUScheduler scheduler;
    std::string currentScreen;
    
    // Display methods
    void displayHeader();
    void displayProcessScreen();
    void displayProcessInfo();
    
    // Command handling
    void mainLoop();
    void handleMainCommand(const std::string& command);
    void handleScreenCommand(const std::string& command);
    
    // Utility
    void clearScreen();
    std::vector<std::string> parseCommand(const std::string& command);
};