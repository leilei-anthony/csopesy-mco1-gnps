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

int FirstFitMemoryAllocator::findContiguousFrames(int count) {
    for (int i = 0; i <= totalFrames - count; i++) {
        bool canFit = true;
        for (int j = i; j < i + count; j++) {
            if (memory[j].ownerPid != -1) {
                canFit = false;
                break;
            }
        }
        if (canFit) return i;
    }
    return -1; // No contiguous space found
}

bool FirstFitMemoryAllocator::allocate(const std::shared_ptr<Process>& proc) {
    pageTables[proc->pid] = {};


    // Calculate required pages
    int requiredPages = (proc->memorySize + memPerFrame - 1) / memPerFrame;

    // Find and allocate contiguous frames for first-fit
    int startFrame = findContiguousFrames(requiredPages);
    if (startFrame == -1) {
        pageTables.erase(proc->pid);
        return false; // Not enough contiguous memory
    }
    
    // Allocate the frames
    for (int i = 0; i < requiredPages; i++) {
        memory[startFrame + i].ownerPid = proc->pid;
        memory[startFrame + i].virtualPage = i;
        pageTables[proc->pid][i] = startFrame + i;
        fifoQueue.push_back(startFrame + i);
    }
    
    // Log to backing store
    std::ofstream store(backingStoreFile, std::ios::app);
    store << "ALLOC pid=" << proc->pid
          << " mem=" << proc->memorySize
          << " pages=" << requiredPages
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
bool FirstFitMemoryAllocator::isAllocated(int pid) const {
    return pageTables.find(pid) != pageTables.end();
}
bool FirstFitMemoryAllocator::isAllocated(const ProcessPtr& process) const {
    return isAllocated(process->pid);
}

void FirstFitMemoryAllocator::dumpStatusToFile(int quantumCycle) const{
        // std::cout << "Dumping memory status to file for quantum cycle: " << quantumCycle << std::endl;
        std::filesystem::create_directories("output");
        std::ofstream file("output/memory_stamp_" + std::to_string(quantumCycle) + ".txt");
        if (!file.is_open()) return;
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S ");
        
        file << "Timestamp: " << ss.str();
        file << "\n";
        
        // Count processes in memory
        std::set<int> activePIDs;
        int usedFrames = 0;
        std::vector<int> holes;
        int currentHole = 0;
        
        for (auto& frame : memory) {
            if (frame.ownerPid == -1) {
                currentHole++;
            } else {
                usedFrames++;
                activePIDs.insert(frame.ownerPid);
                if (currentHole > 0) {
                    holes.push_back(currentHole);
                    currentHole = 0;
                }
            }
        }
        if (currentHole > 0) holes.push_back(currentHole);
        
        // External fragmentation in KB
        int fragmentationBytes = 0;
        for (int hole : holes) {
            fragmentationBytes += hole * memPerFrame;
        }
        
        file << "Processes in memory: " << activePIDs.size() << "\n";
        file << "External fragmentation: " << (fragmentationBytes / 1024) << " KB / " << (fragmentationBytes) << " B\n\n";
        
        // Add end boundary first
        file << "----end----- = " << totalMemory << "\n\n";

        // Collect all process blocks first
        std::vector<std::tuple<int, int, int>> processBlocks; // start, end, pid
        int i = 0;
        while (i < totalFrames) {
            if (memory[i].ownerPid != -1) {
                int start = i;
                int pid = memory[i].ownerPid;
                while (i < totalFrames && memory[i].ownerPid == pid) {
                    i++;
                }
                int end = i;  // exclusive
                processBlocks.push_back({start, end, pid});
            } else {
                i++;
            }
        }
        
        // Output process blocks in reverse order (highest address first)
        for (auto it = processBlocks.rbegin(); it != processBlocks.rend(); ++it) {
            int start = std::get<0>(*it);
            int end = std::get<1>(*it);
            int pid = std::get<2>(*it);
            
            // Format: upper_limit\nP<pid>\nlower_limit\n
            file << (end * memPerFrame) << "\n";
            file << "P" << pid << "\n";
            file << (start * memPerFrame) << "\n";
            file << "\n";
        }
        
        // Add boundary markers
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