#include "MemoryManager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

void FirstFitMemoryAllocator::init(int maxMemory, int frameSize, int /*procLimit*/) {
    totalMemory = maxMemory;
    memPerFrame = frameSize;
    totalFrames = totalMemory / memPerFrame;

    memory.clear();
    pageTables.clear();
    fifoQueue.clear();

    for (int i = 0; i < totalFrames; ++i) {
        memory.emplace_back(i);
    }

    // Clear backing store
    std::ofstream(backingStoreFile).close();
}

bool FirstFitMemoryAllocator::allocate(const std::shared_ptr<Process>& proc) {
    // Step 1: Create an empty page table
    pageTables[proc->pid] = {};

    // Step 2 (Optional): Log to backing store to simulate "loading" instructions from disk
    std::ofstream store(backingStoreFile, std::ios::app);
    store << "ALLOC pid=" << proc->pid
          << " mem=" << proc->memorySize
          << " pages=" << (proc->memorySize + pageSize - 1) / pageSize
          << "\n";

    return true;
}

void FirstFitMemoryAllocator::deallocate(const std::shared_ptr<Process>& proc) {
    for (auto& frame : memory) {
        if (frame.ownerPid == proc->pid) {
            frame.ownerPid = -1;
            frame.virtualPage = -1;
        }
    }

    pageTables.erase(proc->pid);

    fifoQueue.erase(std::remove_if(fifoQueue.begin(), fifoQueue.end(),
        [&](int fId) { return memory[fId].ownerPid == proc->pid; }), fifoQueue.end());
}

void FirstFitMemoryAllocator::dumpStatusToFile(int cycle) const {
    // Sample implementation
    std::ofstream out("mem_status.txt", std::ios::app);
    out << "=== Memory Status at Cycle " << cycle << " ===\n";
    for (const auto& frame : memory) {
        out << "Frame " << frame.frameId << ": ";
        if (frame.occupied) {
            out << "Process " << frame.ownerPid << " Page " << frame.virtualPage << "\n";
        } else {
            out << "Free\n";
        }
    }
    out << "\n";
}

bool FirstFitMemoryAllocator::isAllocated(std::shared_ptr<Process> const& process) const {
    return pageTables.find(process->pid) != pageTables.end();
}

bool FirstFitMemoryAllocator::writeMemory(int pid, uint16_t address, uint16_t value, std::string& errOut) {
    int page = address / pageSize;
    int offset = address % pageSize;
    int frameId = ensurePageMapped(pid, page, errOut);
    if (frameId == -1) return false;

    std::ofstream store(backingStoreFile, std::ios::app);
    store << "WRITE pid=" << pid
          << " page=" << page
          << " addr=0x" << std::hex << address
          << " val=" << value << "\n";
    return true;
}

bool FirstFitMemoryAllocator::readMemory(int pid, uint16_t address, uint16_t& outValue, std::string& errOut) {
    int page = address / pageSize;
    int offset = address % pageSize;
    int frameId = ensurePageMapped(pid, page, errOut);
    if (frameId == -1) return false;

    outValue = 0;  // Simulation: return dummy value

    std::ofstream store(backingStoreFile, std::ios::app);
    store << "READ pid=" << pid
          << " page=" << page
          << " addr=0x" << std::hex << address << "\n";
    return true;
}

int FirstFitMemoryAllocator::ensurePageMapped(int pid, int virtualPage, std::string& errOut) {
    auto& pt = pageTables[pid];
    if (pt.find(virtualPage) != pt.end()) {
        return pt[virtualPage];
    }

    // Page fault
    int freeFrame = findFreeFrame();
    if (freeFrame == -1) {
        if (fifoQueue.empty()) {
            errOut = "Page fault with no available frames for eviction.";
            return -1;
        }

        freeFrame = fifoQueue.front();
        fifoQueue.pop_front();

        int victimPid = memory[freeFrame].ownerPid;
        int victimPage = memory[freeFrame].virtualPage;

        // Log eviction
        std::ofstream store(backingStoreFile, std::ios::app);
        store << "EVICT pid=" << victimPid
              << " page=" << victimPage
              << " from frame=" << freeFrame << "\n";

        pageTables[victimPid].erase(victimPage);
    }

    memory[freeFrame].ownerPid = pid;
    memory[freeFrame].virtualPage = virtualPage;
    pt[virtualPage] = freeFrame;
    fifoQueue.push_back(freeFrame);

    return freeFrame;
}

int FirstFitMemoryAllocator::findFreeFrame() {
    for (auto& frame : memory) {
        if (frame.ownerPid == -1) return frame.frameId;
    }
    return -1;
}

void FirstFitMemoryAllocator::markAccessViolation(std::string& errOut, uint16_t badAddr) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << " | Memory access violation at address 0x"
       << std::hex << badAddr;
    errOut = ss.str();
}

bool FirstFitMemoryAllocator::isValidAddress(uint16_t addr) {
    return addr < 65536;
}