#ifdef _MSC_VER
    // Shut compiler up about strcpy() fopen() not being safe
    #define _CRT_SECURE_NO_WARNINGS 1
#endif // _MSC_VER

// Includes
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h> // uint8_t
    #include <string.h> // memset
    #include <math.h>   // cos sin

// Prototypes
    uint8_t *MemGetMainPtr( uint16_t address );
    uint8_t *MemGetAuxPtr ( uint16_t address );
