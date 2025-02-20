#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cctype>

using namespace std;

// Configuration parameters
const int NUM_CORES = 4;      // Simulated CPU cores
const int MEMORY_SIZE = 4096; // Memory size (in words)

// Represents a RISC-V core (CPU thread)
struct Core
{
    int registers[32]; // General-purpose registers
    int pc;            // Program Counter
    int core_id;       // Unique ID for each core

    Core(int id) : pc(0), core_id(id)
    {
        fill(begin(registers), end(registers), 0);
        registers[3] = core_id; // Store core ID in x3 (arbitrary convention)
    }
};

class RiscVSimulator
{
private:
    vector<Core> cores;          // All CPU cores
    vector<int> memory;          // Simulated RAM
    vector<string> instructions; // Loaded instructions

    // Check if a given register index is valid
    bool is_valid_register(int reg_index)
    {
        return reg_index >= 0 && reg_index < 32;
    }

    // Extracts register index from a string like "x5"
    int extract_reg_index(const string &reg_str)
    {
        if (reg_str.empty() || reg_str[0] != 'x')
            return -1;

        int reg_num = stoi(reg_str.substr(1));
        return (reg_num >= 0 && reg_num < 32) ? reg_num : -1;
    }

    // Parses immediate values (handles decimal, hex, and binary)
    int parse_immediate(const string &str)
    {
        if (str.empty())
            return 0;

        if (str.size() > 2 && str[0] == '0')
        {
            if (str[1] == 'x' || str[1] == 'X')
                return stoi(str, nullptr, 16);
            if (str[1] == 'b' || str[1] == 'B')
                return stoi(str.substr(2), nullptr, 2);
        }
        return stoi(str);
    }
    public:
    RiscVSimulator() : memory(MEMORY_SIZE, 0) {
        for (int i = 0; i < NUM_CORES; i++) {
            cores.emplace_back(i);
        }
    }

    // Loads assembly instructions from a file
    void load_instructions(const string &filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open " << filename << endl;
            return;
        }

        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                instructions.push_back(line);
            }
        }
        cout << "Loaded " << instructions.size() << " instructions from " << filename << "." << endl;
    }

    // Executes loaded instructions across all cores
    void execute() {
        bool cores_active = true;

        while (cores_active) {
            cores_active = false;

            for (auto &core : cores) {
                if (core.pc / 4 < instructions.size()) {
                    cores_active = true;
                    execute_instruction(core, instructions[core.pc / 4]);
                }
            }

            if (!cores_active) {
                cout << "All cores are idle. Stopping execution." << endl;
            }
        }
    }

    // Decodes and executes a single instruction for a given core
    void execute_instruction(Core &core, const string &instruction){}
};
