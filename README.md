# ⚙️ Simulador de Procesador RISC-V (RV32I Core Engine)

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![WebAssembly](https://img.shields.io/badge/WebAssembly-WASM-orange.svg)
![JavaScript](https://img.shields.io/badge/JavaScript-ES6-yellow.svg)
![Python](https://img.shields.io/badge/Python-3.x-green.svg)


## 📖 Descripción General
Este proyecto consiste en el diseño y la implementación de un simulador funcional para la arquitectura de instrucciones **RISC-V (específicamente el conjunto ISA RV32I de 32 bits)**. 

El simulador cuenta con una arquitectura híbrida moderna: un **motor de ejecución de bajo nivel desarrollado en C++ nativo**, el cual es compilado hacia **WebAssembly (WASM)** mediante Emscripten. Esto permite que el procesador se ejecute a velocidades cercanas al hardware nativo directamente en un entorno web aislado (*sandbox*), ofreciendo una interfaz gráfica de usuario (GUI) interactiva, fluida y accesible desde cualquier navegador.

---

## 🛠️ Tecnologías Utilizadas
* **C++17 (Core Engine):** Modelado de la unidad lógica, los 32 registros, el contador de programa (PC), los ciclos de reloj y la memoria RAM.
* **WebAssembly & Emscripten:** Puente tecnológico para compilar el código de C++ a un módulo binario de alta velocidad ejecutable en la web.
* **HTML5, CSS3 y JavaScript:** Interfaz gráfica interactiva con diseño de alto contraste, tipografía optimizada (`JetBrains Mono`) y manipulación del DOM en tiempo real.
* **Python 3:** Utilizado para el despliegue del servidor web local.

---

## ✨ Características Principales (Features)

* **Carga Inteligente Dual de Archivos:**
  * **Soporte Binario Crudo (`.bin`):** Lectura de código máquina mapeado desde `0x00000000`.
  * **Soporte Ejecutable ELF (`.elf`):** Integra un *parser* de cabeceras ELF en C++. Detecta la firma `0x7F 'E' 'L' 'F'`, extrae la sección de código y redirige el PC al punto de entrada real.
* **Control de Ejecución Flexible:** * Modo **Step-by-Step** para depuración.
  * Modo **Run** continuo con selector de velocidad (Slow, Normal, Fast).
  * Función **Reset** por hardware (limpia registros, reloj y RAM).
* **Monitoreo de Estado Gráfico Dinámico:**
  * Panel de desensamblado estático (dirección, hex, nemónico).
  * Grilla interactiva de los 32 registros (`x0`-`x31` y alias) con animaciones visuales instantáneas ante cambios de estado.
  * Visualizador completo de memoria RAM (Hexadecimal y ASCII) con salto rápido.
* **Llamadas al Sistema (Syscalls):**
  * Soporte de la instrucción `ecall`. El motor C++ intercepta la llamada e interactúa con la consola JavaScript para imprimir resultados o detener la ejecución.

---

## 📂 Estructura del Repositorio
* `motor_riscv.cpp`: Código fuente central del procesador y memoria en C++.
* `index.html`: Interfaz de usuario y Wrapper de control en JavaScript.
* `motor.js` / `motor.wasm`: Archivos binarios compilados por Emscripten.
* `prueba.elf` / `prueba.bin`: Archivos resultantes de prueba.

---

## 🚀 Compilación y Ejecución Local

### 1. Compilar el motor (C++ a WASM)
Requiere tener [Emscripten](https://emscripten.org/) instalado. Ejecutar en terminal:
```bash
emcc motor_riscv.cpp -o motor.js -s EXPORTED_FUNCTIONS="['_malloc', '_free', '_sim_reset', '_sim_load_bin', '_sim_load_elf', '_sim_step', '_sim_get_pc', '_sim_get_cycle', '_sim_get_reg', '_sim_read_mem', '_sim_read_instr']" -s EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap', 'HEAPU8']"

python -m http.server 8000
