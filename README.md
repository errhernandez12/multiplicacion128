# Multiplicación de Precisión Extendida (128-bit × 128-bit → 256-bit) en C

Este repositorio contiene la implementación de algoritmos en C para realizar la multiplicación de enteros no asignados de 128 bits con un resultado de 256 bits en arquitecturas x86-64. Se hace uso de uniones C (`union`), registros SIMD de 128 bits (`__m128i` de SSE2) y el empaquetado Little-Endian.

## Estructura del Proyecto

* **`multiplicacion128.c`**: Implementación optimizada por hardware que utiliza la extensión **BMI2** de Intel/AMD mediante el intrínseco `_mulx_u64` (instrucción `MULX`), la cual ejecuta la multiplicación de 64x64 bits sin afectar las banderas de estado (`FLAGS`).
* **`multiplicacion128_sinBMI2.c`**: Versión **portable** para cualquier procesador x86-64. Emula `_mulx_u64` mediante la extensión de compilador `unsigned __int128`, evitando errores de instrucción ilegal (`SIGILL`) en procesadores antiguos o entornos virtuales reducidos.

## Requisitos y Compilación

Requiere un compilador C compatible con x86-64 (GCC, Clang o Intel ICX).

### 1. Compilar versión con soporte BMI2 (Hardware)
```bash
gcc -O3 -mbmi2 multiplicacion128.c -o multiplicacion128
