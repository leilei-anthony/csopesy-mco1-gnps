#pragma once
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <deque>
#include <algorithm>
#include "Process.h"

class MemoryFrame {
public:
    int frameId;
    int ownerPid = -1;
    int virtualPage = -1; // Track which virtual page this frame maps
    bool occupied = false;

    MemoryFrame(int id) : frameId(id) {}
};

class FirstFitMemoryAllocator {
private:
    std::vector<MemoryFrame> memory;
    int memPerFrame;
    int totalFrames;
    int totalMemory;
    int pageIns;
    int pageOuts;
    
    std::map<int, std::map<int, int>> pageTables; // pid -> {virtualPage -> frameId}
    std::deque<int> fifoQueue; // for FIFO page replacement

    std::string backingStoreFile = "csopesy-backing-store.txt";

public:

    void init(int maxMemory, int frameSize, int procLimit);
    std::vector<int> findAnyFreeFrames(int count);
    bool allocate(const std::shared_ptr<Process>& proc);
    void deallocate(const std::shared_ptr<Process>& proc);
    bool isAllocated(int pid) const;
    bool isAllocated(const ProcessPtr& process) const;
    void dumpStatusToFile(int quantumCycle) const;
    bool writeMemory(int pid, uint16_t address, uint16_t value, std::string& errOut);
    bool readMemory(int pid, uint16_t address, uint16_t& outValue, std::string& errOut);
    int ensurePageMapped(int pid, int virtualPage, std::string& errOut);
    int findFreeFrame();
    void markAccessViolation(std::string& errOut, uint16_t badAddr);
    bool isValidAddress(uint32_t addr);

    int getPageIns() const;
    int getPageOuts() const;

    int getMemPerFrame() const { return memPerFrame; }
    int getTotalMemory() const { return totalMemory; }
    int getUsedMemory() const {
        int used = 0;
        for (const auto& frame : memory) {
            if (frame.ownerPid != -1) used++;
        }
        return used * memPerFrame;
    }
    int getFreeMemory() const {
        int free = 0;
        for (const auto& frame : memory) {
            if (frame.ownerPid == -1) free++;
        }
        return free * memPerFrame;
    }
};
