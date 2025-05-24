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

struct ScreenInfo {
    std::string processName;
    int currentLine;
    int totalLines;
    std::string creationTimestamp;
};

class Console {
    // Attributes
private: 
    std::map<std::string, ScreenInfo> screens;
    std::string activeScreenName;

    // Methods
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
    std::cout << "Process Name: " << info.processName << "\n";
    std::cout << "Current Line of Instruction: " << info.currentLine << "\n";
    std::cout << "Total Line of Instruction: " << info.totalLines << "\n";
    std::cout << "Timestamp: " << info.creationTimestamp << "\n";
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
        std::string screen_command = command.substr(7); // "-r <name>" or "-s <name>"

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
                std::cout << "Press Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            } else {
                ScreenInfo newScreen;
                newScreen.processName = screen_name;
                newScreen.currentLine = 0;
                newScreen.totalLines = 100;
                newScreen.creationTimestamp = getCurrentTimestamp();
                screens[screen_name] = newScreen;
                std::cout << "Created new screen: " << screen_name << "\n";
            }
            activeScreenName = screen_name;
            drawScreen(screens[activeScreenName]);
        } else {
            std::cout << "Invalid 'screen' command. Use 'screen -r <name>' or 'screen -s <name>'.\n";
        }
    } else if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
        std::cout << command << " command recognized. Doing something." << std::endl;

        if (command == "clear") {
            system("cls");
            displayHeader();
        } else if (command == "exit") {
            std::exit(0);
        }
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
        screens[activeScreenName].currentLine++;
        if (screens[activeScreenName].currentLine > screens[activeScreenName].totalLines) {
            screens[activeScreenName].currentLine = 1;
        }
        drawScreen(screens[activeScreenName]);
    }
}

int main() {
    Console console;
    console.run();
    return 0;
}
