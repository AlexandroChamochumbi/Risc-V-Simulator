#include <vector>
#include <cstdint>
#include <algorithm>
#include <emscripten.h>

using namespace std;

const size_t MEM_SIZE = 1 << 24; // 16 MB

class RV32Sim {
public:
    int32_t regs[32];
    vector<uint8_t> mem;
    uint32_t pc;
    uint32_t cycle;
    bool halted;

    RV32Sim() { reset(); }

    void reset() {
        fill(begin(regs), end(regs), 0);
        mem.assign(MEM_SIZE, 0);
        pc = 0;
        cycle = 0;
        halted = false;
    }

    uint8_t readByte(uint32_t addr) { return mem[addr & 0xFFFFFF]; }
    uint16_t readHalf(uint32_t addr) { return mem[addr & 0xFFFFFF] | (mem[(addr+1) & 0xFFFFFF] << 8); }
    uint32_t readWord(uint32_t addr) {
        return mem[addr & 0xFFFFFF] | (mem[(addr+1) & 0xFFFFFF] << 8) | 
               (mem[(addr+2) & 0xFFFFFF] << 16) | (mem[(addr+3) & 0xFFFFFF] << 24);
    }
    
    void writeByte(uint32_t addr, uint8_t v) { mem[addr & 0xFFFFFF] = v; }
    void writeHalf(uint32_t addr, uint16_t v) { 
        mem[addr & 0xFFFFFF] = v & 0xFF; 
        mem[(addr+1) & 0xFFFFFF] = (v >> 8) & 0xFF; 
    }
    void writeWord(uint32_t addr, uint32_t v) {
        mem[addr & 0xFFFFFF] = v & 0xFF; 
        mem[(addr+1) & 0xFFFFFF] = (v >> 8) & 0xFF;
        mem[(addr+2) & 0xFFFFFF] = (v >> 16) & 0xFF; 
        mem[(addr+3) & 0xFFFFFF] = (v >> 24) & 0xFF;
    }

    int32_t sext12(uint32_t v) { return ((int32_t)(v << 20)) >> 20; }
    int32_t sext13(uint32_t v) { return ((int32_t)(v << 19)) >> 19; }
    int32_t sext20(uint32_t v) { return ((int32_t)(v << 12)) >> 12; }
    int32_t sext21(uint32_t v) { return ((int32_t)(v << 11)) >> 11; }

    int32_t getReg(int r) { return r == 0 ? 0 : regs[r]; }
    void setReg(int r, int32_t v) { if (r != 0) regs[r] = v; }

    void step() {
        if (halted || pc >= MEM_SIZE) { halted = true; return; }

        uint32_t instr = readWord(pc);
        if (instr == 0) { halted = true; return; } // Fin de programa

        uint32_t opcode = instr & 0x7F;
        uint32_t rd = (instr >> 7) & 0x1F;
        uint32_t funct3 = (instr >> 12) & 0x7;
        uint32_t rs1 = (instr >> 15) & 0x1F;
        uint32_t rs2 = (instr >> 20) & 0x1F;
        uint32_t funct7 = (instr >> 25) & 0x7F;

        uint32_t nextPC = pc + 4;

        int32_t immI = sext12(instr >> 20);
        int32_t immS = sext12(((instr >> 25) << 5) | ((instr >> 7) & 0x1F));
        int32_t immB = sext13((((instr >> 31) & 1) << 12) | (((instr >> 7) & 1) << 11) |
                              (((instr >> 25) & 0x3F) << 5) | (((instr >> 8) & 0xF) << 1));
        uint32_t immU = instr & 0xFFFFF000;
        int32_t immJ = sext21((((instr >> 31) & 1) << 20) | (((instr >> 12) & 0xFF) << 12) |
                              (((instr >> 20) & 1) << 11) | (((instr >> 21) & 0x3FF) << 1));

        int32_t r1 = getReg(rs1);
        int32_t r2 = getReg(rs2);
        uint32_t ur1 = (uint32_t)r1;
        uint32_t ur2 = (uint32_t)r2;

        switch (opcode) {
            case 0x03: // LOAD
                switch (funct3) {
                    case 0x0: setReg(rd, (int32_t)(int8_t)readByte(ur1 + immI)); break;
                    case 0x1: setReg(rd, (int32_t)(int16_t)readHalf(ur1 + immI)); break;
                    case 0x2: setReg(rd, (int32_t)readWord(ur1 + immI)); break;
                    case 0x4: setReg(rd, readByte(ur1 + immI)); break;
                    case 0x5: setReg(rd, readHalf(ur1 + immI)); break;
                } break;
            case 0x13: // OP-IMM
                switch (funct3) {
                    case 0x0: setReg(rd, r1 + immI); break;
                    case 0x1: setReg(rd, r1 << rs2); break;
                    case 0x2: setReg(rd, r1 < immI ? 1 : 0); break;
                    case 0x3: setReg(rd, ur1 < (uint32_t)immI ? 1 : 0); break;
                    case 0x4: setReg(rd, r1 ^ immI); break;
                    case 0x5: setReg(rd, funct7 == 0x20 ? (r1 >> rs2) : (ur1 >> rs2)); break;
                    case 0x6: setReg(rd, r1 | immI); break;
                    case 0x7: setReg(rd, r1 & immI); break;
                } break;
            case 0x17: setReg(rd, pc + immU); break; // AUIPC
            case 0x23: // STORE
                switch (funct3) {
                    case 0x0: writeByte(ur1 + immS, (uint8_t)r2); break;
                    case 0x1: writeHalf(ur1 + immS, (uint16_t)r2); break;
                    case 0x2: writeWord(ur1 + immS, (uint32_t)r2); break;
                } break;
            case 0x33: // OP
                switch ((funct7 << 3) | funct3) {
                    case 0x000: setReg(rd, r1 + r2); break;
                    case 0x100: setReg(rd, r1 - r2); break;
                    case 0x001: setReg(rd, r1 << (ur2 & 31)); break;
                    case 0x002: setReg(rd, r1 < r2 ? 1 : 0); break;
                    case 0x003: setReg(rd, ur1 < ur2 ? 1 : 0); break;
                    case 0x004: setReg(rd, r1 ^ r2); break;
                    case 0x005: setReg(rd, ur1 >> (ur2 & 31)); break;
                    case 0x105: setReg(rd, r1 >> (ur2 & 31)); break;
                    case 0x006: setReg(rd, r1 | r2); break;
                    case 0x007: setReg(rd, r1 & r2); break;
                } break;
            case 0x37: setReg(rd, immU); break; // LUI
            case 0x63: // BRANCH
                switch (funct3) {
                    case 0x0: if (r1 == r2) nextPC = pc + immB; break;
                    case 0x1: if (r1 != r2) nextPC = pc + immB; break;
                    case 0x4: if (r1 < r2) nextPC = pc + immB; break;
                    case 0x5: if (r1 >= r2) nextPC = pc + immB; break;
                    case 0x6: if (ur1 < ur2) nextPC = pc + immB; break;
                    case 0x7: if (ur1 >= ur2) nextPC = pc + immB; break;
                } break;
            case 0x67: setReg(rd, pc + 4); nextPC = (ur1 + immI) & ~1; break; // JALR
            case 0x6F: setReg(rd, pc + 4); nextPC = pc + immJ; break; // JAL
           case 0x73: // SYSTEM (ecall / ebreak)
                if (instr == 0x00000073) { // ecall
                    int32_t a7 = getReg(17); // x17
                    int32_t a0 = getReg(10); // x10
                    
                    if (a7 == 1) { // Imprimir Entero
                        EM_ASM({ logMsg("[ecall 1] Imprime: " + $0, 'ok'); }, a0);
                    } 
                    else if (a7 == 4) { // Imprimir Cadena (String)
                        // Pasamos la dirección (a0) a JS y leemos la memoria del simulador
                        EM_ASM({
                            let str = '';
                            let addr = $0 >>> 0;
                            while (true) {
                                let c = Module._sim_read_mem(addr++);
                                if (c === 0 || str.length > 256) break; // Terminador nulo o límite
                                str += String.fromCharCode(c);
                            }
                            logMsg("[ecall 4] " + str, 'ok');
                        }, a0);
                    }
                    else if (a7 == 11) { // Imprimir Carácter
                        EM_ASM({ logMsg("[ecall 11] Carácter: '" + String.fromCharCode($0) + "'", 'ok'); }, a0);
                    } 
                    else if (a7 == 10) { // Salida limpia (Exit)
                        halted = true;
                        EM_ASM({ logMsg("[ecall 10] Fin del programa (Exit)", 'warn'); });
                    } 
                    else if (a7 == 5) { // ecall 5: Leer Entero (Read Integer)
                        // JS pide el número y se lo devuelve a C++ a través de EM_ASM_INT
                        int32_t val = EM_ASM_INT({
                            let input = prompt("El programa solicita un número entero:", "0");
                            return parseInt(input) || 0; // Si escribe letras, devuelve 0
                        });
                        setReg(10, val); // Guardar el resultado en a0
                        EM_ASM({ logMsg("[ecall 5] Ingresó el número: " + $0, 'ok'); }, val);
                    }
                    else if (a7 == 12) { // ecall 12: Leer Carácter (Read Char)
                        int32_t val = EM_ASM_INT({
                            let input = prompt("El programa solicita un carácter:", "");
                            return (input && input.length > 0) ? input.charCodeAt(0) : 0;
                        });
                        setReg(10, val); // Guardar el código ASCII en a0
                        EM_ASM({ logMsg("[ecall 12] Ingresó el carácter: '" + String.fromCharCode($0) + "'", 'ok'); }, val);
                    }
                    else if (a7 == 8) { // ecall 8: Leer Cadena (Read String)
                        // Para cadenas, a0 tiene la dirección de memoria y a1 el tamaño máximo
                        int32_t maxLen = getReg(11); // a1 (x11)
                        EM_ASM({
                            let input = prompt("El programa solicita un texto (máx " + $1 + " caracteres):", "");
                            if (!input) input = "";
                            let addr = $0 >>> 0;
                            let maxLen = $1;
                            let i = 0;
                            
                            // Escribimos el texto directo en la memoria del simulador (HEAPU8)
                            for (; i < input.length && i < maxLen - 1; i++) {
                                HEAPU8[addr + i] = input.charCodeAt(i);
                            }
                            HEAPU8[addr + i] = 0; // Terminador nulo obligatorio
                            
                            logMsg("[ecall 8] Ingresó el texto: " + input, 'ok');
                        }, a0, maxLen);
                    }
                    else {
                        EM_ASM({ logMsg("[ecall] Syscall no implementado: " + $0, 'err'); }, a7);
                    }
                } else if (instr == 0x00100073) { // ebreak
                    halted = true;
                    EM_ASM({ logMsg("Pausa por ebreak", 'warn'); });
                }
                break;
        }
        pc = nextPC;
        cycle++;
    }
};

RV32Sim sim;

// --- PUENTE WEBASSEMBLY ---
extern "C" {
    EMSCRIPTEN_KEEPALIVE void sim_reset() { sim.reset(); }
    EMSCRIPTEN_KEEPALIVE void sim_load_bin(uint8_t* buffer, int size) {
        sim.reset();
        for(int i = 0; i < size; i++) { sim.writeByte(i, buffer[i]); }
    }
    EMSCRIPTEN_KEEPALIVE int sim_step() { sim.step(); return sim.halted ? 1 : 0; }
    EMSCRIPTEN_KEEPALIVE uint32_t sim_get_pc() { return sim.pc; }
    EMSCRIPTEN_KEEPALIVE uint32_t sim_get_cycle() { return sim.cycle; }
    EMSCRIPTEN_KEEPALIVE int32_t sim_get_reg(int r) { return sim.getReg(r); }
    EMSCRIPTEN_KEEPALIVE uint8_t sim_read_mem(uint32_t addr) { return sim.readByte(addr); }
    EMSCRIPTEN_KEEPALIVE uint32_t sim_read_instr(uint32_t addr) { return sim.readWord(addr); }
}