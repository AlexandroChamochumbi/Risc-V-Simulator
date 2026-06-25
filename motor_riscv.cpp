#include <vector>
#include <cstdint>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
// Librerías de Redes para compilación nativa (GDB)
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif
#endif

using namespace std;

// Estructura del Header principal del archivo ELF
struct Elf32_Ehdr {
    unsigned char e_ident[16]; 
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;     
    uint32_t      e_phoff;     
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;    
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
};

// Estructura del Program Header (cada segmento a cargar)
struct Elf32_Phdr {
    uint32_t p_type;   
    uint32_t p_offset; 
    uint32_t p_vaddr;  
    uint32_t p_paddr;
    uint32_t p_filesz; 
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

const size_t MEM_SIZE = 1 << 24;

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
        if (instr == 0) { halted = true; return; } 

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
            case 0x03: 
                switch (funct3) {
                    case 0x0: setReg(rd, (int32_t)(int8_t)readByte(ur1 + immI)); break;
                    case 0x1: setReg(rd, (int32_t)(int16_t)readHalf(ur1 + immI)); break;
                    case 0x2: setReg(rd, (int32_t)readWord(ur1 + immI)); break;
                    case 0x4: setReg(rd, readByte(ur1 + immI)); break;
                    case 0x5: setReg(rd, readHalf(ur1 + immI)); break;
                } break;
            case 0x13: 
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
            case 0x17: setReg(rd, pc + immU); break; 
            case 0x23: 
                switch (funct3) {
                    case 0x0: writeByte(ur1 + immS, (uint8_t)r2); break;
                    case 0x1: writeHalf(ur1 + immS, (uint16_t)r2); break;
                    case 0x2: writeWord(ur1 + immS, (uint32_t)r2); break;
                } break;
            case 0x33: 
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
            case 0x37: setReg(rd, immU); break; 
            case 0x63: 
                switch (funct3) {
                    case 0x0: if (r1 == r2) nextPC = pc + immB; break;
                    case 0x1: if (r1 != r2) nextPC = pc + immB; break;
                    case 0x4: if (r1 < r2) nextPC = pc + immB; break;
                    case 0x5: if (r1 >= r2) nextPC = pc + immB; break;
                    case 0x6: if (ur1 < ur2) nextPC = pc + immB; break;
                    case 0x7: if (ur1 >= ur2) nextPC = pc + immB; break;
                } break;
            case 0x67: setReg(rd, pc + 4); nextPC = (ur1 + immI) & ~1; break; 
            case 0x6F: setReg(rd, pc + 4); nextPC = pc + immJ; break; 
            case 0x73: 
                if (instr == 0x00000073) { // ecall
                    int32_t a7 = getReg(17); 
                    int32_t a0 = getReg(10); 
                    
                    if (a7 == 1) { 
                        #ifdef __EMSCRIPTEN__
                            EM_ASM({ logMsg("[ecall 1] Imprime: " + $0, 'ok'); }, a0);
                        #else
                            cout << "[ecall 1] Imprime: " << a0 << endl;
                        #endif
                    } 
                    else if (a7 == 10) { 
                        halted = true;
                        #ifdef __EMSCRIPTEN__
                            EM_ASM({ logMsg("[ecall 10] Fin del programa (Exit)", 'warn'); });
                        #else
                            cout << "[ecall 10] Fin del programa (Exit)" << endl;
                        #endif
                    } 
                    else if (a7 == 5) {
                        #ifdef __EMSCRIPTEN__
                            int32_t val = EM_ASM_INT({
                                let input = prompt("El programa solicita un número entero:", "0");
                                return parseInt(input) || 0;
                            });
                        #else
                            int32_t val;
                            cout << "El programa solicita un número entero: ";
                            cin >> val;
                        #endif
                        setReg(10, val);
                    }
                    else {
                        #ifdef __EMSCRIPTEN__
                            EM_ASM({ logMsg("[ecall] Syscall no implementado: " + $0, 'err'); }, a7);
                        #else
                            cout << "[ecall] Syscall no implementado: " << a7 << endl;
                        #endif
                    }
                } else if (instr == 0x00100073) { // ebreak
                    halted = true;
                    #ifdef __EMSCRIPTEN__
                        EM_ASM({ logMsg("Pausa por ebreak", 'warn'); });
                    #else
                        cout << "Pausa por ebreak" << endl;
                    #endif
                }
                break;
        }
        pc = nextPC;
        cycle++;
    }
};

RV32Sim sim;

// ========================================================================
// 1. INTERFAZ WEB (SE COMPILA SÓLO CON EMSCRIPTEN)
// ========================================================================
#ifdef __EMSCRIPTEN__
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
    EMSCRIPTEN_KEEPALIVE int sim_load_elf(uint8_t* elf_data, int size) {
        if (size < sizeof(Elf32_Ehdr)) return -1;
        sim.reset();
        Elf32_Ehdr* ehdr = (Elf32_Ehdr*)elf_data;
        if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || 
            ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
            return -2;
        }
        uint8_t* phdr_base = elf_data + ehdr->e_phoff;
        for (int i = 0; i < ehdr->e_phnum; i++) {
            Elf32_Phdr* phdr = (Elf32_Phdr*)(phdr_base + i * ehdr->e_phentsize);
            if (phdr->p_type == 1) { 
                if (phdr->p_vaddr + phdr->p_memsz <= MEM_SIZE) {
                    memcpy(&sim.mem[phdr->p_vaddr], elf_data + phdr->p_offset, phdr->p_filesz);
                }
            }
        }
        sim.pc = ehdr->e_entry; 
        return 1; 
    }
}

//SOPORTE GDC 
#else

// Función auxiliar para crear un paquete Remote Serial Protocol
string make_gdb_packet(const string& data) {
    uint8_t csum = 0;
    for (char c : data) csum += c;
    stringstream ss;
    ss << "$" << data << "#" << hex << setfill('0') << setw(2) << (int)csum;
    return ss.str();
}

// Función auxiliar para leer todos los registros para GDB
string get_gdb_registers() {
    stringstream ss;
    for(int i = 0; i < 32; i++) {
        uint32_t v = sim.regs[i];
        ss << hex << setfill('0') << setw(2) << (v & 0xff)
           << setw(2) << ((v>>8)&0xff) << setw(2) << ((v>>16)&0xff) << setw(2) << ((v>>24)&0xff);
    }
    uint32_t v = sim.pc;
    ss << hex << setfill('0') << setw(2) << (v & 0xff)
       << setw(2) << ((v>>8)&0xff) << setw(2) << ((v>>16)&0xff) << setw(2) << ((v>>24)&0xff);
    return ss.str();
}

int main(int argc, char* argv[]) {
    cout << "=== RISC-V Native Simulator / GDB Server ===" << endl;

    // 1. Inicializar Sockets (Compatible con Windows y Linux)
    #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    #endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1234);

    // 2. Levantar servidor en el puerto 1234
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);
    cout << "Esperando conexion GDB en localhost:1234..." << endl;

    socklen_t addrlen = sizeof(address);
    SOCKET client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    cout << "GDB Conectado. Iniciando depuracion." << endl;

    // 3. Bucle infinito recibiendo comandos RSP
    char buffer[1024];
    while (true) {
        int valread = recv(client_socket, buffer, 1024, 0);
        if (valread <= 0) break;
        
        // Responder con ACK (Todo paquete recibido se acepta con un +)
        send(client_socket, "+", 1, 0);

        string cmd(buffer, valread);
        
        // Interpretar comandos que vienen dentro de $...#
        if (cmd.find("$g#") != string::npos) {
            // 'g': Leer todos los registros
            string response = make_gdb_packet(get_gdb_registers());
            send(client_socket, response.c_str(), response.length(), 0);
        }
        else if (cmd.find("$c#") != string::npos) {
            // 'c': Continuar ejecución
            while (!sim.halted) sim.step();
            string response = make_gdb_packet("S05"); // Enviar Señal Trap
            send(client_socket, response.c_str(), response.length(), 0);
        }
        else if (cmd.find("$s#") != string::npos) {
            // 's': Step (Un solo paso)
            sim.step();
            string response = make_gdb_packet("S05");
            send(client_socket, response.c_str(), response.length(), 0);
        }
        else if (cmd.find("$?#") != string::npos) {
            // '?': Razón de parada
            string response = make_gdb_packet("S05");
            send(client_socket, response.c_str(), response.length(), 0);
        }
        else if (cmd[0] == '$') {
            // Comando no soportado, devolver paquete vacío para que GDB sepa
            string response = make_gdb_packet("");
            send(client_socket, response.c_str(), response.length(), 0);
        }
    }

    cout << "Conexion cerrada." << endl;
    closesocket(client_socket);
    closesocket(server_fd);
    
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
#endif