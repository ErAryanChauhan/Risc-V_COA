#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cctype>

using namespace std;

// Configuration parameters
const int NUM_CORES = 4;       // Simulated CPU cores
const int MEMORY_SIZE = 4096;  // Memory size (in words)

// Represents a RISC-V core (CPU thread)
struct Core {
    int registers[32];  // General-purpose registers
    int pc;             // Program Counter
    int core_id;        // Unique ID for each core

    Core(int id) : pc(0), core_id(id) {
        fill(begin(registers), end(registers), 0);
        registers[3] = core_id; // Store core ID in x3 (arbitrary convention)
    }
};
