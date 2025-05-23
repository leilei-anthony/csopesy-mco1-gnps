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

void displayHeader() {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "\nHello, Welcome to CSOPESY commandline!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
}

struct ScreenInfo {
    std::string processName;
    int currentLine;
    int totalLines;
    std::string creationTimestamp;
};

// The created screens are stored in this map
std::map<std::string, ScreenInfo> screens;
// Current active screen (empty string if in main menu)
std::string activeScreenName = "";

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%m/%d/%Y, %I:%M:%S %p");
    return ss.str();
}

void drawScreen(const ScreenInfo& info) {
    system("cls");
    std::cout << "------------------------------------------\n";
    std::cout << "SCREEN: " << info.processName << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Process Name: " << info.processName << "\n";
    std::cout << "Current Line of Instruction: " << info.currentLine << "\n";
    std::cout << "Total Line of Instruction: " << info.totalLines << "\n";
    std::cout << "Timestamp: " << info.creationTimestamp << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Type 'exit' to return to main menu.\n";
}


void handleCommand() {
    std::vector<std::string> validCommands = {"initialize", "screen", "scheduler-test", "scheduler-stop", "report-util", "clear", "exit"};
    std::string command;

    while (true) {
        // In main menu
        if (activeScreenName.empty()) {
            std::cout << ">> ";
            std::getline(std::cin, command);

            // screen commands
            // Parse command for "screen -r <name>" or "screen -s <name>"
            std::string cmd_prefix = command.substr(0, 7); // "screen "
            if (cmd_prefix == "screen ") {
                std::string screen_command = command.substr(7); // "-r <name>" or "-s <name>"

                // Starts with "-r "
                if (screen_command.rfind("-r ", 0) == 0) {
                    std::string screen_name = screen_command.substr(3);
                    if (screens.count(screen_name)) {
                        activeScreenName = screen_name;
                        drawScreen(screens[activeScreenName]);
                    } else {
                        std::cout << "Screen '" << screen_name << "' not found.\n";
                    }
                } 
                
                // Starts with "-s "
                else if (screen_command.rfind("-s ", 0) == 0) {
                    std::string screen_name = screen_command.substr(3);
                    if (screens.count(screen_name)) {
                        std::cout << "Screen '" << screen_name << "' already exists. Switching to it.\n";
                        activeScreenName = screen_name;
                        drawScreen(screens[activeScreenName]);
                    } 
                    
                    else {
                        // Create a new screen
                        ScreenInfo newScreen;
                        newScreen.processName = screen_name;
                        newScreen.currentLine = 0; // Placeholder
                        newScreen.totalLines = 100; // Placeholder
                        newScreen.creationTimestamp = getCurrentTimestamp();
                        screens[screen_name] = newScreen;
                        activeScreenName = screen_name;
                        drawScreen(screens[activeScreenName]);
                    }
                } 
                
                else {
                    std::cout << "Invalid 'screen' command. Use 'screen -r <name>' or 'screen -s <name>'.\n";
                }
            } 
            
            else {
                // Check if the command is valid from the main menu
                if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
                    std::cout << command << " command recognized. Doing something." << std::endl;

                    // Handle the command
                    if (command == "initialize") {
                        // Placeholder for initialize logic
                    } else if (command == "scheduler-test") {
                        // Placeholder for scheduler-test logic
                    } else if (command == "scheduler-stop") {
                        // Placeholder for scheduler-stop logic
                    } else if (command == "report-util") {
                        // Placeholder for report-util logic
                    } else if (command == "clear") {
                        system("cls");
                        displayHeader();
                    } else if (command == "exit") {
                        break;
                    }
                } else {
                    std::cout << "Unrecognized command. Try again.\n";
                }
            }
        } 
        
        // In a screen
        else {
            std::cout << "[" << activeScreenName << "]>> ";
            std::getline(std::cin, command);

            if (command == "exit") {
                activeScreenName = ""; // Return to main menu
                system("cls");
                displayHeader(); // Redraw header for main menu
            } else {
                // Placeholder for commands within a screen
                std::cout << "Command '" << command << "' executed within screen '" << activeScreenName << "'.\n";
                screens[activeScreenName].currentLine++;
                if (screens[activeScreenName].currentLine > screens[activeScreenName].totalLines) {
                    screens[activeScreenName].currentLine = 1; // Loop around or handle end of process
                }
                drawScreen(screens[activeScreenName]); // Redraw screen after command
            }
        }
    }
}

int main()
{
    displayHeader();

    handleCommand();

    return 0;
}