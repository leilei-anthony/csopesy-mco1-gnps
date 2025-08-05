#include "MemoryManager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <thread>

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

std::vector<int> FirstFitMemoryAllocator::findAnyFreeFrames(int count) {
    std::vector<int> freeFrames;
    for (int i = 0; i < totalFrames && freeFrames.size() < count; ++i) {
        if (memory[i].ownerPid == -1) {
            freeFrames.push_back(i);
        }
    }
    return (freeFrames.size() == count) ? freeFrames : std::vector<int>{}; // Return empty if not enough
}

bool FirstFitMemoryAllocator::allocate(const std::shared_ptr<Process>& proc) {
    int requiredPages = (proc->memorySize + memPerFrame - 1) / memPerFrame;
    pageTables[proc->pid] = {};

    std::ofstream store(backingStoreFile, std::ios::app);

    // Step 1: Log allocation
    if (store.is_open()) {
        store << "ALLOC pid=" << proc->pid
              << " mem=" << proc->memorySize
              << " pages=" << requiredPages << "\n";
    }

    for (int i = 0; i < requiredPages; ++i) {
        int frameIndex = -1;

        // Try to find a free frame
        auto free = findAnyFreeFrames(1);
        if (!free.empty()) {
            frameIndex = free[0];
        } else {
            // No free frame — perform FIFO replacement
            if (fifoQueue.empty()) {
                if (store.is_open()) store << "FAIL pid=" << proc->pid << " reason=No frames and queue empty\n";
                pageTables.erase(proc->pid);
                store.close();
                return false;
            }

            frameIndex = fifoQueue.front();
            fifoQueue.pop_front();

            // Victim info
            int victimPid = memory[frameIndex].ownerPid;
            int victimVPage = memory[frameIndex].virtualPage;

            // Remove mapping
            pageTables[victimPid].erase(victimVPage);

            // Log swapout
            if (store.is_open()) {
                store << "SWAPOUT pid=" << victimPid
                      << " vpage=" << victimVPage
                      << " pframe=" << frameIndex << "\n";
            }

            // Count as page out
            pageOuts++;

            // Mark frame as available
            memory[frameIndex].ownerPid = -1;
            memory[frameIndex].virtualPage = -1;
        }

        // Use frameIndex for this process
        memory[frameIndex].ownerPid = proc->pid;
        memory[frameIndex].virtualPage = i;
        pageTables[proc->pid][i] = frameIndex;
        fifoQueue.push_back(frameIndex);

        // Log page fault and swapin
        if (store.is_open()) {
            store << "PAGEFAULT pid=" << proc->pid
                  << " vpage=" << i << "\n";
            store << "SWAPIN pid=" << proc->pid
                  << " vpage=" << i
                  << " pframe=" << frameIndex << "\n";
        }

        // Count as page in
        pageIns++;
    }

    if (store.is_open()) {
        store.close();
    }

    return true;
}


void FirstFitMemoryAllocator::deallocate(const std::shared_ptr<Process>& proc) {
    // Remove frames from FIFO *before* freeing
    fifoQueue.erase(
        std::remove_if(fifoQueue.begin(), fifoQueue.end(),
            [&](int fId) { return memory[fId].ownerPid == proc->pid; }),
        fifoQueue.end()
    );

    // Now clear memory frames
    for (auto& frame : memory) {
        if (frame.ownerPid == proc->pid) {
            frame.ownerPid = -1;
            frame.virtualPage = -1;
        }
    }

    // Remove from page table
    pageTables.erase(proc->pid);

    std::ofstream store(backingStoreFile, std::ios::app);
if (store.is_open()) {
    store << "DEALLOC pid=" << proc->pid << "\n";
    store.close();
}

}

bool FirstFitMemoryAllocator::isAllocated(int pid) const {
    return pageTables.find(pid) != pageTables.end();
}
bool FirstFitMemoryAllocator::isAllocated(const ProcessPtr& process) const {
    return isAllocated(process->pid);
}

void FirstFitMemoryAllocator::dumpStatusToFile(int quantumCycle) const {
    std::filesystem::create_directories("output");
    std::ofstream file("output/memory_stamp_" + std::to_string(quantumCycle) + ".txt");
    if (!file.is_open()) return;

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S ");
    file << "Timestamp: " << ss.str() << "\n";

    // Memory summary
    int usedFrames = 0;
    std::set<int> activePIDs;

    for (const auto& frame : memory) {
        if (frame.ownerPid != -1) {
            usedFrames++;
            activePIDs.insert(frame.ownerPid);
        }
    }

    file << "Processes in memory: " << activePIDs.size() << "\n";
    file << "Total memory: " << (totalMemory / 1024) << " KB / " << totalMemory << " B\n";
    file << "Used memory: " << (getUsedMemory() / 1024) << " KB / " << getUsedMemory() << " B\n";
    file << "Free memory: " << (getFreeMemory() / 1024) << " KB / " << getFreeMemory() << " B\n\n";
    file << "Used frames: " << usedFrames << " / " << totalFrames << "\n";
    file << "Free frames: " << (totalFrames - usedFrames) << "\n\n";

    // Memory map (top to bottom)
    file << "----end----- = " << totalMemory << "\n\n";

    for (int i = totalFrames - 1; i >= 0; --i) {
        const auto& frame = memory[i];
        int upper = (i + 1) * memPerFrame;
        int lower = i * memPerFrame;

        file << upper << "\n";
        if (frame.ownerPid != -1) {
            file << "P" << frame.ownerPid << ":page#" << frame.virtualPage << "\n";
        } else {
            file << "FREE\n";
        }
        file << lower << "\n\n";
    }

    file << "----start----- = 0\n";
    file.close();
}


bool FirstFitMemoryAllocator::writeMemory(int pid, uint16_t address, uint16_t value, std::string& errOut) {
    int page = address / memPerFrame;
    int offset = address % memPerFrame;
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
    int page = address / memPerFrame;
    int offset = address % memPerFrame;
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

int FirstFitMemoryAllocator::getPageIns() const {
    return pageIns;
}

int FirstFitMemoryAllocator::getPageOuts() const {
    return pageOuts;
}

