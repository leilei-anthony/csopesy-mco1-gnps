#pragma once
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include "Process.h"
#include <set>
#include <chrono>
#include <iomanip>

class MemoryFrame {
public:
    int frameId;
    int ownerPid = -1;  // -1 means free

    MemoryFrame(int id) : frameId(id) {}
    
};

class FirstFitMemoryAllocator {
private:
    std::vector<MemoryFrame> memory;
    int memPerFrame;
    int memPerProc;
    int totalFrames;
    int totalMemory;

public:
    void init(int maxMemory, int frameSize, int procLimit) {
        totalMemory = maxMemory;
        memPerFrame = frameSize;
        memPerProc = procLimit;
        totalFrames = totalMemory / memPerFrame;

        memory.clear();
        for (int i = 0; i < totalFrames; ++i) {
            memory.emplace_back(i);
        }
    }

    bool canFitProcess() {
        return findFreeBlock(memPerProc / memPerFrame) != -1;
    }

    bool allocate(const std::shared_ptr<Process>& proc) {
        int neededFrames = memPerProc / memPerFrame;
        int start = findFreeBlock(neededFrames);
        if (start == -1) return false;

        for (int i = start; i < start + neededFrames; ++i) {
            memory[i].ownerPid = proc->pid;
        }
        return true;
    }

    void deallocate(const std::shared_ptr<Process>& proc) {
        for (auto& frame : memory) {
            if (frame.ownerPid == proc->pid) {
                frame.ownerPid = -1;
            }
        }
    }
    
    /* void dumpStatusToFile(int quantumCycle) {
    std::ofstream file("memory_stamp_" + std::to_string(quantumCycle) + ".txt");
    if (!file.is_open()) return;

    file << "Timestamp (Quantum Cycle): " << quantumCycle << "\n";

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
    file << "External fragmentation: " << (fragmentationBytes / 1024) << " KB\n";

    // ASCII Memory Layout
    file << "\nMemory layout:\n";
    for (int i = 0; i < totalFrames; ++i) {
        if (i % 64 == 0) file << "\n";
        file << (memory[i].ownerPid == -1 ? '.' : '#');
    }

    file << "\n\nMemory block details:\n";
    int i = 0;
    file << "----end----- = 0\n";
    while (i < totalFrames) {
        if (memory[i].ownerPid != -1) {
            int start = i;
            int pid = memory[i].ownerPid;
            while (i < totalFrames && memory[i].ownerPid == pid) {
                i++;
            }
            int end = i;  // exclusive
            file << (end * memPerFrame) << "\n";
            file << "P" << pid << "\n";
            file << (start * memPerFrame) << "\n\n";
        } else {
            i++;
        }
    }
    file << "----start----- = 0\n";

    file << "\n";
    file.close();
} */

void dumpStatusToFile(int quantumCycle) {
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


private:
    int findFreeBlock(int neededFrames) {
        int freeCount = 0;
        for (int i = 0; i < totalFrames; ++i) {
            if (memory[i].ownerPid == -1) {
                freeCount++;
                if (freeCount == neededFrames) return i - neededFrames + 1;
            } else {
                freeCount = 0;
            }
        }
        return -1;
    }

public:
    int getMemPerFrame() const { return memPerFrame; }
    int getMemPerProc() const { return memPerProc; }
    int getTotalMemory() const { return totalMemory; }
};