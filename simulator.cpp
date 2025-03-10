#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cctype>
#include <iomanip> // For formatting output
#include <map>     // For instruction latencies

using namespace std;

// Configuration parameters
const int NUM_CORES = 4;      // Simulated CPU cores
const int MEMORY_SIZE = 4096; // Memory size (in words)

// Instruction structure
struct Instruction
{
    string opcode;
    int rd, rs1, rs2, imm; // Register indices and immediate
    int core_id;            // The core this instruction belongs to
    int pc;                 // Program counter value for this instruction

    Instruction(string op = "", int r = -1, int s1 = -1, int s2 = -1, int i = 0, int id = -1, int pc_val = 0)
        : opcode(op), rd(r), rs1(s1), rs2(s2), imm(i), core_id(id), pc(pc_val) {}
};

// Represents a RISC-V core (CPU thread)
struct Core
{
    int registers[32]; // General-purpose registers
    int pc;            // Program Counter
    int core_id;       // Unique ID for each core
    bool stalled;      // Flag to indicate if the core is stalled

    Core(int id) : pc(0), core_id(id), stalled(false)
    {
        fill(begin(registers), end(registers), 0);
        registers[3] = core_id; // Store core ID in x3 (arbitrary convention)
    }
};

// Pipeline Stage structure
struct PipelineStage
{
    Instruction instruction;
    bool valid; // Indicates if the stage contains a valid instruction
    int latency_counter; // Counter for instruction latency
    PipelineStage() : valid(false), latency_counter(0) {}
};

class RiscVSimulator
{
private:
    vector<Core> cores;          // All CPU cores
    vector<int> memory;          // Simulated RAM
    vector<string> instructions; // Loaded instructions

    // Pipeline stages for each core
    vector<PipelineStage> fetch_stage;
    vector<PipelineStage> decode_stage;
    vector<PipelineStage> execute_stage;
    vector<PipelineStage> memory_stage;
    vector<PipelineStage> writeback_stage;

    // Instruction Latencies (user configurable)
    map<string, int> instruction_latencies;

    bool forwarding_enabled; // Flag to enable or disable data forwarding

    // Statistics
    int total_cycles;
    int total_stalls;

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

        try
        {
            int reg_num = stoi(reg_str.substr(1));
            return (reg_num >= 0 && reg_num < 32) ? reg_num : -1;
        }
        catch (const invalid_argument &)
        {
            return -1; // Invalid register
        }
    }

    // Parses immediate values (handles decimal, hex, and binary)
    int parse_immediate(const string &str)
    {
        if (str.empty())
            return 0;

        try
        {
            if (str.size() > 2 && str[0] == '0')
            {
                if (str[1] == 'x' || str[1] == 'X')
                    return stoi(str, nullptr, 16); // Hexadecimal
                if (str[1] == 'b' || str[1] == 'B')
                    return stoi(str.substr(2), nullptr, 2); // Binary
            }
            return stoi(str); // Decimal
        }
        catch (const invalid_argument &)
        {
            return 0; // Invalid immediate, default to 0
        }
    }

    // Fetches the instruction for a given core
    Instruction fetch(Core &core)
    {
        if (core.pc / 4 < instructions.size())
        {
            string instruction_str = instructions[core.pc / 4];
            istringstream iss(instruction_str);
            string opcode, arg1, arg2, arg3;
            iss >> opcode >> arg1 >> arg2 >> arg3;

            int rd = extract_reg_index(arg1);
            int rs1 = extract_reg_index(arg2);
            int rs2 = extract_reg_index(arg3);
            int imm = parse_immediate(arg3); // Use arg3 for immediate value

            return Instruction(opcode, rd, rs1, rs2, imm, core.core_id, core.pc);
        }
        return Instruction(); // Return a default instruction if no more instructions
    }

    // Decodes the instruction and performs register fetch
    Instruction decode(Instruction &instruction)
    {
        // This is where you would typically check for data dependencies
        // and potential stalls.  For now, we'll just pass the instruction along.
        return instruction;
    }

    // Executes the instruction
    void execute(Instruction &instruction, Core &core)
    {
        if (instruction.opcode == "JAL")
        { // Jump and Link
            if (is_valid_register(instruction.rd))
            {
                core.registers[instruction.rd] = instruction.pc + 4; // Store return address
                core.pc += instruction.imm;
            }
        }
        else if (instruction.opcode == "BNE")
        { // Branch if Not Equal
            if (is_valid_register(instruction.rd) && is_valid_register(instruction.rs1))
            {
                if (core.registers[instruction.rd] != core.registers[instruction.rs1])
                {
                    core.pc += instruction.imm;
                }
                else
                {
                    core.pc += 4;
                }
                return; // Important: Skip the default PC increment
            }
        }
        else if (instruction.opcode == "ADD")
        { // Addition
            if (is_valid_register(instruction.rd) && is_valid_register(instruction.rs1) && is_valid_register(instruction.rs2))
            {
                core.registers[instruction.rd] = core.registers[instruction.rs1] + core.registers[instruction.rs2];
            }
        }
        else if (instruction.opcode == "SUB")
        { // Subtraction
            if (is_valid_register(instruction.rd) && is_valid_register(instruction.rs1) && is_valid_register(instruction.rs2))
            {
                core.registers[instruction.rd] = core.registers[instruction.rs1] - core.registers[instruction.rs2];
            }
        }
        else if (instruction.opcode == "SWAP")
        { // Swap two registers
            if (is_valid_register(instruction.rs1) && is_valid_register(instruction.rs2))
            {
                swap(core.registers[instruction.rs1], core.registers[instruction.rs2]);
            }
        }
        core.pc += 4; // Default PC increment
    }

    // Memory access stage (currently empty)
    void memory_access(Instruction &instruction)
    {
        // Implement memory read/write operations here
    }

    // Writeback stage
    void writeback(Instruction &instruction, Core &core)
    {
        // This is where you write the result back to the register file.
        // For now, it's empty as the execution directly updates registers.
    }

    // Checks for data dependencies and inserts stalls if necessary.
    bool check_data_hazards(Core &core)
    {
        // Example: Check if the current instruction in the decode stage depends
        // on a register being written to by an instruction in the execute or
        // memory stage.
        if (!decode_stage[core.core_id].valid)
            return false;

        Instruction &decode_inst = decode_stage[core.core_id].instruction;

        // Check against execute stage
        if (execute_stage[core.core_id].valid && execute_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &execute_inst = execute_stage[core.core_id].instruction;
            if ((decode_inst.rs1 == execute_inst.rd || decode_inst.rs2 == execute_inst.rd) && forwarding_enabled == false)
            {
                total_stalls++;
                return true; // Stall needed
            }
        }

        // Check against memory stage
        if (memory_stage[core.core_id].valid && memory_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &memory_inst = memory_stage[core.core_id].instruction;
            if ((decode_inst.rs1 == memory_inst.rd || decode_inst.rs2 == memory_inst.rd) && forwarding_enabled == false)
            {
                total_stalls++;
                return true; // Stall needed
            }
        }

        // Check against writeback stage
        if (writeback_stage[core.core_id].valid && writeback_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &writeback_inst = writeback_stage[core.core_id].instruction;
            if ((decode_inst.rs1 == writeback_inst.rd || decode_inst.rs2 == writeback_inst.rd) && forwarding_enabled == false)
            {
                total_stalls++;
                return true; // Stall needed
            }
        }

        return false; // No stall needed
    }

    // Performs data forwarding if enabled
    void perform_data_forwarding(Core &core)
    {
        if (!decode_stage[core.core_id].valid)
            return;

        Instruction &decode_inst = decode_stage[core.core_id].instruction;

        // Forward from execute stage
        if (execute_stage[core.core_id].valid && execute_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &execute_inst = execute_stage[core.core_id].instruction;
            if (decode_inst.rs1 == execute_inst.rd)
            {
                decode_inst.rs1 = execute_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from execute to decode for register x" << decode_inst.rs1 << endl;
            }
            if (decode_inst.rs2 == execute_inst.rd)
            {
                decode_inst.rs2 = execute_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from execute to decode for register x" << decode_inst.rs2 << endl;
            }
        }

        // Forward from memory stage
        if (memory_stage[core.core_id].valid && memory_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &memory_inst = memory_stage[core.core_id].instruction;
            if (decode_inst.rs1 == memory_inst.rd)
            {
                decode_inst.rs1 = memory_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from memory to decode for register x" << decode_inst.rs1 << endl;
            }
            if (decode_inst.rs2 == memory_inst.rd)
            {
                decode_inst.rs2 = memory_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from memory to decode for register x" << decode_inst.rs2 << endl;
            }
        }

        // Forward from writeback stage
        if (writeback_stage[core.core_id].valid && writeback_stage[core.core_id].instruction.rd != -1)
        {
            Instruction &writeback_inst = writeback_stage[core.core_id].instruction;
            if (decode_inst.rs1 == writeback_inst.rd)
            {
                decode_inst.rs1 = writeback_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from writeback to decode for register x" << decode_inst.rs1 << endl;
            }
            if (decode_inst.rs2 == writeback_inst.rd)
            {
                decode_inst.rs2 = writeback_inst.rd; // Forward the result
                cout << "Data forwarding: Core " << core.core_id << ", forwarding from writeback to decode for register x" << decode_inst.rs2 << endl;
            }
        }
    }

    // Sorts a partition of memory assigned to a core
    void bubble_sort_memory(Core &core)
    {
        int start_idx = core.core_id * (MEMORY_SIZE / NUM_CORES);
        int end_idx = start_idx + (MEMORY_SIZE / NUM_CORES);

        for (int i = start_idx; i < end_idx - 1; i++)
        {
            for (int j = start_idx; j < end_idx - (i - start_idx) - 1; j++)
            {
                if (memory[j] > memory[j + 1])
                {
                    swap(memory[j], memory[j + 1]);
                }
            }
        }
    }

public:
    RiscVSimulator() : memory(MEMORY_SIZE, 0),
                        fetch_stage(NUM_CORES),
                        decode_stage(NUM_CORES),
                        execute_stage(NUM_CORES),
                        memory_stage(NUM_CORES),
                        writeback_stage(NUM_CORES),
                        forwarding_enabled(true), // Default: forwarding enabled
                        total_cycles(0),
                        total_stalls(0)
    {
        for (int i = 0; i < NUM_CORES; i++)
        {
            cores.emplace_back(i);
        }

        // Default instruction latencies
        instruction_latencies["ADD"] = 1;
        instruction_latencies["SUB"] = 1;
        instruction_latencies["JAL"] = 1;
        instruction_latencies["BNE"] = 1;
        instruction_latencies["SWAP"] = 1;
    }

    // Allows user to set instruction latencies
    void set_instruction_latency(const string &opcode, int latency)
    {
        instruction_latencies[opcode] = latency;
    }

    // Enables or disables data forwarding
    void enable_forwarding(bool enable)
    {
        forwarding_enabled = enable;
    }

    // Loads assembly instructions from a file
    void load_instructions(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            cerr << "Error: Cannot open " << filename << endl;
            return;
        }

        string line;
        while (getline(file, line))
        {
            if (!line.empty())
            {
                instructions.push_back(line);
            }
        }
        cout << "Loaded " << instructions.size() << " instructions from " << filename << "." << endl;
    }

    // Executes loaded instructions across all cores (Pipelined)
    void execute()
    {
        bool cores_active = true;
        while (cores_active)
        {
            cores_active = false;
            total_cycles++;

            // Iterate through each core and process the pipeline stages
            for (auto &core : cores)
            {
                if (core.pc / 4 < instructions.size() ||
                    fetch_stage[core.core_id].valid ||
                    decode_stage[core.core_id].valid ||
                    execute_stage[core.core_id].valid ||
                    memory_stage[core.core_id].valid ||
                    writeback_stage[core.core_id].valid)
                {
                    cores_active = true; // Core is still active

                    // Writeback Stage
                    if (writeback_stage[core.core_id].valid)
                    {
                        cout << "Core " << core.core_id << " - Writeback: " << writeback_stage[core.core_id].instruction.opcode << endl;
                        writeback(writeback_stage[core.core_id].instruction, core);
                        writeback_stage[core.core_id].valid = false;
                    }

                    // Memory Stage
                    if (memory_stage[core.core_id].valid)
                    {
                        cout << "Core " << core.core_id << " - Memory: " << memory_stage[core.core_id].instruction.opcode << endl;
                        memory_access(memory_stage[core.core_id].instruction);
                        PipelineStage &stage = memory_stage[core.core_id];
                        writeback_stage[core.core_id] = stage;
                        memory_stage[core.core_id].valid = false;
                    }

                    // Execute Stage
                    if (execute_stage[core.core_id].valid)
                    {
                        // Check if the instruction has finished its latency
                        if (execute_stage[core.core_id].latency_counter > 1)
                        {
                            execute_stage[core.core_id].latency_counter--;
                        }
                        else
                        {
                            cout << "Core " << core.core_id << " - Execute: " << execute_stage[core.core_id].instruction.opcode << endl;
                            execute(execute_stage[core.core_id].instruction, core);
                            PipelineStage &stage = execute_stage[core.core_id];
                            memory_stage[core.core_id] = stage;
                            execute_stage[core.core_id].valid = false;
                        }
                    }

                    // Decode Stage
                    if (decode_stage[core.core_id].valid)
                    {
                        if (forwarding_enabled)
                        {
                            perform_data_forwarding(core);
                        }
                        cout << "Core " << core.core_id << " - Decode: " << decode_stage[core.core_id].instruction.opcode << endl;
                        Instruction decoded_instruction = decode(decode_stage[core.core_id].instruction);
                        PipelineStage &stage = decode_stage[core.core_id];
                        execute_stage[core.core_id] = stage;
                        execute_stage[core.core_id].latency_counter = instruction_latencies[decoded_instruction.opcode];
                        decode_stage[core.core_id].valid = false;
                    }

                    // Fetch Stage
                    if (fetch_stage[core.core_id].valid)
                    {
                        // Check for data hazards before moving to the decode stage
                        if (check_data_hazards(core))
                        {
                            core.stalled = true;
                            cout << "Core " << core.core_id << " - Stalled at Fetch due to data hazard" << endl;
                            continue; //

                        }
                        else
                        {
                            core.stalled = false;
                            cout << "Core " << core.core_id << " - Fetch: " << fetch_stage[core.core_id].instruction.opcode << endl;
                            Instruction fetched_instruction = fetch_stage[core.core_id].instruction;
                            PipelineStage &stage = fetch_stage[core.core_id];
                            decode_stage[core.core_id] = stage;
                            fetch_stage[core.core_id].valid = false;
                        }
                    }

                    // Fetch new instruction if the core is not stalled
                    if (!core.stalled && core.pc / 4 < instructions.size())
                    {
                        Instruction new_instruction = fetch(core);
                        fetch_stage[core.core_id].instruction = new_instruction;
                        fetch_stage[core.core_id].valid = true;
                        core.pc += 4; // Increment PC after fetching
                    }
                }
            }
        }

        // Print final statistics
        cout << "\nSimulation completed in " << total_cycles << " cycles." << endl;
        cout << "Total stalls: " << total_stalls << endl;
    }

    // Prints the contents of memory
    void print_memory()
    {
        cout << "Memory Contents:" << endl;
        for (int i = 0; i < MEMORY_SIZE; i++)
        {
            cout << "Address " << i << ": " << memory[i] << endl;
        }
    }

    // Prints the contents of registers for all cores
    void print_registers()
    {
        for (auto &core : cores)
        {
            cout << "Core " << core.core_id << " Registers:" << endl;
            for (int i = 0; i < 32; i++)
            {
                cout << "x" << i << ": " << core.registers[i] << endl;
            }
            cout << endl;
        }
    }

    // Sorts memory partitions assigned to each core
    void sort_memory()
    {
        for (auto &core : cores)
        {
            bubble_sort_memory(core);
        }
        cout << "Memory sorted by each core." << endl;
    }
};

int main()
{
    RiscVSimulator simulator;

    // Load instructions from a file
    simulator.load_instructions("instructions.txt");

    // Enable or disable data forwarding
    simulator.enable_forwarding(true);

    // Set custom instruction latencies (optional)
    simulator.set_instruction_latency("ADD", 2);
    simulator.set_instruction_latency("SUB", 2);

    // Execute the instructions
    simulator.execute();

    // Print final register and memory states
    simulator.print_registers();
    simulator.print_memory();

    // Sort memory partitions
    simulator.sort_memory();
    simulator.print_memory();

    return 0;
}