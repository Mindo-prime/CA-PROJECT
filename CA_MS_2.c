//gcc CA_MS_2.c assembly_parser.c -o CA_MS_2
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "instruction_set.h"

uint16_t instructionMemory[1024];
uint8_t dataMemory[2048];     
uint8_t registers[64];
   
uint8_t SREG;
uint16_t pc;

uint8_t old_registers[64];    // To track register changes
uint8_t old_dataMemory[2048]; // To track memory changes
struct decoded *decodedInstruction;
size_t* NumberofInstructions;
static int cycle_count = 0;

struct decoded {
    int opcode;
    int r1;
    int r2;
    int immediate;
    int address;
};

// Behold the intermideate reg
// Between Fetch and Decode
typedef struct {
    uint16_t instr;
    uint16_t pc_plus;
    int      valid;
} IF_ID_t;

// Between Decode and Execute
typedef struct {
    int      opcode, r1, r2;
    int16_t  imm;      
    uint8_t  regval1, regval2;
    uint16_t pc_plus;
    int      valid;
} ID_EX_t;


IF_ID_t  IF_ID = {0};
ID_EX_t  ID_EX = {0};
int branched = 0;
int valid = 0;

#define C_FLAG 4  // Carry Flag
#define V_FLAG 3  // Overflow Flag
#define N_FLAG 2  // Negative Flag
#define S_FLAG 1  // Sign Flag
#define Z_FLAG 0  // Zero Flag

extern uint16_t* parseInstructions(size_t* NumberofInstructions); // is defined in assembly_parser.c

void set_flag(int flag_bit) {
    SREG |= (1<<flag_bit);//1<<0,
}

void clear_flag(int flag_bit) {
    SREG &= ~(1<<flag_bit);
}

int get_flag(int flag_bit) {
    return (SREG>>flag_bit)&0b1;
}

void update_flags(uint8_t result, int overflow, int carry) {
    if (result == 0) {
        set_flag(Z_FLAG);
    } else {
        clear_flag(Z_FLAG);
    }
    if (result & 0x80) { //10000000
        set_flag(N_FLAG);
    } else {
        clear_flag(N_FLAG);
    }
    if (overflow) {
        set_flag(V_FLAG);
    } else {
        clear_flag(V_FLAG);
    }
    if (carry) {
        set_flag(C_FLAG);
    } else {
        clear_flag(C_FLAG);
    }
    if (get_flag(N_FLAG) ^ get_flag(V_FLAG)){
        set_flag(S_FLAG);
    }else{
        clear_flag(S_FLAG);
    }
}
// Binary int format in c is 0b00000000000000000000000000000000 (32 bits)
uint16_t fetch() {
    return  instructionMemory[pc++];       
}

struct decoded decode(int instruction) {
       
        struct decoded decoded_instruction;
        decoded_instruction.opcode = instruction>>12; 
        decoded_instruction.r1 = (instruction>>6)&0b111111;     
        decoded_instruction.r2 = 
            decoded_instruction.immediate=
                decoded_instruction.address = instruction&0b111111;      
       /* printf("\n---------- \n");
        printf("decoding instruction %x\n",instruction);
        printf("Instruction %i\n",pc);
        printf("opcode = %i\n",decoded_instruction.opcode);
        printf("r1 = %i\n",decoded_instruction.r1);
        printf("r2 = %i\n",decoded_instruction.r2);
        printf("Immediate = %i\n",decoded_instruction.r2);
        printf("Address = %i\n",decoded_instruction.r2);
        printf("\n---------- \n");*/

        return decoded_instruction;    
}

void execute(struct decoded dec) {
    int carry = 0;
    int overflow =0;
    uint8_t result =1;
    switch (dec.opcode) {
        case add_opcode: // ADD
            printf("executing ADD R%d, R%d\n",dec.r1,dec.r2);
            carry = ((int)registers[dec.r1] + (int)registers[dec.r2]) > 0xFF;
            result = registers[dec.r1] = registers[dec.r1] + registers[dec.r2];
            overflow = (~(registers[dec.r1] ^ registers[dec.r2]) & (registers[dec.r1] ^ result) & 0x80) ? 1 : 0;
            break;
        case sub_opcode: // SUB
            printf("executing SUB R%d, R%d\n",dec.r1,dec.r2);
            carry = registers[dec.r1] < registers[dec.r2];
            result = registers[dec.r1] = registers[dec.r1] - registers[dec.r2]; 
            overflow = (((registers[dec.r1] ^ registers[dec.r2]) & (registers[dec.r1] ^ result)) & 0x80) ? 1 : 0;
            break;
        case mul_opcode: // MUL
            printf("executing MUL R%d, R%d\n",dec.r1,dec.r2);
            result = registers[dec.r1] = registers[dec.r1] * registers[dec.r2];
            carry = ((int)registers[dec.r1] * (int)registers[dec.r2]) > 0xFF;
            break;
        case movi_opcode: // MOVI
            printf("executing MOVI R%d, %d\n",dec.r1,dec.immediate);
            registers[dec.r1] = dec.immediate;
            break;
        case beqz_opcode: // BEQZ
            printf("executing BEQZ R%d, R%d\n",dec.r1,dec.immediate);
            if (registers[dec.r1] == 0){
                if (ID_EX.valid) { // We're in pipelined mode
                    pc = ID_EX.pc_plus + dec.immediate; 
                } else { // We're in single-cycle mode
                    pc = pc + dec.immediate; // PC already incremented in fetch
                }
                branched = 1;
            }
            break;
        case andi_opcode: // ANDI
            printf("executing AND R%d, R%d\n",dec.r1,dec.immediate);
            result = registers[dec.r1] = registers[dec.r1] & dec.immediate;
            break;
        case eor_opcode: // EOR
            printf("executing EOR R%d, R%d\n",dec.r1,dec.r2);
            result = registers[dec.r1] = registers[dec.r1] ^ registers[dec.r2];
            break;
        case br_opcode: // BR
            printf("executing BR R%d, %d\n",dec.r1, dec.r2);
            pc = ((uint16_t)registers[dec.r1] << 8) | registers[dec.r2];// Not sure if that is correct bro my friend said r1 >>  8
            branched = 1;
            break;
        case sal_opcode: // SAL
            printf("executing SAL R%d, %d\n",dec.r1, dec.immediate);
            registers[dec.r1] = registers[dec.r1] << dec.immediate; 
            break;  
        case sar_opcode: // SAR
            printf("executing SAR %d\n", dec.immediate); 
            registers[dec.r1] = ((int8_t)registers[dec.r1]) >> dec.immediate;
            break;
        case ldr_opcode: // LDR
            printf("executing LB R%d, R%d\n",dec.r1,dec.address);
            registers[dec.r1] = dataMemory[dec.address];
            break;
        case str_opcode: // STR
            printf("executing SB R%d, %d\n",dec.r1, dec.address);
            dataMemory[dec.address] = registers[dec.r1];
            break;
        default:
            printf(" Unknown opcode\n");
    }
    update_flags(result,carry,overflow);
}

void single_instruction_cycle(){
    printf("\n===== SINGLE INSTRUCTION CYCLE START =====\n");
    
    // Save the current state for change tracking
    uint16_t old_pc = pc;
    uint16_t fetch_pc = pc;
    for (int i = 0; i < 64; i++) {
        old_registers[i] = registers[i];
    }
    for (int i = 0; i < 2048; i++) {
        old_dataMemory[i] = dataMemory[i];
    }
    
    // FETCH
    printf("\nFETCH STAGE:\n");
    uint16_t instruction = fetch();
    printf("  Fetched instruction: 0x%04X\n", instruction);
    printf("  PC: %d -> %d\n", old_pc, pc);
    
    // DECODE
    printf("\nDECODE STAGE:\n");
    struct decoded decoded_instruction = decode(instruction);
    printf("  Decoded: opcode=%d, R1=%d, R2/imm=%d\n", decoded_instruction.opcode, decoded_instruction.r1, decoded_instruction.r2);
    
    // EXECUTE
    printf("\nEXECUTE STAGE:\n");
    printf("  Instruction: ");
    switch(decoded_instruction.opcode) {
        case add_opcode: printf("ADD R%d, R%d", decoded_instruction.r1, decoded_instruction.r2); break;
        case sub_opcode: printf("SUB R%d, R%d", decoded_instruction.r1, decoded_instruction.r2); break;
        case mul_opcode: printf("MUL R%d, R%d", decoded_instruction.r1, decoded_instruction.r2); break;
        case movi_opcode: printf("MOVI R%d, %d", decoded_instruction.r1, decoded_instruction.immediate); break;
        case beqz_opcode: printf("BEQZ R%d, %d", decoded_instruction.r1, decoded_instruction.immediate); break;
        case andi_opcode: printf("ANDI R%d, %d", decoded_instruction.r1, decoded_instruction.immediate); break;
        case eor_opcode: printf("EOR R%d, R%d", decoded_instruction.r1, decoded_instruction.r2); break;
        case br_opcode: printf("BR R%d, %d", decoded_instruction.r1, decoded_instruction.r2); break;
        case sal_opcode: printf("SAL R%d, %d", decoded_instruction.r1, decoded_instruction.immediate); break;
        case sar_opcode: printf("SAR R%d, %d", decoded_instruction.r1, decoded_instruction.immediate); break;
        case ldr_opcode: printf("LDR R%d, %d", decoded_instruction.r1, decoded_instruction.address); break;
        case str_opcode: printf("STR R%d, %d", decoded_instruction.r1, decoded_instruction.address); break;
        default: printf("Unknown opcode %d", decoded_instruction.opcode);
    }
    printf("\n");
    
    // Print input values
    printf("  Input values: R%d=%d", decoded_instruction.r1, registers[decoded_instruction.r1]);
    if (decoded_instruction.opcode == add_opcode || decoded_instruction.opcode == sub_opcode || 
        decoded_instruction.opcode == mul_opcode || decoded_instruction.opcode == eor_opcode) {
        printf(", R%d=%d", decoded_instruction.r2, registers[decoded_instruction.r2]);
    }
    printf("\n");
    old_pc = pc;
    branched = 0;
    // Execute the instruction
    execute(decoded_instruction);
    
    // Check for and log register changes
    for (int i = 0; i < 64; i++) {
        if (registers[i] != old_registers[i]) {
            printf("  Register change: R%d: %d -> %d\n", 
                   i, old_registers[i], registers[i]);
        }
    }
    
    // Check for and log memory changes
    for (int i = 0; i < 2048; i++) {
        if (dataMemory[i] != old_dataMemory[i]) {
            printf("  Memory change: Address %d: %d -> %d\n", 
                   i, old_dataMemory[i], dataMemory[i]);
        }
    }
    
    if (branched) {
        printf("  Branch taken! PC updated from %d to %d\n", fetch_pc + 1, pc); // fetch_pc+1 is the PC after normal fetch
    }
    // Log flag changes
    printf("  SREG: 0x%02X\n", SREG);
    printf("  Flags: Z=%d S=%d N=%d V=%d C=%d\n",
           get_flag(Z_FLAG), get_flag(S_FLAG), get_flag(N_FLAG), 
           get_flag(V_FLAG), get_flag(C_FLAG));
    
    printf("\n===== SINGLE INSTRUCTION CYCLE END =====\n");
}

void print_data() {
    printf("Data Memory:\n");
    for (int i = 0; i<5 && i < sizeof(dataMemory) / sizeof(dataMemory[0]); i++) {// 5 is just for testing, we can change it later
        printf("Address %d: %d\n", i, dataMemory[i]);
    }
    printf("Registers:\n");
    for (int i = 0; i<5 && i < sizeof(registers) / sizeof(registers[0]); i++) {
        printf("R%d: %d\n", i, registers[i]);
    }
    printf("SREG: %x\n", SREG);
}
void print_final_state() {
    printf("\n\n===== FINAL CPU STATE =====\n\n");
    
    // Print all registers with values
    printf("Register File:\n");
    for (int i = 0; i < 64; i++) {
        printf("R%-2d: %3d (0x%02X)", i, registers[i], registers[i]);
        if ((i + 1) % 4 == 0) printf("\n");
        else printf("\t");
    }
    
    // Print special registers
    printf("\nSpecial Registers:\n");
    printf("PC: %d (0x%04X)\n", pc, pc);
    printf("SREG: 0x%02X (", SREG);
    printf("Z=%d S=%d N=%d V=%d C=%d)\n",
           get_flag(Z_FLAG), get_flag(S_FLAG), get_flag(N_FLAG),
           get_flag(V_FLAG), get_flag(C_FLAG));
    
    // Print non-zero data memory
    printf("\nData Memory (non-zero locations):\n");
    printf("Address\tValue\tAddress\tValue\tAddress\tValue\tAddress\tValue\n");
    int empty_row = 1;
    for (int i = 0; i < 2048; i += 4) {
        if (dataMemory[i] || dataMemory[i+1] || dataMemory[i+2] || dataMemory[i+3]) {
            empty_row = 0;
            printf("0x%04X: %02X\t0x%04X: %02X\t0x%04X: %02X\t0x%04X: %02X\n", 
                i, dataMemory[i], i+1, dataMemory[i+1], 
                i+2, dataMemory[i+2], i+3, dataMemory[i+3]);
        }
    }
    if (empty_row) {
        printf("(All memory locations are 0)\n");
    }
    
    // Print instruction memory content
    printf("\nInstruction Memory:\n");
    printf("Address\tValue\tDisassembled\n");
    for (int i = 0; i < *NumberofInstructions; i++) {
        struct decoded instr = decode(instructionMemory[i]);
        printf("0x%04X: 0x%04X\t", i, instructionMemory[i]);
        
        // Disassemble instruction
        switch(instr.opcode) {
            case add_opcode: printf("ADD R%d, R%d", instr.r1, instr.r2); break;
            case sub_opcode: printf("SUB R%d, R%d", instr.r1, instr.r2); break;
            case mul_opcode: printf("MUL R%d, R%d", instr.r1, instr.r2); break;
            case movi_opcode: printf("MOVI R%d, %d", instr.r1, instr.immediate); break;
            case beqz_opcode: printf("BEQZ R%d, %d", instr.r1, instr.immediate); break;
            case andi_opcode: printf("ANDI R%d, %d", instr.r1, instr.immediate); break;
            case eor_opcode: printf("EOR R%d, R%d", instr.r1, instr.r2); break;
            case br_opcode: printf("BR R%d, R%d", instr.r1, instr.r2); break;
            case sal_opcode: printf("SAL R%d, %d", instr.r1, instr.immediate); break;
            case sar_opcode: printf("SAR R%d, %d", instr.r1, instr.immediate); break;
            case ldr_opcode: printf("LDR R%d, %d", instr.r1, instr.address); break;
            case str_opcode: printf("STR R%d, %d", instr.r1, instr.address); break;
            default: printf("Unknown opcode %d", instr.opcode);
        }
        printf("\n");
    }
    printf("\n===== END OF FINAL CPU STATE =====\n");
}

void pipelined_cycle(size_t totalInstructions, size_t* fetchedCount) {
    printf("\n===== CLOCK CYCLE %d START =====\n", ++cycle_count);
    
    // Reset branch flag if pipeline is stalled
    if (!IF_ID.valid && !ID_EX.valid && *fetchedCount < totalInstructions) {
        branched = 0;
        printf("  Resetting branch flag to continue pipeline\n");
    }
    
    // ===== EXECUTE STAGE =====
    printf("EXECUTE STAGE: ");
    if (ID_EX.valid) {
        // Save old register values to detect changes
        for (int i = 0; i < 64; i++) {
            old_registers[i] = registers[i];
        }
        for (int i = 0; i < 2048; i++) {
            old_dataMemory[i] = dataMemory[i];
        }
        
        // Store current PC before potentially branching
        uint16_t current_pc = pc;
        
        // Execute the instruction
        struct decoded dec;
        dec.opcode = ID_EX.opcode;
        dec.r1 = ID_EX.r1;
        dec.r2 = ID_EX.r2;
        dec.immediate = ID_EX.imm;
        dec.address = ID_EX.imm;
        
        // Log input values to the execute stage
        printf("\n  Instruction: ");
        switch(dec.opcode) {
            case add_opcode: printf("ADD R%d, R%d", dec.r1, dec.r2); break;
            case sub_opcode: printf("SUB R%d, R%d", dec.r1, dec.r2); break;
            case mul_opcode: printf("MUL R%d, R%d", dec.r1, dec.r2); break;
            case movi_opcode: printf("MOVI R%d, %d", dec.r1, dec.immediate); break;
            case beqz_opcode: printf("BEQZ R%d, %d", dec.r1, dec.immediate); break;
            case andi_opcode: printf("ANDI R%d, %d", dec.r1, dec.immediate); break;
            case eor_opcode: printf("EOR R%d, R%d", dec.r1, dec.r2); break;
            case br_opcode: printf("BR R%d, %d", dec.r1, dec.r2); break;
            case sal_opcode: printf("SAL R%d, %d", dec.r1, dec.immediate); break;
            case sar_opcode: printf("SAR R%d, %d", dec.r1, dec.immediate); break;
            case ldr_opcode: printf("LDR R%d, %d", dec.r1, dec.address); break;
            case str_opcode: printf("STR R%d, %d", dec.r1, dec.address); break;
            default: printf("Unknown opcode %d", dec.opcode);
        }
        
        printf("\n  Input values: R%d=%d", dec.r1, ID_EX.regval1);
        if (dec.opcode == add_opcode || dec.opcode == sub_opcode || 
            dec.opcode == mul_opcode || dec.opcode == eor_opcode) {
            printf(", R%d=%d", dec.r2, ID_EX.regval2);
        }
        
        // Track if this is a branch instruction
        branched = 0;
        
        // Execute the instruction
        execute(dec);
        
        // Check for and log register changes
        for (int i = 0; i < 64; i++) {
            if (registers[i] != old_registers[i]) {
                printf("\n  Register change in EXECUTE: R%d: %d -> %d", 
                       i, old_registers[i], registers[i]);
            }
        }
        
        // Check for and log memory changes
        for (int i = 0; i < 2048; i++) {
            if (dataMemory[i] != old_dataMemory[i]) {
                printf("\n  Memory change in EXECUTE: Address %d: %d -> %d", 
                       i, old_dataMemory[i], dataMemory[i]);
            }
        }
        
        // If PC changed, log it
        if (pc != current_pc + (branched ? 0 : 1)) {
            printf("\n  PC change in EXECUTE: %d -> %d (Branch taken)", 
                   current_pc, pc);
        }
        
        ID_EX.valid = 0;
        
        // If a branch was taken, flush the pipeline
        if (branched) {
            IF_ID.valid = 0;
            printf("\n  Branch taken! Flushing pipeline. New PC: %d", pc);
        }
    } else {
        printf("No instruction\n");
    }
    
    // ===== DECODE STAGE =====
    printf("\nDECODE STAGE: ");
    if (IF_ID.valid) {
        uint16_t instruction = IF_ID.instr;
        struct decoded decoded_instruction = decode(instruction);
        
        // Log the instruction being decoded
        printf("\n  Instruction: 0x%04X", instruction);
        printf("\n  Decoded: opcode=%d, R1=%d, R2/imm=%d", 
               decoded_instruction.opcode, decoded_instruction.r1, 
               decoded_instruction.r2);
        
        // Update ID_EX pipeline register
        ID_EX.valid = 1;
        ID_EX.opcode = decoded_instruction.opcode;
        ID_EX.r1 = decoded_instruction.r1;
        ID_EX.r2 = decoded_instruction.r2;
        ID_EX.imm = decoded_instruction.immediate;
        ID_EX.regval1 = registers[decoded_instruction.r1];
        ID_EX.regval2 = registers[decoded_instruction.r2];
        ID_EX.pc_plus = IF_ID.pc_plus;
    } else {
        printf("No instruction\n");
    }
    
    // ===== FETCH STAGE =====
    printf("\nFETCH STAGE: ");
    if (!branched && *fetchedCount < totalInstructions) {
        uint16_t old_pc = pc;
        IF_ID.instr = fetch();
        IF_ID.pc_plus = pc;
        IF_ID.valid = 1;
        (*fetchedCount)++;
        printf("\n  Fetched instruction %zu: 0x%04X", *fetchedCount, IF_ID.instr);
        printf("\n  PC: %d -> %d", old_pc, pc);
    } else if (*fetchedCount >= totalInstructions) {
        IF_ID.valid = 0;
        printf("No more instructions to fetch\n");
    } else {
        printf("Stalled due to branch\n");
    }
    
    printf("\n===== CLOCK CYCLE END =====\n");
}


void main() {
    NumberofInstructions = (size_t*)malloc(sizeof(size_t));
    uint16_t* loaded_instructions = parseInstructions(NumberofInstructions); //parseInstructions() is defined in assembly_parser.c
    for (int i = 0; i < *NumberofInstructions; i++) {
        instructionMemory[i] = loaded_instructions[i];
    }
    free(loaded_instructions);

    int i = 0;
    while (i < *NumberofInstructions && pc < *NumberofInstructions) {
        single_instruction_cycle();
        i++;
    } 
    print_data(); 
    printf("Pipelined execution:\n");
    size_t totalInstructions = *NumberofInstructions;
    size_t fetchedCount = 0;
    // Reset pipeline registers and PC
    IF_ID.valid = 0;
    ID_EX.valid = 0;
    branched = 0;
    pc = 0;
    SREG = 0; 
    // Initialize registers and memory
    for (int i = 0; i < 64; i++) {
        registers[i] = 0;
    }
    for (int i = 0; i < 2048; i++) {
        dataMemory[i] = 0;
    }
    // Execution loop
    int cycle = 0;
    while (fetchedCount < totalInstructions || IF_ID.valid || ID_EX.valid) {
        cycle++;
        printf("\n===== CYCLE %d =====\n", cycle);
        printf("PC=%d, Fetched=%zu/%zu, IF_ID.valid=%d, ID_EX.valid=%d\n", pc, fetchedCount, totalInstructions, IF_ID.valid, ID_EX.valid);
        pipelined_cycle(totalInstructions, &fetchedCount);
    }

    print_final_state();
    // Print final state 
    free(NumberofInstructions);
}
