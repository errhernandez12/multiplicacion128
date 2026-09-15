#include <stdio.h>       // Incluye funciones estándar de entrada y salida (printf)
#include <emmintrin.h>   // Incluye los intrínsecos de SSE2 (_mm_set_epi64x)
#include <stdint.h>      // Define tipos de enteros de tamaño fijo (uint64_t, uint8_t)

// Unión para representar un operando de 128 bits
typedef union {
    __m128i Num;               // Registro SIMD de 128 bits de Intel (SSE2)
    unsigned long long u64[2]; // 2 enteros de 64 bits: u64[0] = parte baja, u64[1] = parte alta
    unsigned char bytes[16];   // 16 bytes continuos en memoria para leer byte a byte
} UInt128;

// Unión para representar el resultado de la multiplicación de 256 bits (32 bytes)
typedef union {
    __m128i Num[2];            // 2 registros SIMD de 128 bits: Num[0] = bits 0-127, Num[1] = bits 128-255
    unsigned long long u64[4]; // 4 enteros de 64 bits: W0, W1, W2 y W3
    unsigned char bytes[32];   // 32 bytes continuos para representar el número completo
} UInt256;

// Función auxiliar para emular _mulx_u64 de manera portable (sin requerir la extensión BMI2 por hardware)
static inline unsigned long long mulx_portable(unsigned long long a, unsigned long long b, unsigned long long *hi) {
    unsigned __int128 prod = (unsigned __int128)a * b;
    *hi = (unsigned long long)(prod >> 64);
    return (unsigned long long)prod;
}

// Función auxiliar para imprimir un bloque de bytes en Hexadecimal de MSB a LSB
void imprimir_bytes(const unsigned char *b, int n) {
    for (int i = n - 1; i >= 0; i--) {
        printf("%02X ", b[i]);
    }
    printf("\n");
}

// Función auxiliar para imprimir un bloque de bytes en Binario (bits) de MSB a LSB
void imprimir_bits(const unsigned char *b, int n) {
    for (int i = n - 1; i >= 0; i--) {
        for (int bit = 7; bit >= 0; bit--) {
            printf("%d", (b[i] >> bit) & 1);
        }
        if (i % 8 == 0) printf(" ");
    }
    printf("\n");
}

// Función principal de multiplicación 128x128 -> 256 bits
UInt256 multiplicar_128x128(UInt128 A, UInt128 B) {
    unsigned long long L0, H0, L1, H1, L2, H2, L3, H3;

    // Multiplicaciones de 64x64 bits mediante la función auxiliar portable
    L0 = mulx_portable(A.u64[0], B.u64[0], &H0);
    L1 = mulx_portable(A.u64[0], B.u64[1], &H1);
    L2 = mulx_portable(A.u64[1], B.u64[0], &H2);
    L3 = mulx_portable(A.u64[1], B.u64[1], &H3);

    // Suma de productos cruzados para obtener los bits 64-127 (Palabra W1)
    unsigned __int128 sum1 = (unsigned __int128)H0 + L1 + L2;
    unsigned long long W0 = L0;
    unsigned long long W1 = (unsigned long long)sum1;

    // Suma de productos cruzados y acarreos para los bits 128-191 (Palabra W2)
    unsigned __int128 sum2 = (unsigned __int128)H1 + H2 + L3 + (sum1 >> 64);
    unsigned long long W2 = (unsigned long long)sum2;

    // Palabra W3 contiene los bits 192-255
    unsigned long long W3 = H3 + (unsigned long long)(sum2 >> 64);

    // Creación de la estructura de resultado tipo UInt256
    UInt256 res;
    res.Num[0] = _mm_set_epi64x((long long)W1, (long long)W0);
    res.Num[1] = _mm_set_epi64x((long long)W3, (long long)W2);

    return res;
}

int main() {
    UInt128 A, B;

    // Inicialización del operando A
    A.u64[0] = 0xFFFFFFFFFFFFFFFFULL; // Parte baja (Bits 0-63)
    A.u64[1] = 0x0000000000000001ULL; // Parte alta (Bits 64-127)

    // Inicialización del operando B
    B.u64[0] = 0x0000000000000002ULL; // Parte baja (Bits 0-63)
    B.u64[1] = 0x0000000000000000ULL; // Parte alta (Bits 64-127)

    printf("================ OPERANDO A (128 bits) ================\n");
    printf("Bytes (MSB -> LSB): "); imprimir_bytes(A.bytes, 16);
    printf("Bits  (MSB -> LSB): "); imprimir_bits(A.bytes, 16);

    printf("\n================ OPERANDO B (128 bits) ================\n");
    printf("Bytes (MSB -> LSB): "); imprimir_bytes(B.bytes, 16);
    printf("Bits  (MSB -> LSB): "); imprimir_bits(B.bytes, 16);

    // Llamada a la multiplicación portable
    UInt256 R = multiplicar_128x128(A, B);

    printf("\n================ RESULTADO (256 bits) ================\n");
    printf("Bytes (MSB -> LSB):\n"); imprimir_bytes(R.bytes, 32);
    printf("\nBits  (MSB -> LSB):\n"); imprimir_bits(R.bytes, 32);

    printf("\n================ APUNTADORES Y DIRECCIONES ================\n");
    printf("Dirección base &R               : %p\n", (void*)&R);
    printf("Dirección &R.Num[0] (__m128i)  : %p\n", (void*)&R.Num[0]);
    printf("Dirección &R.Num[1] (__m128i)  : %p\n", (void*)&R.Num[1]);
    printf("Dirección &R.u64[0] (Bits 0-63): %p\n", (void*)&R.u64[0]);
    printf("Dirección &R.bytes[0]          : %p\n", (void*)&R.bytes[0]);

    return 0;
}
