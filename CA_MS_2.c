//gcc CA_MS_2.c assembly_parser.c -o CA_MS_2
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "instruction_set.h"

// Memory and register definition - maintain data types as specified
uint16_t instructionMemory[1024];  // 16-bit instructions as specified
uint8_t dataMemory[2048];          // 8-bit memory locations
uint8_t registers[64];             // 8-bit registers
uint8_t SREG;                      // 8-bit status register  
uint16_t pc;                       // 16-bit program counter

// Pipeline registers with proper data types
typedef struct {
    uint16_t instr;
    uint16_t pc;
    int valid;
} IF_ID_t;

typedef struct {
    uint8_t opcode;
    uint8_t r1;
    uint8_t r2;
    int8_t imm;      // Signed immediate (2's complement)
    uint8_t regval1;
    uint8_t regval2;
    uint16_t pc;
    int valid;
} ID_EX_t;

IF_ID_t IF_ID = {0};
ID_EX_t ID_EX = {0};
int branched = 0;

// Flag definitions
#define C_FLAG 4  // Carry Flag
#define V_FLAG 3  // Overflow Flag
#define N_FLAG 2  // Negative Flag
#define S_FLAG 1  // Sign Flag
#define Z_FLAG 0  // Zero Flag

// Flag manipulation functions
void set_flag(int flag_bit) {
    SREG |= (1 << flag_bit);
}

void clear_flag(int flag_bit) {
    SREG &= ~(1 << flag_bit);
}

int get_flag(int flag_bit) {
    return (SREG >> flag_bit) & 0x01;
}

// Update flags correctly
void update_flags(uint8_t result, int carry, int overflow) {
    // Zero flag
    if (result == 0) {
        set_flag(Z_FLAG);
    } else {
        clear_flag(Z_FLAG);
    }
    
    // Negative flag
    if (result & 0x80) {
        set_flag(N_FLAG);
    } else {
        clear_flag(N_FLAG);
    }
    
    // Overflow flag
    if (overflow) {
        set_flag(V_FLAG);
    } else {
        clear_flag(V_FLAG);
    }
    
    // Carry flag
    if (carry) {
        set_flag(C_FLAG);
    } else {
        clear_flag(C_FLAG);
    }
    
    // Sign flag (N XOR V)
    if ((get_flag(N_FLAG) ^ get_flag(V_FLAG)) == 1) {
        set_flag(S_FLAG);
    } else {
        clear_flag(S_FLAG);
    }
}

// Instruction decode structure
struct decoded {
    uint8_t opcode;
    uint8_t r1;
    uint8_t r2;
    int8_t immediate;  // Signed immediate
    uint8_t address;
};

// Fetch stage - returns 16-bit instruction
uint16_t fetch() {
    printf("  [FETCH] PC=%d, Instruction=0x%04X\n", pc, instructionMemory[pc]);
    return instructionMemory[pc++];
}

// Decode stage - parses 16-bit instruction
struct decoded decode(uint16_t instruction) {
    struct decoded dec;
    dec.opcode = (instruction >> 12) & 0xF;
    dec.r1 = (instruction >> 6) & 0x3F;
    dec.r2 = instruction & 0x3F;
    dec.immediate = instruction & 0x3F;  // Sign extend for immediate values
    dec.address = instruction & 0x3F;
    
    // Sign extend the immediate if not a shift instruction
    if (dec.opcode != sal_opcode && dec.opcode != sar_opcode) {
        // Sign extension for 6-bit to 8-bit
        if (dec.immediate & 0x20) {
            dec.immediate |= 0xC0;  // Extend with 1s
        }
    }
    
    printf("  [DECODE] Opcode=%d, R1=%d, R2=%d, IMM=%d\n", 
           dec.opcode, dec.r1, dec.r2, dec.immediate);
    
    return dec;
}

// Execute stage - performs ALU operations and updates register values
void execute(struct decoded dec, uint16_t instruction_pc) {
    uint8_t old_value;
    int carry = 0;
    int overflow = 0;
    uint8_t result = 0;
    char instr_name[10] = "";
    
    // Track old register values for change reporting
    old_value = registers[dec.r1];
    
    switch (dec.opcode) {
        case add_opcode: // ADD
            strcpy(instr_name, "ADD");
            printf("  [EXECUTE] ADD R%d, R%d (values: %d, %d)\n", 
                   dec.r1, dec.r2, registers[dec.r1], registers[dec.r2]);
            
            // Calculate carry using 9th bit
            carry = ((int)registers[dec.r1] + (int)registers[dec.r2]) > 0xFF;
            
            // Calculate overflow using XOR of carries
            {
                int bit6_carry = ((registers[dec.r1] & 0x40) + (registers[dec.r2] & 0x40)) > 0x40;
                int bit7_carry = ((registers[dec.r1] & 0x80) + (registers[dec.r2] & 0x80)) > 0x80;
                overflow = bit6_carry ^ bit7_carry;
            }
            
            // Perform addition
            result = registers[dec.r1] = registers[dec.r1] + registers[dec.r2];
            break;
            
        case sub_opcode: // SUB
            strcpy(instr_name, "SUB");
            printf("  [EXECUTE] SUB R%d, R%d (values: %d, %d)\n", 
                   dec.r1, dec.r2, registers[dec.r1], registers[dec.r2]);
            
            // Calculate carry for subtraction
            carry = registers[dec.r1] < registers[dec.r2];
            
            // Perform subtraction
            result = registers[dec.r1] = registers[dec.r1] - registers[dec.r2];
            
            // Calculate overflow using XOR of carries
            overflow = ((registers[dec.r1] ^ registers[dec.r2]) & (registers[dec.r1] ^ result) & 0x80) ? 1 : 0;
            break;
            
        case mul_opcode: // MUL
            strcpy(instr_name, "MUL");
            printf("  [EXECUTE] MUL R%d, R%d (values: %d, %d)\n", 
                   dec.r1, dec.r2, registers[dec.r1], registers[dec.r2]);
            
            // Calculate carry for multiplication
            carry = ((int)registers[dec.r1] * (int)registers[dec.r2]) > 0xFF;
            
            // Perform multiplication
            result = registers[dec.r1] = registers[dec.r1] * registers[dec.r2];
            break;
            
        case movi_opcode: // MOVI
            strcpy(instr_name, "MOVI");
            printf("  [EXECUTE] MOVI R%d, %d\n", dec.r1, dec.immediate);
            
            // Load immediate value
            registers[dec.r1] = dec.immediate & 0xFF;
            result = registers[dec.r1];
            break;
            
        case beqz_opcode: // BEQZ
            strcpy(instr_name, "BEQZ");
            printf("  [EXECUTE] BEQZ R%d, %d (value: %d)\n", 
                   dec.r1, dec.immediate, registers[dec.r1]);
            
            // Branch if register is zero
            if (registers[dec.r1] == 0) {
                // PC = instruction_pc + 1 + immediate
                // -1 because pc was already incremented in fetch
                uint16_t new_pc = instruction_pc + 1 + dec.immediate;
                printf("  [BRANCH] Taking branch to PC=%d\n", new_pc);
                pc = new_pc;
                branched = 1;  // Set branch flag
            } else {
                printf("  [BRANCH] Not taken\n");
            }
            break;
            
        case andi_opcode: // ANDI
            strcpy(instr_name, "ANDI");
            printf("  [EXECUTE] ANDI R%d, %d (value: %d)\n", 
                   dec.r1, dec.immediate, registers[dec.r1]);
            
            // Perform AND with immediate
            result = registers[dec.r1] = registers[dec.r1] & dec.immediate;
            break;
            
        case eor_opcode: // EOR
            strcpy(instr_name, "EOR");
            printf("  [EXECUTE] EOR R%d, R%d (values: %d, %d)\n", 
                   dec.r1, dec.r2, registers[dec.r1], registers[dec.r2]);
            
            // Perform XOR operation
            result = registers[dec.r1] = registers[dec.r1] ^ registers[dec.r2];
            break;
            
        case br_opcode: // BR
            strcpy(instr_name, "BR");
            printf("  [EXECUTE] BR R%d, %d\n", dec.r1, dec.r2);
            
            // Branch to address from registers
            pc = (registers[dec.r1] << 8) | registers[dec.r2];
            printf("  [BRANCH] Taking branch to PC=%d\n", pc);
            branched = 1;  // Set branch flag
            break;
            
        case sal_opcode: // SAL
            strcpy(instr_name, "SAL");
            printf("  [EXECUTE] SAL R%d, %d (value: %d)\n", 
                   dec.r1, dec.immediate, registers[dec.r1]);
            
            // Left shift
            registers[dec.r1] = registers[dec.r1] << dec.immediate;
            result = registers[dec.r1];
            break;
            
        case sar_opcode: // SAR
            strcpy(instr_name, "SAR");
            printf("  [EXECUTE] SAR R%d, %d (value: %d)\n", 
                   dec.r1, dec.immediate, registers[dec.r1]);
            
            // Arithmetic right shift (preserves sign bit)
            registers[dec.r1] = ((int8_t)registers[dec.r1]) >> dec.immediate;
            result = registers[dec.r1];
            break;
            
        case ldr_opcode: // LDR
            strcpy(instr_name, "LDR");
            printf("  [EXECUTE] LDR R%d, %d\n", dec.r1, dec.address);
            
            // Load from memory
            registers[dec.r1] = dataMemory[dec.address];
            result = registers[dec.r1];
            printf("  [MEMORY] Loading from address %d: value=%d\n", 
                   dec.address, registers[dec.r1]);
            break;
            
        case str_opcode: // STR
            strcpy(instr_name, "STR");
            printf("  [EXECUTE] STR R%d, %d (value: %d)\n", 
                   dec.r1, dec.address, registers[dec.r1]);
            
            // Store to memory
            uint8_t old_mem = dataMemory[dec.address];
            dataMemory[dec.address] = registers[dec.r1];
            printf("  [MEMORY] Memory[%d] changed: %d -> %d\n", 
                   dec.address, old_mem, dataMemory[dec.address]);
            break;
            
        default:
            printf("  [EXECUTE] Unknown opcode: %d\n", dec.opcode);
    }
    
    // Report register value changes
    if (registers[dec.r1] != old_value && dec.opcode != str_opcode) {
        printf("  [REGISTER] R%d changed: %d -> %d\n", dec.r1, old_value, registers[dec.r1]);
    }
    
    // Update flags for ALU operations
    if (dec.opcode == add_opcode || dec.opcode == sub_opcode || 
        dec.opcode == mul_opcode || dec.opcode == andi_opcode || 
        dec.opcode == eor_opcode || dec.opcode == sal_opcode || 
        dec.opcode == sar_opcode) {
        
        uint8_t old_sreg = SREG;
        update_flags(result, carry, overflow);
        
        // Report SREG changes
        if (old_sreg != SREG) {
            printf("  [SREG] changed: 0x%02X -> 0x%02X\n", old_sreg, SREG);
        }
    }
}

// Enhanced pipelined execution with detailed reporting
void pipelined_cycle(size_t *instructionsRemaining) {
    printf("\n--- CYCLE START ---\n");
    
    // Execute Stage
    if (ID_EX.valid) {
        printf("EXECUTE STAGE: Opcode=%d, R1=%d, R2=%d, IMM=%d\n",
               ID_EX.opcode, ID_EX.r1, ID_EX.r2, ID_EX.imm);
        
        // Create a decoded structure for execution
        struct decoded dec;
        dec.opcode = ID_EX.opcode;
        dec.r1 = ID_EX.r1;
        dec.r2 = ID_EX.r2;
        dec.immediate = ID_EX.imm;
        dec.address = ID_EX.imm;
        
        // Reset branch flag
        branched = 0;
        
        // Execute with PC value saved during decode
        execute(dec, ID_EX.pc);
    } else {
        printf("EXECUTE STAGE: <Empty>\n");
    }
    
    // Decode Stage - Only if not branched in this cycle
    if (IF_ID.valid && !branched) {
        printf("DECODE STAGE: Instruction=0x%04X, PC=%d\n", IF_ID.instr, IF_ID.pc);
        struct decoded decoded_instruction = decode(IF_ID.instr);
        
        // Update ID/EX pipeline register
        ID_EX.valid = 1;
        ID_EX.opcode = decoded_instruction.opcode;
        ID_EX.r1 = decoded_instruction.r1;
        ID_EX.r2 = decoded_instruction.r2;
        ID_EX.imm = decoded_instruction.immediate;
        ID_EX.regval1 = registers[decoded_instruction.r1];
        ID_EX.regval2 = registers[decoded_instruction.r2];
        ID_EX.pc = IF_ID.pc;  // Save PC for branch address calculation
    } else {
        // If branched or no valid instruction to decode
        if (branched) {
            printf("DECODE STAGE: <Flushed due to branch>\n");
        } else {
            printf("DECODE STAGE: <Empty>\n");
        }
        ID_EX.valid = 0;
    }
    
    // Fetch Stage - Only if not branched in this cycle
    if (!branched && *instructionsRemaining > 0) {
        printf("FETCH STAGE: PC=%d\n", pc);
        IF_ID.instr = fetch();
        IF_ID.pc = pc - 1;  // Save PC of this instruction
        IF_ID.valid = 1;
        (*instructionsRemaining)--;
    } else {
        if (branched) {
            printf("FETCH STAGE: <Flushed due to branch>\n");
            // When a branch is taken, the IF/ID register is invalidated
            IF_ID.valid = 0;
        } else if (*instructionsRemaining == 0) {
            printf("FETCH STAGE: <No more instructions>\n");
            IF_ID.valid = 0;
        }
    }
    
    printf("--- CYCLE END ---\n");
}

// Main function
int main() {
    // Initialize memory and registers
    for (int i = 0; i < 64; i++) {
        registers[i] = 0;
    }
    for (int i = 0; i < 2048; i++) {
        dataMemory[i] = 0;
    }
    SREG = 0;
    pc = 0;

    // Parse the instructions
    size_t numberOfInstructions;
    uint16_t* loaded_instructions = parseInstructions(&numberOfInstructions);
    if (loaded_instructions == NULL) {
        printf("Failed to load instructions. Exiting.\n");
        return 1;
    }
    
    // Copy instructions to instruction memory
    for (int i = 0; i < numberOfInstructions; i++) {
        instructionMemory[i] = loaded_instructions[i];
    }
    free(loaded_instructions);
    
    // Initial state
    printf("Initial state:\n");
    print_data();
    
    // Reset pipeline registers and PC
    IF_ID.valid = 0;
    ID_EX.valid = 0;
    branched = 0;
    pc = 0;
    
    // Pipelined execution
    printf("\nStarting pipelined execution:\n");
    size_t remaining = numberOfInstructions;
    size_t cycle = 1;
    
    // Continue until all instructions are processed and pipeline is empty
    while (remaining > 0 || IF_ID.valid || ID_EX.valid) {
        printf("\n===== Cycle %zu =====\n", cycle++);
        pipelined_cycle(&remaining);
        
        // Print remaining instructions and pipeline state
        printf("Remaining instructions: %zu\n", remaining);
        printf("Pipeline state: IF/ID valid=%d, ID/EX valid=%d\n", 
               IF_ID.valid, ID_EX.valid);
    }
    
    // Final state
    printf("\nFinal state after execution:\n");
    print_data();
    
    // Print full memory and registers
    printf("\nFull Register File:\n");
    for (int i = 0; i < 64; i++) {
        printf("R%d: %d\n", i, registers[i]);
    }
    
    printf("\nSREG: 0x%02X\n", SREG);
    printf("PC: %d\n", pc);
    
    printf("\nData Memory (first 20 locations):\n");
    for (int i = 0; i < 20; i++) {
        printf("Memory[%d]: %d\n", i, dataMemory[i]);
    }
    
    return 0;
}
