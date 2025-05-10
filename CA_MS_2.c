//gcc program CA_MS_2.c assembly_parser.c -o CA_MS_2
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

#define C_FLAG 4  // Carry Flag
#define V_FLAG 3  // Overflow Flag
#define N_FLAG 2  // Negative Flag
#define S_FLAG 1  // Sign Flag
#define Z_FLAG 0  // Zero Flag

extern uint16_t* parseInstructions(void);

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

        printf("Instruction %i\n",pc);
        printf("opcode = %i\n",decoded_instruction.opcode);
        printf("r1 = %i\n",decoded_instruction.r1);
        printf("r2 = %i\n",decoded_instruction.r2);
        printf("Immediate = %i\n",decoded_instruction.r2);
        printf("Address = %i\n",decoded_instruction.r2);
        printf("---------- \n");

        return decoded_instruction;    
}

void execute(struct decoded dec) {
    int carry = 0;
    int overflow =0;
    uint8_t result =0;
    switch (dec.opcode) {
        case 0: // ADD
            printf(", ADD R%d, R%d\n",dec.r1,dec.r2);
            result = registers[dec.r1] = registers[dec.r1] + registers[dec.r2];
            carry = ((int)registers[dec.r1] + (int)registers[dec.r2]) > 0xFF;
            overflow = (~(registers[dec.r1] ^ registers[dec.r2]) & (registers[dec.r1] ^ result) & 0x80) ? 1 : 0;
            break;
        case 1: // SUB
            printf(", SUB R%d, R%d\n",dec.r1,dec.r2);
            registers[dec.r1] = registers[dec.r1] - registers[dec.r2];
            carry = registers[dec.r1] < registers[dec.r2];
            overflow = (((registers[dec.r1] ^ registers[dec.r2]) & (registers[dec.r1] ^ result)) & 0x80) ? 1 : 0;
            registers[dec.r1] = result;
            break;
        case 2: // MUL
            printf(", MUL R%d, R%d\n",dec.r1,dec.r2);
            result = registers[dec.r1] = registers[dec.r1] * registers[dec.r2];
            carry = ((int)registers[dec.r1] * (int)registers[dec.r2]) > 0xFF;
            overflow = 0; 
            break;
        case 3: // MOVI
            printf(", MOVI R%d, R%d\n",dec.r1,dec.immediate);
            result = registers[dec.r1] = dec.immediate;
            break;
        case 4: // BEQZ
            printf(", BEQZ R%d, R%d\n",dec.r1,dec.immediate);
            if (registers[dec.r1] == 0)
                pc += dec.immediate; //+1 was already done in fetch phase
            break;
        case 5: // ANDI
            printf(", AND R%d, R%d\n",dec.r1,dec.immediate);
            result = registers[dec.r1] = registers[dec.r1] & dec.immediate;
            break;
        case 6: // EOR
            printf(",EOR R%d, R%d\n",dec.r1,dec.r2);
            result = registers[dec.r1] = registers[dec.r1] ^ registers[dec.r2];
            break;
        case 7: // BR
            printf(", BR R%d, %d\n",dec.r1, dec.r2);
            pc = (8<<registers[dec.r1]) | registers[dec.r2]; 
            break;
        case 8: // SAL
            printf(", SAL R%d, %d\n",dec.r1, dec.immediate);
            registers[dec.r1] = registers[dec.r1] << dec.immediate; 
            break;  
        case 9: // SAR
            printf(", SAR %d\n", dec.immediate); //do we have to simulate sign extension?
            registers[dec.r1] =  registers[dec.r1]>>dec.immediate;
            break;
        case 10: // LDR
            printf(", LB R%d, R%d\n",dec.r1,dec.address);
            registers[dec.r1] = dataMemory[dec.address];
            break;
        case 11: // STR
            printf(", SB R%d, %d\n",dec.r1, dec.address);
            dataMemory[dec.address] = registers[dec.r1];
            break;
        default:
            printf(", Unknown opcode\n");
    }
    update_flags(result,carry,overflow);
}

void instruction_cycle(){
    uint16_t instruction = fetch();
    struct decoded decoded_instruction = decode(instruction);
    execute(decoded_instruction);
}

void main() {
    uint16_t* loaded_instructions = parseInstructions(); //parseInstructions() is defined in assembly_parser.c
    size_t NumberofInstructions = sizeof(loaded_instructions)/sizeof(loaded_instructions[0]);
    for (int i = 0; i < NumberofInstructions; i++) {
        instructionMemory[i] = loaded_instructions[i];
    }
    free(loaded_instructions);
    for (int i = 0; i < NumberofInstructions; i++) {
        instruction_cycle();
    }           
}