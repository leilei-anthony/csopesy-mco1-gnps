#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>
#include <vector>
#include <algorithm>

void displayHeader() {
    std::cout << "  ____ ____   ___  ____  _____ ______   __\n";
    std::cout << " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n";
    std::cout << "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n";
    std::cout << "| |___ ___) | |_| |  __/| |___ ___) || |  \n";
    std::cout << " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";
    std::cout << "\nHello, Welcome to CSOPESY commandline!\n";
    std::cout << "Type 'exit' to quit, 'clear' to clear the screen\n";
}

void handleCommand() {
    std::vector<std::string> validCommands = {"initialize", "screen", "scheduler-test", "scheduler-stop", "report-util", "clear", "exit"};
    std::string command;

    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, command);

        // Check if the command is valid
        if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
            std::cout << command << " command recognized. Doing something." << std::endl;

            // Handle the command
            if (command == "initialize") {
            } else if (command == "screen") {
            } else if (command == "scheduler-test") {
            } else if (command == "scheduler-stop") {
            } else if (command == "report-util") {
            } else if (command == "clear") {
                system("cls");
            } else if (command == "exit") {
                break;
            }
        } else {
            std::cout << "Unrecognized command. Try again.\n";
        }
    }
}

int main()
{
    displayHeader();

    handleCommand();

    return 0;
}