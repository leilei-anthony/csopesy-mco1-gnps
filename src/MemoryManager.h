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

    std::map<int, std::map<int, int>> pageTables; // pid -> {virtualPage -> frameId}
    std::deque<int> fifoQueue; // for FIFO page replacement

    const int pageSize = 256; // Example page size, can be configurable
    std::string backingStoreFile = "csopesy-backing-store.txt";

public:

    void init(int maxMemory, int frameSize, int procLimit);
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
    bool isValidAddress(uint16_t addr);

    int getMemPerFrame() const { return memPerFrame; }
    int getTotalMemory() const { return totalMemory; }
};
