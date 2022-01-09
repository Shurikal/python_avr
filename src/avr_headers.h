#ifndef AVR_HEADERS
#define AVR_HEADERS

#include <stdint.h>
#include <string.h>

#define REGISTER_SIZE (32)
#define IO_REGISTER_SIZE (64)
#define SRAM_SIZE (1024)
#define PROGRAM_MEMORY_SIZE (1024)

typedef struct {
    PyObject_HEAD
    uint8_t     sreg;
    uint8_t     registers[REGISTER_SIZE];
    uint8_t     io_registers[IO_REGISTER_SIZE];
    uint8_t     sram[SRAM_SIZE];
    uint16_t    program_memory[PROGRAM_MEMORY_SIZE];
    uint16_t    program_counter;
    uint8_t     break_point_reached;
    PyObject    *x_attr;        /* Attributes dictionary */
} AVRoObject;

#endif