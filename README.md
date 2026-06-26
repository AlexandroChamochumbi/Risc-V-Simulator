# ⚙️ Simulador de Procesador RISC-V (RV32I Core Engine)

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![WebAssembly](https://img.shields.io/badge/WebAssembly-WASM-orange.svg)
![GDB](https://img.shields.io/badge/GDB-RSP%20Server-purple.svg)
![JavaScript](https://img.shields.io/badge/JavaScript-ES6-yellow.svg)
![Python](https://img.shields.io/badge/Python-3.x-green.svg)
## 📁 Enlace a Google Drive

🔗 https://drive.google.com/drive/folders/1-9toG2bLuI8nF9bDxZ_3t30552olbOe7?usp=sharing
## 📁 Enlace a Repositorio Github

🔗 https://github.com/AlexandroChamochumbi/Risc-V-Simulator

## 📖 Descripción General

Este proyecto consiste en el diseño y la implementación de un simulador funcional para la arquitectura de instrucciones **RISC-V (específicamente el conjunto ISA RV32I de 32 bits)**. 

El simulador cuenta con una arquitectura híbrida moderna y versátil:
1. **Entorno Web Interactivo:** El motor de ejecución de bajo nivel desarrollado en C++ se compila hacia **WebAssembly (WASM)** mediante Emscripten, permitiendo ejecutar el procesador en un navegador web con una interfaz gráfica (GUI) fluida.
2. **Entorno de Depuración Nativo (GDB Server):** El mismo motor puede compilarse de forma nativa para ejecutarse en consola, levantando un servidor TCP que implementa el protocolo oficial **GDB RSP**, permitiendo que depuradores industriales reales se conecten al simulador a distancia.

---

## 🛠️ Tecnologías Utilizadas

* **C++17 (Core Engine):** Modelado de la unidad lógica, los 32 registros, el contador de programa (PC), los ciclos de reloj y la memoria RAM.
* **GDB RSP (Remote Serial Protocol):** Implementación de un *GDB Stub* nativo mediante Sockets TCP para interactuar con herramientas de depuración estándar.
* **WebAssembly & Emscripten:** Puente tecnológico para compilar el código de C++ a un módulo binario ejecutable en la web.
* **HTML5, CSS3 y JavaScript:** Interfaz gráfica interactiva con diseño de alto contraste, tipografía optimizada (`JetBrains Mono`) y manipulación del DOM en tiempo real.
* **Python 3:** Utilizado para el despliegue del servidor web local.

---

## ✨ Características Principales (Features)

* **Soporte Avanzado para GDB (Remote Debugging):**
  * Integra un servidor GDB nativo que escucha en el puerto `1234`.
  * Soporta comandos estándar de la industria como lectura de registros (`info registers`), ejecución paso a paso (`stepi`) y continuación de flujo (`continue`).
* **Carga Inteligente Dual de Archivos:**
  * **Soporte Binario Crudo (`.bin`):** Lectura de código máquina mapeado desde `0x00000000`.
  * **Soporte Ejecutable ELF (`.elf`):** Integra un *parser* de cabeceras ELF en C++. Detecta la firma `0x7F 'E' 'L' 'F'`, extrae la sección de código y redirige el PC al punto de entrada real.
* **Control de Ejecución Flexible:**
  * Modo **Step-by-Step** para depuración manual.
  * Modo **Run** continuo con selector de velocidad (Slow, Normal, Fast).
  * Función **Reset** por hardware (limpia registros, reloj y RAM).
* **Monitoreo de Estado Gráfico Dinámico (Modo Web):**
  * Panel de desensamblado estático (dirección, hex, nemónico).
  * Grilla interactiva de los 32 registros (`x0`-`x31` y alias) con animaciones visuales instantáneas.
  * Visualizador completo de memoria RAM (Hexadecimal y ASCII) con salto rápido.
* **Llamadas al Sistema (Syscalls):**
  * Soporte de la instrucción `ecall`. El motor C++ intercepta la llamada para imprimir resultados en la consola o detener la ejecución (con soporte para emulación de entornos tipo SPIM).

---

## 📂 Estructura del Repositorio

* `motor_riscv.cpp`: Código fuente central del procesador, memoria y servidor GDB en C++.
* `simulador_gdb.exe`: Ejecutable nativo compilado para el modo Servidor GDB.
* `index.html`: Interfaz de usuario y Wrapper de control en JavaScript para el entorno Web.
* `motor.js` / `motor.wasm`: Archivos binarios compilados por Emscripten.
* `prueba.elf` / `prueba.bin`: Archivos de código RISC-V utilizados para pruebas.

---

## 🚀 Compilación y Ejecución Local

### Opción A: Despliegue en Entorno Web (WASM)

1. Requiere tener [Emscripten](https://emscripten.org/) instalado y las variables de entorno activadas mediante .\emsdk_env.ps1 (Windows). Compila el motor ejecutando en la terminal:
   ```bash
   emcc motor_riscv.cpp -o motor.js -s EXPORTED_FUNCTIONS="['_malloc', '_free', '_sim_reset', '_sim_load_bin', '_sim_load_elf', '_sim_step', '_sim_get_pc', '_sim_get_cycle', '_sim_get_reg', '_sim_read_mem', '_sim_read_instr']" -s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'HEAPU8']"
   ```
2. Levanta el servidor web local con Python:
   ```bash
   python -m http.server 8000
   ```
3. Abre tu navegador en `http://localhost:8000`.

### Opción B: Uso del Servidor de Depuración Remota (GDB Server)

Para comprobar el correcto funcionamiento del protocolo de red y la integración con el depurador oficial de RISC-V, sigue estos pasos:

1. **Iniciar el Simulador Nativo:**
   Abre una terminal y ejecuta el simulador en modo servidor. Se quedará en estado de escucha activa:
   ```powershell
   .\simulador_gdb.exe
   ```
   *Salida esperada: `Esperando conexion GDB en localhost:1234...`*

2. **Iniciar el Depurador de GNU:**
   En una **segunda terminal**, abre el depurador oficial de la arquitectura (por ejemplo, la suite de *xPack GNU RISC-V Embedded GCC*):
   ```powershell
   .\riscv-none-elf-gdb.exe
   ```

3. **Conectar e Interactuar a través de GDB:**
   Dentro del prompt de GDB `(gdb)`, ejecuta los siguientes comandos en orden:
   ```gdb
   # 1. Configurar la arquitectura para un núcleo de 32 bits
   (gdb) set architecture riscv:rv32

   # 2. Conectarse al puerto expuesto por nuestro simulador C++
   (gdb) target remote localhost:1234

   # 3. Solicitar el estado actual de los registros simulados
   (gdb) info registers
   ```
   *Al realizar la conexión, la pantalla del simulador cambiará automáticamente a `GDB Conectado. Iniciando depuracion.`, confirmando el correcto intercambio de paquetes bajo el estándar RSP.*