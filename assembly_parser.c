#include <stdio.h>
#include <stdint.h>
#include <string.h> 
#include <stdlib.h>  

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
        if (immediate < 0 || immediate > 0x3F) {
            printf("Error: Immediate value out of range\n");
            return -1;
        }
        binaryInstruction |= immediate ; 
    }

    return binaryInstruction;
}

uint16_t getBinaryInstruction(char* instruction){
    uint16_t binaryInstruction = 0;
    char* token = strtok(instruction, " ");
    int opcode = 0;
  
    if (strcmp(token, "ADD") == 0) {
        opcode = 0;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "SUB") == 0) {
        opcode = 1;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "MUL") == 0) {
        opcode = 2;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "MOVI") == 0) {
        opcode = 3;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "BEQZ") == 0) {
        opcode = 4;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "ANDI") == 0) {
        opcode = 5;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "EOR") == 0) {
        opcode = 6;
        binaryInstruction = getBinaryInstructionHelper(opcode,register_type);
    } else if (strcmp(token, "BR") == 0) {
        opcode = 7;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "SAL") == 0) {
        opcode = 8;
      binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    } else if (strcmp(token, "SAR") == 0) {
        opcode = 9;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }else if (strcmp(token, "LDR") == 0) {
        opcode = 10;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }else if (strcmp(token, "STR") == 0) {
        opcode = 11;
        binaryInstruction = getBinaryInstructionHelper(opcode,immediate_type);
    }
    return binaryInstruction;
}

uint16_t* parseInstructions(void) {
    FILE* file = fopen("assembly.txt", "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return NULL;
    }

    // Allocate memory dynamically
    uint16_t* instructionMemory = (uint16_t*)malloc(1024 * sizeof(uint16_t));
    if (instructionMemory == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        // Remove newline if present
        line[strcspn(line, "\n")] = 0;
        instructionMemory[i++] = getBinaryInstruction(line);
    }

    fclose(file);
    return instructionMemory;
}
