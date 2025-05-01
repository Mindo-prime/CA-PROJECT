#include <stdio.h>
#include <stdint.h>
/*CA lab notes
- Must convert instructions to binary, then convert it to decimal which will be stored in the memory array
- read the tables instruction has an opcode. And its function.
- split the instruction in to fields according to describion
- to convert numbers to binary in c use bit masking, we can first shift left by the number of bits to the left, if it is in the center we can shift then we can and to remove bits to the right, 
- example, (85976>>decimal no of bits) & 0b1111 this indicates that it is binary
-  fetch, decode, execute then memory
- this is 90% of the project
- pipeline still didn't take it
*/
uint16_t instructionMemory[1024];
uint8_t dataMemory[2048];     
uint8_t registers[64];
   
uint8_t SREG;
uint16_t pc;

#define C_FLAG 4  // Carry Flag
#define V_FLAG 3  // Overflow Flag
#define N_FLAG 2  // Negative Flag
#define S_FLAG 1  // Sign Flag
#define Z_FLAG 0  // Zero Flag

#define NumberofInstructions 10

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
    if (result & 0x80) {
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
}
// Binary int format in c is 0b00000000000000000000000000000000 (32 bits)
void decode(int instruction);
void fetch() {
       
        int instruction = 0;
       
        // Complete the fetch() body...
        for(int i = 0;i < NumberofInstructions; i++){
            instruction = instructionMemory[i];
            decode(instruction);
            pc++;
        }
       
        // Complete the fetch() body...
       
}

void decode(int instruction) {
       
        int opcode = instruction>>12;  // bits31:26
        int r1 = (instruction>>6)&0b111111;      // bits25:21
        int r2_imm = instruction&0b111111;      // bit20:16

        // Complete the decode() body...
        switch (opcode) {
            case 0: // ADD
                printf(", ADD R%d, R%d\n",r1,r2_imm);
                break;
            case 1: // SUB
                printf(", SUB R%d, R%d\n",r1,r2_imm);
                break;
            case 2: // MUL
                printf(", MUL R%d, R%d\n",r1,r2_imm);
                break;
            case 3: // ANDI
                printf(", ANDI R%d, R%d\n",r1,r2_imm);
                break;
            case 4: // EOR
                printf(", EOR R%d, R%d\n",r1,r2_imm);
                break;
            case 5: // LDM
                printf(", LDM R%d, R%d\n",r1,r2_imm);
                break;
            case 6: // STM
                printf(", STM R%d, R%d\n",r1,r2_imm);
                break;
            case 7: // SAL
                printf(", SAL R%d, %d\n",r1, r2_imm);
                break;
            case 8: // SAR
                printf(", SAR R%d, %d\n",r1, r2_imm);
                break;
            case 9: // JMP
                printf(", JMP %d\n", r2_imm);
                break;
            case 10: // JR
                printf(", JR R%d, R%d\n",r1,r2_imm);
                break;
            case 11: // BEQZ
                printf(", BEQZ R%d, %d\n",r1, r2_imm);
                break;
            default:
                printf(", Unknown opcode\n");
        }
       
        // Printings
       
        printf("Instruction %i\n",pc);
                printf("opcode = %i\n",opcode);
                printf("rs = %i\n",r1);
                printf("rt = %i\n",r2_imm);
                printf("---------- \n");
            
}

void main() {
          
    fetch();
   
    // Expected output
   
    /*
   
    Instruction 0
    opcode = -5 (signed) or 11 (unsigned)
    rs = 1
    rt = 4
    rd = 2
    shift amount = 8
    function = 2873
    immediate = 166713
    address = 21138233
    value[rs] = 16
    value[rt] = 102
   
    ----------
   
    Instruction 1
    opcode = 3
    rs = 11
    rt = 2
    rd = 12
    shift amount = 9
    function = 946
    immediate = 824242
    address = 187470770
    value[rs] = 9
    value[rt] = 87
   
    ----------
   
    */ 
               
}