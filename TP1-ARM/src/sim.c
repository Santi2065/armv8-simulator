#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"

void process_instruction()
{
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    uint32_t opcode = (instruction >> 21) & 0x7FF;
    uint32_t opcode_high = (instruction >> 24) & 0xFF;  // Los 8 bits más altos

    uint32_t Rd = instruction & 0x1F;
    uint32_t Rn = (instruction >> 5) & 0x1F; 
    uint32_t Rm = (instruction >> 16) & 0x1F; 
    
    printf("PC: 0x%016lX | Instruction: 0x%08X | Opcode: 0x%X\n", 
       (unsigned long) CURRENT_STATE.PC, instruction, opcode);
    printf("Instrucción: 0x%08X, opcode_high: 0x%X, primeros 8 bits: 0x%X\n", 
       instruction, opcode_high, instruction >> 24);

    // Caso especial para instrucciones B.Cond (comienzan con 0x54)
    if (opcode_high == 0x54) {
        int flag_n = CURRENT_STATE.FLAG_N;
        int flag_z = CURRENT_STATE.FLAG_Z;
        uint8_t cond = instruction & 0xF;
        int32_t imm19 = (instruction >> 5) & 0x7FFFF;  // Inmediato de 19 bits
    
        // Extensión de signo del inmediato
        if (imm19 & (1 << 18)) {
            imm19 |= ~((1 << 19) - 1);
        }
        imm19 <<= 2;
    
        uint64_t new_address = CURRENT_STATE.PC + imm19;
    
        printf("B.Cond | cond: 0x%X | flag_n: %d | flag_z: %d | imm19: 0x%X | new_address: 0x%016lX\n", 
               cond, flag_n, flag_z, imm19, new_address);
    
        int should_branch = 0;
        switch (cond) {
            case 0x0: // BEQ
                should_branch = (flag_z == 1);
                break;
            case 0x1: // BNE
                should_branch = (flag_z == 0);
                break;
            case 0xC: // BGT
                should_branch = (flag_z == 0 && flag_n == 0);
                break;
            case 0xB: // BLT
                should_branch = (flag_n == 1);
                break;
            case 0xA: // BGE
                should_branch = (flag_n == 0);
                break;
            case 0xD: // BLE
                should_branch = (flag_z == 1 || flag_n == 1);
                break;
            default:
                printf("B.Cond: Unknown condition 0x%X\n", cond);
                NEXT_STATE.PC = CURRENT_STATE.PC + 4;
                return;
        }
    
        if (should_branch) {
            printf("B.Cond: Jumping to address 0x%016lX\n", new_address);
            NEXT_STATE.PC = new_address;
        } else {
            printf("B.Cond: Not jumping\n");
            NEXT_STATE.PC = CURRENT_STATE.PC + 4;
        }
        
        return;
    } else {

        // Switch regular para el resto de instrucciones
        switch (opcode) {
            case 0x6A2:  // HLT
                RUN_BIT = FALSE;  // Detener simulación
                break;

            case 0x558: {  // ADDS Register

                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];  // Manejo de XZR
                int64_t reg_Xm = (Rm == 31) ? 0 : CURRENT_STATE.REGS[Rm];

                int64_t result = reg_Xn + reg_Xm;
                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;  // XZR permanece en 0

                NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
                NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
                break;
            }

            case 0x758: {  // SUBS Register (también implementa CMP Register cuando Rd=31/XZR)
       
                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t reg_Xm = (Rm == 31) ? 0 : CURRENT_STATE.REGS[Rm];
                int64_t result = reg_Xn - reg_Xm;
                
                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;
                
                // Actualizar FLAGS
                NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
                NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
                break;
            }
            
            case 0x588: {  // ADDS Immediate

                uint32_t imm12 = (instruction >> 10) & 0xFFF;

                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t imm = imm12;
                int64_t result = reg_Xn + imm;

                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;

                NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
                NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
                break;
            }

            case 0x788: {  // SUBS Immediate (también implementa CMP Immediate cuando Rd=31/XZR)

                uint32_t imm12 = (instruction >> 10) & 0xFFF;
                uint32_t shift = (instruction >> 22) & 0x3;
                
                // Aplicar shift si es necesario (01 = LSL #12)
                if (shift == 1) {
                    imm12 <<= 12;
                }
                
                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t result = reg_Xn - imm12;
                
                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;
                
                // Actualizar FLAGS
                NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
                NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
                break;
            }
            case 0x750: {  // ANDS (Shifted Register)

                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t reg_Xm = (Rm == 31) ? 0 : CURRENT_STATE.REGS[Rm];
                int64_t result = reg_Xn & reg_Xm;

                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;

                // Actualizar FLAGS
                NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
                NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
                break;
            }
            case 0x650: {  // EOR (Shifted Register)

                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t reg_Xm = (Rm == 31) ? 0 : CURRENT_STATE.REGS[Rm];
                int64_t result = reg_Xn ^ reg_Xm;

                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;
                break;
            }
            case 0x550: {  // ORR (Shifted Register)

                int64_t reg_Xn = (Rn == 31) ? 0 : CURRENT_STATE.REGS[Rn];
                int64_t reg_Xm = (Rm == 31) ? 0 : CURRENT_STATE.REGS[Rm];
                int64_t result = reg_Xn | reg_Xm;

                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;
                break;
            }
            case 0x0A0: {  // B (Branch)
                int32_t imm26 = (instruction & 0x03FFFFFF);  
                
                // Sign-extend el immediate a 64 bits y multiplicar por 4 (instrucciones de 4 bytes)
                int64_t offset = ((int64_t)((int32_t)(imm26 << 6)) >> 6) * 4;

                NEXT_STATE.PC = CURRENT_STATE.PC + offset;
                break;
            }
            case 0x6B0: {  // BR (Branch Register)

                NEXT_STATE.PC = CURRENT_STATE.REGS[Rn];
                break;
            }
            case 0x694: {  // MOVZ

                uint32_t imm16 = (instruction >> 5) & 0xFFFF;  // Extraer immediate de 16 bits
                uint32_t hw = (instruction >> 21) & 0x3;  // Extraer hw (shift amount)
                
                // De acuerdo a la consigna, solo implementamos para hw = 0
                int64_t result = imm16;
                
                // Si hw no es 0,  mostramos una advertencia pero continuamos con hw = 0
                if (hw != 0) {
                    printf("MOVZ: Advertencia - hw != 0 no implementado, usando hw = 0\n");
                }
                
                NEXT_STATE.REGS[Rd] = (Rd == 31) ? 0 : result;
                break;
            }

            case 0x69B: {  // LSL (Immediate), e.g., lsl X4, X3, 4
  
                    uint32_t immr = (instruction >> 10) & 0x3F;  

                    uint64_t shift = 63 - immr; 
                    uint64_t src = CURRENT_STATE.REGS[Rn];
                    uint64_t result = src << shift;
                    if (Rd != 31) NEXT_STATE.REGS[Rd] = result;
                    printf("LSL: X%u = 0x%" PRIX64 " << %" PRIu64 " -> X%u = 0x%" PRIX64 "\n", Rn, src, shift, Rd, result);

                    NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[Rd] == 0) ? 1 : 0;
                    NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[Rd] < 0) ? 1 : 0;


                    break;
                }

            case 0x69A:// LSR
                {   

                        uint32_t imms = (instruction >> 16) & 0x3F;  // imms

                        uint64_t shift = imms; 
                        uint64_t src = CURRENT_STATE.REGS[Rn];
                        uint64_t result = src >> shift;
                        if (Rd != 31) NEXT_STATE.REGS[Rd] = result;
                        printf("LSR: X%u = 0x%" PRIX64 " >> %" PRIu64 " -> X%u = 0x%" PRIX64 "\n", Rn, src, shift, Rd, result);

                        NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[Rd] == 0) ? 1 : 0;
                        NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[Rd] < 0) ? 1 : 0;


                        break;
                }
            case 0x7c0: // STUR 
            {
                int32_t imm9 = (instruction >> 12) & 0x1FF;     

                if (imm9 & 0x100) { 
                    imm9 |= 0xFFFFFE00; 
                } 
               
                uint32_t address = CURRENT_STATE.REGS[Rn] + imm9;     
                mem_write_32(address, CURRENT_STATE.REGS[Rd]); 
            }
            break;
            case 0x1c0: // STURB 
                {

                    int32_t imm9 = (instruction >> 12) & 0x1FF;
                    if (imm9 & 0x100){ 
                        imm9 |= 0xFFFFFE00; 
                    }

                uint32_t address = CURRENT_STATE.REGS[Rn] + imm9;
                uint32_t value = mem_read_32(address);
                value = (value & 0xFFFFFF00) | (CURRENT_STATE.REGS[Rd] & 0xFF);
                mem_write_32(address, value);
                }
                break;
            case 0x3E1: // STURH 
                {

                    int32_t imm9 = (instruction >> 12) & 0x1FF;
                    if (imm9 & 0x100) { 
                        imm9 |= 0xFFFFFE00; 
                    }

                    uint32_t address = CURRENT_STATE.REGS[Rn] + imm9;
                    uint32_t value = mem_read_32(address);
                    value = (value & 0xFFFF0000) | (CURRENT_STATE.REGS[Rd] & 0xFFFF);
                    mem_write_32(address, value);
                }
                break;
            case 0x7c2: // LDUR 
                {
                    int32_t imm9 = (instruction >> 12) & 0x1FF;
                    if (imm9 & 0x100) { 
                        imm9 |= 0xFFFFFE00; 
                    }

                    uint32_t address = CURRENT_STATE.REGS[Rn] + imm9;
                    uint32_t half_value_1 = mem_read_32(address);
                    uint32_t half_value_2 = mem_read_32(address + 4); 
                    uint64_t value = ((uint64_t)half_value_2 << 32) | half_value_1;
                    if (Rd != 31) NEXT_STATE.REGS[Rd] = value;
                }
                break;
            case 0x1c2: // LDURB 
                {
                    int32_t imm9 = (instruction >> 12) & 0x1FF;
                    uint64_t address = CURRENT_STATE.REGS[Rn] + imm9;
                    uint32_t byte_value = mem_read_32(address);
                    uint64_t value = (uint64_t)byte_value; 
                    if (Rd != 31) NEXT_STATE.REGS[Rd] = value;
                }
                break;
            case 0x3c2: // LDURH 
                {
                    int32_t imm9 = (instruction >> 12) & 0x1FF;
                    uint64_t address = CURRENT_STATE.REGS[Rn] + imm9;
                    uint32_t half_value = mem_read_32(address);
                    uint64_t value = (uint64_t)half_value;
                    if (Rd != 31) NEXT_STATE.REGS[Rd] = half_value;
                }
                break;


            default:
                printf("Instrucción desconocida: %x\n", opcode);
                break;
        }
    }

    // Actualiza el PC para instrucciones que no sean saltos explícitos
    if (opcode != 0x0A0 && opcode != 0x6B0 && opcode_high != 0x54) {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
}
