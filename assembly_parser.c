#include <stdio.h>
#include <stdint.h>
#include <string.h> 
#include <stdlib.h>  
#include "instruction_set.h"

enum instruction_type{
 register_type, 
 immediate_type
};


uint16_t getBinaryInstructionHelper(int opcode,enum instruction_type type){
    uint16_t binaryInstruction = (opcode << 12); 
    char* token = strtok(NULL, " ");

    int r1 = atoi(&token[1]);
    binaryInstruction |= (r1 << 6); 

    token = strtok(NULL, " ");
    if (type == register_type) {
        int r2 = atoi(&token[1]);
        if (r2 < 0 || r2 > 63) {
            printf("Error: Register value out of range\n");
            return -1;
        }
        binaryInstruction |= (r2 << 0); 
    } else if (type == immediate_type) {
        int immediate = atoi(token);
        if (immediate < -32 || immediate > 31) {
            printf("Error: Immediate value out of range\n");
            return -1;
        }
        binaryInstruction |= immediate & 0b111111; 
    }

    return binaryInstruction;
}

uint16_t getBinaryInstruction(char* instruction){
    uint16_t binaryInstruction = 0;
    char* token = strtok(instruction, " ");
    int opcode = 0;
  
    if (strcmp(token, "ADD") == 0) {
        opcode = add_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "SUB") == 0) {
        opcode = sub_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "MUL") == 0) {
        opcode = mul_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "MOVI") == 0) {
        opcode = movi_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "BEQZ") == 0) {
        opcode = beqz_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "ANDI") == 0) {
        opcode = andi_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "EOR") == 0) {
        opcode = eor_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "BR") == 0) {
        opcode = br_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "SAL") == 0) {
        opcode = sal_opcode;
      binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "SAR") == 0) {
        opcode = sar_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }else if (strcmp(token, "LDR") == 0) {
        opcode = ldr_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }else if (strcmp(token, "STR") == 0) {
        opcode = str_opcode;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }
    return binaryInstruction;
}

uint16_t* parseInstructions(size_t* instructionCount) {
    FILE* file = fopen("assembly.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return NULL;
    }
    // Add this check
    if (instructionCount == NULL) {
        printf("Error: NULL pointer for instructionCount\n");
        fclose(file);
        return NULL;
    }
    
    printf("Parsing instructions from file...\n");
    
   
    char line[256];
    *instructionCount = 0;
    while (fgets(line, sizeof(line), file)){
        (*instructionCount)++;
    }
    uint16_t* instructionMemory = (uint16_t*)malloc((*instructionCount) * sizeof(uint16_t));
    if (instructionMemory == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    rewind(file);
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        printf("Parsing line: %s;\n", line);
        instructionMemory[i++] = getBinaryInstruction(line);
        printf("Instruction %d:%04X\n", i, instructionMemory[i-1]);
    }

    fclose(file);
    return instructionMemory;
}
