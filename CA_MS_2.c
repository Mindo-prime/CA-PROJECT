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
    if (N_FLAG ^ V_FLAG){
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
            if (registers[dec.r1] == 0)
                pc += dec.immediate; //+1 was already done in fetch phase
                branched = 1;
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
            pc = (registers[dec.r1] >> 8) | registers[dec.r2];// Not sure if that is correct bro my friend said r1 >>  8
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
    uint16_t instruction = fetch();
    struct decoded decoded_instruction = decode(instruction);
    execute(decoded_instruction);
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
struct decoded *decodedInstruction;
void pipelined_cycle(){
    //execute_stage
    if(ID_EX.valid){
        execute(*decodedInstruction);
        if(branched){
            ID_EX.valid = 0;
            IF_ID.valid = 0;
            branched = 0;
        }
    }
    //decode_stage;
    ID_EX.valid = IF_ID.valid;
    ID_EX.pc_plus = IF_ID.pc_plus;
    if (IF_ID.valid) {
        *decodedInstruction = decode(IF_ID.instr);
        
        ID_EX.opcode = decodedInstruction->opcode;
        ID_EX.r1 = decodedInstruction->r1;
        ID_EX.r2 = decodedInstruction->r2;
        ID_EX.imm = decodedInstruction->immediate;
        ID_EX.regval1 = registers[decodedInstruction->r1];
        ID_EX.regval2 = registers[decodedInstruction->r2];
    }
    //fetch_stage
    if(pc <1024){
        IF_ID.instr = fetch();
        IF_ID.pc_plus = pc;
        IF_ID.valid = 1;
    }else{
        IF_ID.valid = 0;
    }
}

void main() {
    size_t* NumberofInstructions;
    NumberofInstructions = (size_t*)malloc(sizeof(size_t));
    uint16_t* loaded_instructions = parseInstructions(NumberofInstructions); //parseInstructions() is defined in assembly_parser.c
    for (int i = 0; i < *NumberofInstructions; i++) {
        instructionMemory[i] = loaded_instructions[i];
    }
    free(loaded_instructions);
    /*for (int i = 0; i < *NumberofInstructions; i++) {
        single_instruction_cycle();
    } */       
    print_data(); 
    printf("Pipelined execution:\n"); 
    IF_ID.valid = 0;
    ID_EX.valid = 0;
    pc = 0;
    printf("Pipelined execution:\n");
    decodedInstruction = (struct decoded*)malloc(sizeof(struct decoded));
    while(valid == 0|| IF_ID.valid == 1 || ID_EX.valid == 1){ 
        valid = 1;
        pipelined_cycle();
    }    
    print_data(); 
}
