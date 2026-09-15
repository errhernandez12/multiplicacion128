#include <stdio.h>       // Incluye funciones estándar de entrada y salida (printf)
#include <immintrin.h>    // Incluye los intrínsecos de Intel (_mulx_u64, _mm_set_epi64x)
#include <stdint.h>       // Define tipos de enteros de tamaño fijo (uint64_t, uint8_t)

// Unión para representar un operando de 128 bits
typedef union {
    __m128i Num;            // Registro SIMD de 128 bits de Intel (SSE2)
    unsigned long long u64[2]; // 2 enteros de 64 bits: u64[0] = parte baja, u64[1] = parte alta
    unsigned char bytes[16];   // 16 bytes continuos en memoria para leer byte a byte
} UInt128;                  // Nombre del tipo de dato estructurado

// Unión para representar el resultado de la multiplicación de 256 bits (32 bytes)
typedef union {
    __m128i Num[2];            // 2 registros SIMD de 128 bits: Num[0] = bits 0-127, Num[1] = bits 128-255
    unsigned long long u64[4]; // 4 enteros de 64 bits: W0, W1, W2 y W3
    unsigned char bytes[32];   // 32 bytes continuos para representar el número completo
} UInt256;                  // Nombre del tipo de dato estructurado

// Función auxiliar para imprimir un bloque de bytes en Hexadecimal de MSB a LSB
void imprimir_bytes(const unsigned char *b, int n) {
    for (int i = n - 1; i >= 0; i--) { // Bucle descendente: inicia en el último byte (MSB) y baja a 0 (LSB)
        printf("%02X ", b[i]);          // Imprime el valor de cada byte en formato hexadecimal de 2 dígitos
    }
    printf("\n");                       // Imprime un salto de línea al terminar
}

// Función auxiliar para imprimir un bloque de bytes en Binario (bits) de MSB a LSB
void imprimir_bits(const unsigned char *b, int n) {
    for (int i = n - 1; i >= 0; i--) {     // Bucle descendente para recorrer del byte más alto al más bajo
        for (int bit = 7; bit >= 0; bit--) { // Recorre cada uno de los 8 bits del byte actual
            printf("%d", (b[i] >> bit) & 1); // Desplaza el bit actual a la posición 0 y aplica una máscara AND
        }
        if (i % 8 == 0) printf(" ");         // Inserta un espacio de separación visual cada 64 bits (8 bytes)
    }
    printf("\n");                           // Imprime un salto de línea al terminar
}

// Función principal de multiplicación 128x128 -> 256 bits usando intrínsecos de Intel
UInt256 multiplicar_intel_bmi2(UInt128 A, UInt128 B) {
    // Declaración de variables para almacenar los resultados parciales de 64x64 bits
    unsigned long long L0, H0, L1, H1, L2, H2, L3, H3;

    // Multiplicación 1 (A_baja * B_baja): _mulx_u64 guarda los 64 bits bajos en L0 y los altos en H0
    L0 = _mulx_u64(A.u64[0], B.u64[0], &H0);

    // Multiplicación 2 (A_baja * B_alta): _mulx_u64 guarda los 64 bits bajos en L1 y los altos en H1
    L1 = _mulx_u64(A.u64[0], B.u64[1], &H1);

    // Multiplicación 3 (A_alta * B_baja): _mulx_u64 guarda los 64 bits bajos en L2 y los altos en H2
    L2 = _mulx_u64(A.u64[1], B.u64[0], &H2);

    // Multiplicación 4 (A_alta * B_alta): _mulx_u64 guarda los 64 bits bajos en L3 y los altos en H3
    L3 = _mulx_u64(A.u64[1], B.u64[1], &H3);

    // Suma de productos cruzados para obtener los bits 64-127 (Palabra W1)
    unsigned __int128 sum1 = (unsigned __int128)H0 + L1 + L2; // Suma H0 con los productos cruzados L1 y L2
    unsigned long long W0 = L0;                             // W0 son los 64 bits más bajos directos de L0
    unsigned long long W1 = (unsigned long long)sum1;       // W1 obtiene los 64 bits bajos de sum1

    // Suma de productos cruzados y acarreos para los bits 128-191 (Palabra W2)
    unsigned __int128 sum2 = (unsigned __int128)H1 + H2 + L3 + (sum1 >> 64); // Incluye el acarreo de sum1
    unsigned long long W2 = (unsigned long long)sum2;       // W2 obtiene los 64 bits bajos de sum2

    // Palabra W3 contiene la parte más alta (bits 192-255) sumando H3 y el acarreo restante de sum2
    unsigned long long W3 = H3 + (unsigned long long)(sum2 >> 64);

    // Creación de la estructura de resultado tipo UInt256
    UInt256 res;

    // Carga las palabras W0 y W1 en el primer registro __m128i (Num[0]) usando el intrínseco _mm_set_epi64x
    res.Num[0] = _mm_set_epi64x((long long)W1, (long long)W0); // Recibe el valor alto primero y el bajo después

    // Carga las palabras W2 y W3 en el segundo registro __m128i (Num[1])
    res.Num[1] = _mm_set_epi64x((long long)W3, (long long)W2); // Recibe el valor alto primero y el bajo después

    return res; // Devuelve la unión poblada con los 256 bits calculados
}

int main() {
    // Declaración de las variables de entrada del tipo UInt128
    UInt128 A, B;

    // Inicialización del operando A usando sus campos de 64 bits dentro de la unión
    A.u64[0] = 0xFFFFFFFFFFFFFFFFULL; // Parte baja de A (Bits 0-63)
    A.u64[1] = 0x0000000000000001ULL; // Parte alta de A (Bits 64-127)

    // Inicialización del operando B
    B.u64[0] = 0x0000000000000002ULL; // Parte baja de B (Bits 0-63)
    B.u64[1] = 0x0000000000000000ULL; // Parte alta de B (Bits 64-127)

    // Visualización del Operando A
    printf("================ OPERANDO A (128 bits) ================\n");
    printf("Bytes (MSB -> LSB): "); imprimir_bytes(A.bytes, 16); // Muestra A en hexadecimal por byte
    printf("Bits  (MSB -> LSB): "); imprimir_bits(A.bytes, 16);  // Muestra A representado en bits

    // Visualización del Operando B
    printf("\n================ OPERANDO B (128 bits) ================\n");
    printf("Bytes (MSB -> LSB): "); imprimir_bytes(B.bytes, 16); // Muestra B en hexadecimal por byte
    printf("Bits  (MSB -> LSB): "); imprimir_bits(B.bytes, 16);  // Muestra B representado en bits

    // Llamada a la función de multiplicación
    UInt256 R = multiplicar_intel_bmi2(A, B);

    // Visualización del Resultado de 256 bits
    printf("\n================ RESULTADO (256 bits) ================\n");
    printf("Bytes (MSB -> LSB):\n"); imprimir_bytes(R.bytes, 32); // Muestra los 32 bytes del resultado
    printf("\nBits  (MSB -> LSB):\n"); imprimir_bits(R.bytes, 32); // Muestra los 256 bits del resultado

    // Muestra de las direcciones de memoria para comprobar el solapamiento de la unión
    printf("\n================ APUNTADORES Y DIRECCIONES ================\n");
    printf("Dirección base &R              : %p\n", (void*)&R);            // Dirección de inicio del resultado R
    printf("Dirección &R.Num[0] (__m128i)  : %p\n", (void*)&R.Num[0]);     // Dirección del registro SIMD bajo
    printf("Dirección &R.Num[1] (__m128i)  : %p\n", (void*)&R.Num[1]);     // Dirección del registro SIMD alto (+16 bytes)
    printf("Dirección &R.u64[0] (Bits 0-63): %p\n", (void*)&R.u64[0]);     // Dirección de la palabra u64 de menor peso
    printf("Dirección &R.bytes[0]          : %p\n", (void*)&R.bytes[0]);  // Dirección del primer byte en memoria

    return 0; // Finalización correcta del programa
}
