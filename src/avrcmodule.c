#include "Python.h"
#include "avr_headers.h"

#include <stdint.h>
/* AVRo objects */
static PyTypeObject AVRo_Type;

#define AVRoObject_Check(v)      Py_IS_TYPE(v, &AVRo_Type)

#define get_bit(n,k) ((n & ( 1 << k )) >> k)

#define g_rd3   (get_bit(rd,3))
#define g_rr3   (get_bit(rr,3))
#define g_r3    (get_bit(result,3))

#define g_rd7   (get_bit(rd,7))
#define g_rr7   (get_bit(rr,7))
#define g_r7    (get_bit(result,7))

#define x_register ((self->registers[27] << 8) + self->registers[26])
#define y_register ((self->registers[29] << 8) + self->registers[28])
#define z_register ((self->registers[31] << 8) + self->registers[30])

#define instr_check(instruction, mask, operation) ((instruction & mask) == operation)

#define DEBUG

#define NOT_IMPLEMENTED (0)
#define GENERALIZATION_IMPLEMENTED (0)

static AVRoObject *
newAVRoObject(PyObject *arg)
{
    AVRoObject *self;
    self = PyObject_New(AVRoObject, &AVRo_Type);
    if (self == NULL)
        return NULL;
    self->x_attr = NULL;

    // set SREG to 0
    self->sreg = 0;

    self->break_point_reached = 0;

    // set all registers to 0
    memset(&self->registers, 0, REGISTER_SIZE);

    // set io_registers to 0
    memset(&self->io_registers, 0, IO_REGISTER_SIZE);

    // set sram to 0
    memset(&self->sram, 0, SRAM_SIZE);

    // set the program memory to zero
    memset(&self->program_memory, 0, 2*PROGRAM_MEMORY_SIZE);

    // set program_counter to zero
    self->program_counter = 0;
    return self;
}

/* AVRo methods */
static void
AVRo_dealloc(AVRoObject *self)
{
    Py_XDECREF(self->x_attr);
    PyObject_Free(self);
}

static PyObject *
AVRo_get_sreg(AVRoObject *self, PyObject *args)
{
    return Py_BuildValue("H", self->sreg);
}

static PyObject *
AVRo_set_sreg(AVRoObject *self, PyObject *args)
{
    uint8_t new_sreg;
    if (!PyArg_ParseTuple(args, "k", &new_sreg)){
        //todo
        Py_RETURN_NONE;
    }
    if (new_sreg>255 || new_sreg < 0){
        Py_RETURN_NONE;
    }

    self->sreg = (uint8_t) new_sreg;

    return Py_BuildValue("H",self->sreg);
}

static PyObject *
AVRo_get_register(AVRoObject *self, PyObject *args)
{
    int64_t index;
    if (!PyArg_ParseTuple(args, "k", &index) )//|| index < 0 || index >= REGISTER_SIZE)
        //todo
         Py_RETURN_NONE;
    return Py_BuildValue("k",self->registers[index]);
}

static PyObject *
AVRo_set_register(AVRoObject *self, PyObject *args, PyObject *keywds){
    uint8_t new_value;
    uint64_t index;

    static char *kwlist[] = {"index", "new_value", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "kk", kwlist, &index, &new_value) || index < 0 || index >= REGISTER_SIZE || new_value>255 || new_value < 0) {
        //todo
         Py_RETURN_NONE;
    }

    self->registers[index] = (uint8_t) new_value;
    return Py_BuildValue("k", self->registers[index]);

}

static PyObject *
AVRo_get_program_counter(AVRoObject *self, PyObject *args)
{
    return Py_BuildValue("k", self->program_counter);
}

static PyObject *
AVRo_set_program_memory(AVRoObject *self, PyObject *args, PyObject *keywds){
    uint64_t index = 0;
    uint16_t instruction = 0;

    static char *kwlist[] = {"instruction", "index", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "kk", kwlist, &instruction, &index) || index < 0 || index >= PROGRAM_MEMORY_SIZE || instruction>65535 || instruction < 0)
        //todo
         Py_RETURN_NONE;
    //printf("SREG as int %s\n", program);
    self->program_memory[index] = instruction;
    return Py_BuildValue("k",self->program_memory[index]);
}

static PyObject *
AVRo_get_program_memory(AVRoObject *self, PyObject *args)
{
    int64_t index;
    if (!PyArg_ParseTuple(args, "k", &index) || index < 0 || index >= PROGRAM_MEMORY_SIZE)
        //todo
         Py_RETURN_NONE;
    return Py_BuildValue("k",self->program_memory[index]);
}

static PyObject *
AVRo_get_program_memory_size(AVRoObject *self, PyObject *args)
{
    return Py_BuildValue("k",PROGRAM_MEMORY_SIZE);
}

static PyObject *
AVRo_get_sram_size(AVRoObject *self, PyObject *args)
{
    return Py_BuildValue("k",SRAM_SIZE);
}


static int run_instruction(AVRoObject *self){
    // Load instruction from program memory using the program counter
    uint16_t instruction = self->program_memory[self->program_counter];

    if (instruction == 0){
        // NOP
        #ifdef DEBUG
        printf("NOP\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0001110000000000)){
        // ADC
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd + rr + get_bit(self->sreg,0);

        // Update SREG
        uint8_t h = ( g_rd3 & g_rr3 ) | ( g_rr3 & (!g_r3) ) | ( (!g_r3) & g_rd3 );
        uint8_t v = ( g_rd7 & g_rr7 & (!g_r7) ) | ((!g_rd7) & (!g_rr7) & g_r7);
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t c = ( g_rd7 & g_rr7) | (g_rr7 & (!g_r7)) | ( (!g_r7) & g_rd7 );
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000) | (h << 5) | (s << 4)  | (v << 3)  | (n << 2)  | (z << 1) | c;
        // Move result to storage
        self->registers[(instruction & 0b0000000111110000) >> 4] = result;

        #ifdef DEBUG
        printf("ADC\n");
        //printf("Rd i: %i\n", (instruction & 0b0000000111110000) >> 4);
        //printf("Rr i: %i\n", (instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5));
        //printf("Rd: %i\n",rd);
        //printf("Rr: %i\n",rr);
        //printf("Result: %i\n",result);
        #endif

    }else if(instr_check(instruction, 0b1111110000000000, 0b0000110000000000)){
        // ADD
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd + rr;

        // Update SREG
        uint8_t h = ( g_rd3 & g_rr3 ) | ( g_rr3 & (!g_r3) ) | ( (!g_r3) & g_rd3 );
        uint8_t v = ( g_rd7 & g_rr7 & (!g_r7) ) | ((!g_rd7) & (!g_rr7) & g_r7);
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t c = ( g_rd7 & g_rr7) | (g_rr7 & (!g_r7)) | ( (!g_r7) & g_rd7 );
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000) | (h << 5) | (s << 4)  | (v << 3)  | (n << 2)  | (z << 1) | c;


        // Move result to storage
        self->registers[(instruction & 0b0000000111110000) >> 4] = result;

        #ifdef DEBUG
        printf("ADD\n");
        #endif

    }else if(NOT_IMPLEMENTED){
        // ADIW
    }else if(instr_check(instruction, 0b1111110000000000, 0b0010000000000000)){
        // AND
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd & rr;

        // Update SREG

        uint8_t v = 0;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11100001)  | (s << 4)  | (v << 3)  | (n << 2)  | (z << 1);


        self->registers[(instruction & 0b0000000111110000) >> 4] = result;

        #ifdef DEBUG
        printf("AND\n");
        #endif

    }else if(instr_check(instruction, 0b1111000000000000, 0b0111000000000000)){
        // ANDI
        uint8_t d = 16 + ((instruction & 0b0000000011110000) >> 4);
        uint8_t rd = self->registers[d] ;
        uint8_t k = (instruction & 0b0000000000001111) + ((instruction & 0b0000111100000000)>>4);
        uint8_t result = rd & k;

        // Update SREG
        uint8_t v = 0;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11100001)  | (s << 4)  | (v << 3)  | (n << 2)  | (z << 1);

        self->registers[d] = result;

        #ifdef DEBUG
        printf("ANDI\n");
        #endif

    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000101)){
        // ASR
        // Signed for arithmetic shift
        uint8_t rd = self->registers[16 + ((instruction & 0b0000000111110000) >> 4)] ;
        uint8_t result = rd >> 1;

        // Update SREG
        uint8_t c = get_bit(rd,0);
        uint8_t z = result == 0;
        uint8_t n = g_r7;
        uint8_t v = n ^ c;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 011100000) | (s << 4)  | (v << 3)  | (n << 2)  | (z << 1) | c;


        self->registers[16 + ((instruction & 0b0000000111110000) >> 4)] = result;

        #ifdef DEBUG
        printf("ASR\n");
        #endif

    }else if(instr_check(instruction, 0b1111111110001111, 0b1001010010001000)){
        // BCLR
        uint8_t position = (instruction & 0b0000000001110000) >> 4;
        // todo
        uint8_t test = 0;

        memcpy(&test,&self->sreg,1);

        printf("Sreg = %i\n",test);
        self->sreg = ((uint8_t)self->sreg) & ~(1<<position);

        #ifdef DEBUG
        printf("BCLR\n");
        #endif

    }else if(instr_check(instruction, 0b1111111000001000, 0b1111100000000000)){
        // BLD
        uint8_t position = (instruction & 0b0000000000000111);
        // todo
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4];

        self->registers[(instruction & 0b0000000111110000) >> 4] = ( (~(1 << position )) & rd) | ((get_bit(self->sreg, 6))<< position);

        #ifdef DEBUG
        printf("BLD\n");
        #endif

    }else if(instr_check(instruction, 0b1111110000000000, 0b1111010000000000)){
        // BRBC
        uint8_t position = (instruction & 0b0000000000000111);
        int8_t k = (get_bit(instruction,9)<< 7) | (get_bit(instruction,9)<< 6) | ((instruction & 0b0000000111111000)>>3);

        // todo
        if(!get_bit(self->sreg,position)){
            self->program_counter+=k;
        }

        #ifdef DEBUG
        printf("BRBC\n");
        #endif

    }else if(instr_check(instruction, 0b1111110000000000, 0b1111000000000000)){
        // BRBS
        uint8_t position = (instruction & 0b0000000000000111);
        int8_t k = (get_bit(instruction,9)<< 7) | (get_bit(instruction,9)<< 6) | ((instruction & 0b0000000111111000)>>3);

        // todo
        if(get_bit(self->sreg,position)){
            self->program_counter+=k;
        }
        #ifdef DEBUG
        printf("BRBS\n");
        #endif
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRCC
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRCS
    }else if(instr_check(instruction, 0b1111111111111111, 0b1001010110011000)){
        // BREAK
        self->break_point_reached = 1;

        #ifdef DEBUG
        printf("BREAK\n");
        #endif
    }else if(GENERALIZATION_IMPLEMENTED){
        // BREQ
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRGE
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRHC
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRHS
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRID
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRIE
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRLO
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRLT
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRMI
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRNE
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRPL
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRSH
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRTC
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRTS
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRVC
    }else if(GENERALIZATION_IMPLEMENTED){
        // BRVS
    }else if(instr_check(instruction, 0b1111111110001111, 0b1001010000001000)){
        // BSET
        uint8_t position = (instruction & 0b0000000001110000) >> 4;
        // todo
        self->sreg = (self->sreg) | (1<<position);
        #ifdef DEBUG
        printf("BSET\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001000, 0b1111101000000000)){
        // BST
        uint8_t position = (instruction & 0b0000000000000111);
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4];
        // todo
        self->sreg = (self->sreg & 0b10111111) | (get_bit(rd,position)<<6);

        #ifdef DEBUG
        printf("BST\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // CALL
    }else if(instr_check(instruction, 0b1111111100000000, 0b1001100000000000)){
        // CBI
        uint8_t position = (instruction & 0b0000000000000111);
        uint8_t sram = self->sram[(instruction & 0b0000000011111000) >> 3];
        self->sram[(instruction & 0b0000000011111000) >> 3] = sram & (~(1<<position));

        #ifdef DEBUG
        printf("CBI\n");
        #endif
    }else if(GENERALIZATION_IMPLEMENTED){
        // CBR, see ANDI
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000000)){
        // COM
        uint8_t rd = self->registers[((instruction & 0b0000000111110000) >> 4)] ;
        uint8_t result = 255-rd;

        // Update SREG
        uint8_t c = 1;
        uint8_t z = result == 0;
        uint8_t n = g_r7;
        uint8_t v = 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11101111) | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;

        self->registers[((instruction & 0b0000000111110000) >> 4)] = result;
        #ifdef DEBUG
        printf("COM\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0001010000000000)){
        // CP
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd - rr;

        // Update SREG
        uint8_t h = ( (!g_rd3) & g_rr3 ) | ( g_rr3 & g_r3 ) | ( g_r3 & (!g_rd3) );
        uint8_t v = ( g_rd7 & (!g_rr7) & (!g_r7) ) | ((!g_rd7) & g_rr7 & g_r7);
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t c = ((!g_rd7) & g_rr7) | (g_rr7 & g_r7 ) | ( g_r7 & (!g_rd7) );
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000)  | (h << 5) | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;

        #ifdef DEBUG
        printf("CP\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0000010000000000)){
        // CPC
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd - rr - get_bit(self->sreg,0);

        // Update SREG
        uint8_t h = ( (!g_rd3) & g_rr3 ) | ( g_rr3 & g_r3 ) | ( g_r3 & (!g_rd3) );
        uint8_t v = ( g_rd7 & (!g_rr7) & (!g_r7) ) | ((!g_rd7) & g_rr7 & g_r7);
        uint8_t n = g_r7;
        uint8_t old_z = get_bit(self->sreg,1);
        uint8_t z = (result == 0) & old_z;
        uint8_t c = ((!g_rd7) & g_rr7) | (g_rr7 & g_r7 ) | ( g_r7 & (!g_rd7) );
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000)  | (h << 5) | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;


        #ifdef DEBUG
        printf("CPC\n");
        #endif
    }else if(instr_check(instruction, 0b1111000000000000, 0b0011000000000000)){
        // CPI
        uint8_t rd = self->registers[16 + ((instruction & 0b0000000011110000) >> 4)] ;
        uint8_t k = ((instruction & 0b0000111100000000) >> 4) + (instruction & 0b0000000000001111);
        uint8_t result = rd - k;

        // Update SREG
        uint8_t h = ( (!g_rd3) & get_bit(k,3) ) | ( get_bit(k,3) & g_r3 ) | ( g_r3 & (!g_rd3) );
        uint8_t v = ( g_rd7 & (!g_r7) & (!get_bit(k,7)) ) | ((!g_rd7) & get_bit(k,7) & g_r7);
        uint8_t n = g_r7;
        uint8_t old_z = get_bit(self->sreg,1);
        uint8_t z = (result == 0) & old_z;
        uint8_t c = ((!g_rd7) & get_bit(k,7)) | (get_bit(k,7) & g_r7 ) | ( g_r7 & (!g_rd7) );
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000)  | (h << 5) | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;


        #ifdef DEBUG
        printf("CPI\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0001000000000000)){
        // CPSE
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd - rr;

        if(result == 0){
            //todo, 2 word instruction check, then program_counter+=2
            self->program_counter+=1;

        }

        #ifdef DEBUG
        printf("CPSE\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000001010)){
        // DEC
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t result = rd - 1;

        uint8_t v = rd == 128;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11100001) | (s <<4) | (v <<3) | (n <<2) | (z <<1);

        #ifdef DEBUG
        printf("DEC\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // DES
    }else if(NOT_IMPLEMENTED){
        // EICALL
    }else if(NOT_IMPLEMENTED){
        // EIJMP
    }else if(NOT_IMPLEMENTED){
        // ELPM
    }else if(instr_check(instruction, 0b1111110000000000, 0b0010010000000000)){
        // EOR
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t rr= self->registers[(instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5)];
        uint8_t result = rd ^ rr;

        uint8_t v = 0;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11100001) | (s <<4) | (v <<3) | (n <<2) | (z <<1);

        self->registers[(instruction & 0b0000000111110000) >> 4] = result;
        #ifdef DEBUG
        printf("EOR\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // FMUL
    }else if(NOT_IMPLEMENTED){
        // FMULS
    }else if(NOT_IMPLEMENTED){
        // FMULSU
    }else if(NOT_IMPLEMENTED){
        // ICALL
    }else if(NOT_IMPLEMENTED){
        // IJMP
    }else if(instr_check(instruction, 0b1111100000000000, 0b1011000000000000)){
        // IN
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t A  = ((instruction & 0b0000011000000000) >> 9) + (instruction & 0b0000000000001111);

        self->registers[d] = self->io_registers[A];

        #ifdef DEBUG
        printf("IN\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000011)){
        // INC
        uint8_t rd = self->registers[(instruction & 0b0000000111110000) >> 4] ;
        uint8_t result = rd + 1;

        uint8_t v = result == 127;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000)  | (s << 4) | (v << 3) | (n << 2) | (z << 1);

        self->registers[(instruction & 0b0000000111110000) >> 4] = result;
        #ifdef DEBUG
        printf("INC\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // JMP
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001001000000100)){
        // LAC
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t rd = self->registers[d];

        self->registers[d] = self->registers[z_register];
        self->registers[z_register] = (255 - rd) & self->registers[z_register]; //todo, not sure if right

        #ifdef DEBUG
        printf("LAC\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001001000000101)){
        // LAS
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t rd = self->registers[d];

        self->registers[d] = self->registers[z_register];
        self->registers[z_register] = rd | self->registers[z_register]; //todo, not sure if right
        #ifdef DEBUG
        printf("LAS\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001001000000111)){
        // LAT
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t rd = self->registers[d];

        self->registers[d] = self->registers[z_register];
        self->registers[z_register] = rd ^ self->registers[z_register]; //todo, not sure if right

        #ifdef DEBUG
        printf("LAT\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // LD
    }else if(instr_check(instruction, 0b1111000000000000, 0b1110000000000000)){
        // LDI
        uint8_t d = 16 + ((instruction & 0b0000000011110000) >> 4);
        uint8_t k = (instruction & 0b0000000000001111) + ((instruction & 0b0000111100000000)>>4);

        self->registers[d] = k;
        #ifdef DEBUG
        printf("LDI\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // LDS
    }else if(NOT_IMPLEMENTED){
        // LPM
    }else if(NOT_IMPLEMENTED){
        // LSL, same as ADD Rd, Rd
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000110)){
        // LSR
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t rd = self->registers[d];

        uint8_t result = rd >> 1 ;

        uint8_t n = 0;
        uint8_t z = result == 0;
        uint8_t c = get_bit(rd,0);
        uint8_t v = n ^ c;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000)  | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;

        #ifdef DEBUG
        printf("LSR\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0010110000000000)){
        // MOV
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t r= (instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5);

        self->registers[d] = self->registers[r];

        #ifdef DEBUG
        printf("MOV\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // MOVW
    }else if(NOT_IMPLEMENTED){
        // MUL
    }else if(NOT_IMPLEMENTED){
        // MULS
    }else if(NOT_IMPLEMENTED){
        // LMULSU
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000001)){
        // NEG
        uint8_t d = (instruction & 0b0000000111110000) >> 4;

        uint8_t rd = self->registers[d];

        uint8_t result = 255 - rd;

        uint8_t h = g_r3 | (!g_rd3);
        uint8_t v = result == 128;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t c = result != 0;
        uint8_t s = n ^ v;

        self->sreg = (self->sreg & 0b11000000) | (h << 4) | (s << 4) | (v << 3) | (n << 2) | (z << 1) | c;

        self->registers[d] = result;

        #ifdef DEBUG
        printf("NEG\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0010100000000000)){
        // OR
        uint8_t d = (instruction & 0b0000000111110000) >> 4;
        uint8_t r = (instruction & 0b0000000000001111) + ((instruction & 0b0000001000000000) >> 5);

        uint8_t rd = self->registers[d];
        uint8_t rr = self->registers[r];

        uint8_t result = rd | rr;

        uint8_t v = 0;
        uint8_t n = g_r7;
        uint8_t z = result == 0;
        uint8_t s = n ^ v;
        self->sreg = (self->sreg & 0b11100001) | (s << 4) | (v << 3) | (n << 2) | (z << 1) ;

        self->registers[d] = result;

        #ifdef DEBUG
        printf("OR\n");
        #endif
    }else if(instr_check(instruction, 0b1111000000000000, 0b0110000000000000)){
        // ORI
        #ifdef DEBUG
        printf("ORI\n");
        #endif
    }else if(instr_check(instruction, 0b1111100000000000, 0b1011100000000000)){
        // OUT
        #ifdef DEBUG
        printf("OUT\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // POP
    }else if(NOT_IMPLEMENTED){
        // PUSH
    }else if(NOT_IMPLEMENTED){
        // RCALL
    }else if(NOT_IMPLEMENTED){
        // RET
    }else if(NOT_IMPLEMENTED){
        // RETI
    }else if(instr_check(instruction, 0b1111000000000000, 0b1100000000000000)){
        // RJMP
        #ifdef DEBUG
        printf("RJMP\n");
        #endif
    }else if(GENERALIZATION_IMPLEMENTED){
        // ROL, same as ADC Rd, Rd
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000111)){
        // ROR
        #ifdef DEBUG
        printf("ROR\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0000100000000000)){
        // SBC
        #ifdef DEBUG
        printf("SBC\n");
        #endif
    }else if(instr_check(instruction, 0b1111000000000000, 0b0100000000000000)){
        // SBCI
        #ifdef DEBUG
        printf("SBCI\n");
        #endif
    }else if(instr_check(instruction, 0b1111111100000000, 0b1001101000000000)){
        // SBI
        #ifdef DEBUG
        printf("SBI\n");
        #endif
    }else if(instr_check(instruction, 0b1111111100000000, 0b1001100100000000)){
        // SBIC
        #ifdef DEBUG
        printf("SBIC\n");
        #endif
    }else if(instr_check(instruction, 0b1111111100000000, 0b1001101100000000)){
        // SBIS
        #ifdef DEBUG
        printf("SBIS\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // SBIW
    }else if(GENERALIZATION_IMPLEMENTED){
        // SBR, ORI Rd, K
    }else if(instr_check(instruction, 0b1111111000001000, 0b1111110000000000)){
        // SBRC
        #ifdef DEBUG
        printf("SBRC\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001000, 0b1111111000000000)){
        // SBRS
        #ifdef DEBUG
        printf("SBRS\n");
        #endif
    }else if (GENERALIZATION_IMPLEMENTED){
        // SER, LDI
    }else if(instr_check(instruction, 0b1111111111111111, 0b1001010110001000)){
        // SLEEP
        #ifdef DEBUG
        printf("SLEEP\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // SPM
    }else if(NOT_IMPLEMENTED){
        // ST
    }else if(NOT_IMPLEMENTED){
        // STS
    }else if(instr_check(instruction, 0b1111110000000000, 0b0001100000000000)){
        // SUB
        #ifdef DEBUG
        printf("SUB\n");
        #endif
    }else if(instr_check(instruction, 0b1111000000000000, 0b0101000000000000)){
        // SUBI
        #ifdef DEBUG
        printf("SUBI\n");
        #endif
    }else if(instr_check(instruction, 0b1111111000001111, 0b1001010000000010)){
        // SWAP
        #ifdef DEBUG
        printf("SWAP\n");
        #endif
    }else if(instr_check(instruction, 0b1111110000000000, 0b0010000000000000)){
        // TST
        #ifdef DEBUG
        printf("TST\n");
        #endif
    }else if(NOT_IMPLEMENTED){
        // WDR
    }else if(NOT_IMPLEMENTED){
        // XCH
    }

    self->program_counter+=1;

    return 0;
}

static PyObject *
AVRo_run_next_instruction(AVRoObject *self, PyObject *args)
{
    run_instruction(self);
    Py_RETURN_NONE;
}


static PyObject *
AVRo_run_until_break(AVRoObject *self, PyObject *args)
{
    while(!self->break_point_reached){
        run_instruction(self);
    }

    Py_RETURN_NONE;
}

static PyObject *
AVRo_run_instructions(AVRoObject *self, PyObject *args)
{
    // todo: Run X instructions
    uint64_t number_of_instructions;
    uint64_t executed_instructions = 0;
    if (!PyArg_ParseTuple(args, "k", &number_of_instructions)){
        //todo
        Py_RETURN_NONE;
    }

    while(executed_instructions < number_of_instructions){ // todo: Add Break instruction additionally
        run_instruction(self);
        executed_instructions+=1;
    }


    Py_RETURN_NONE;
}


static PyMethodDef AVRo_methods[] = {
    {"get_sreg",                (PyCFunction)AVRo_get_sreg,                             METH_VARARGS,                   PyDoc_STR("get SREG")},
    {"set_sreg",                (PyCFunction)AVRo_set_sreg,                             METH_VARARGS,                   PyDoc_STR("set SREG")},
    {"get_register",            (PyCFunction)AVRo_get_register,                         METH_VARARGS,                   PyDoc_STR("set SREG")},
    {"set_register",            (PyCFunction)(void(*)(void))AVRo_set_register,          METH_VARARGS | METH_KEYWORDS,   PyDoc_STR("updates register")},
    {"get_program_counter",     (PyCFunction)AVRo_get_program_counter,                  METH_VARARGS,                   PyDoc_STR("Get program counter")},
    {"set_program_memory",      (PyCFunction)(void(*)(void))AVRo_set_program_memory,    METH_VARARGS | METH_KEYWORDS,   PyDoc_STR("Set the program memory")},
    {"get_program_memory",      (PyCFunction)AVRo_get_program_memory,                   METH_VARARGS,                   PyDoc_STR("Get program counter")},
    {"get_program_memory_size", (PyCFunction)AVRo_get_program_memory_size,              METH_VARARGS,                   PyDoc_STR("Get program memory size")},
    {"get_sram_size",           (PyCFunction)AVRo_get_sram_size,                        METH_VARARGS,                   PyDoc_STR("Get sram size")},
    {"run_next_instruction",    (PyCFunction)AVRo_run_next_instruction,                 METH_VARARGS,                   PyDoc_STR("Run a single instruction")},
    {"run_until_break",         (PyCFunction)AVRo_run_until_break,                      METH_VARARGS,                   PyDoc_STR("Run up to and including the Break instruction")},
    {"run_instructions",        (PyCFunction)AVRo_run_instructions,                     METH_VARARGS,                   PyDoc_STR("Run x instructions")},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
AVRo_getattro(AVRoObject *self, PyObject *name)
{
    if (self->x_attr != NULL) {
        PyObject *v = PyDict_GetItemWithError(self->x_attr, name);
        if (v != NULL) {
            Py_INCREF(v);
            return v;
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int64_t
AVRo_setattr(AVRoObject *self, const char *name, PyObject *v)
{
    if (self->x_attr == NULL) {
        self->x_attr = PyDict_New();
        if (self->x_attr == NULL)
            return -1;
    }
    if (v == NULL) {
        int rv = PyDict_DelItemString(self->x_attr, name);
        if (rv < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing AVRo attribute");
        return rv;
    }
    else
        return PyDict_SetItemString(self->x_attr, name, v);
}


static PyTypeObject AVRo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "avrmodule.AVRoObject",
    .tp_basicsize = sizeof(AVRoObject),
    .tp_dealloc = (destructor)AVRo_dealloc,
    .tp_getattr = (getattrfunc)0,
    .tp_setattr = (setattrfunc)AVRo_setattr,
    .tp_getattro = (getattrofunc)AVRo_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = AVRo_methods,
};

/* --------------------------------------------------------------------- */

/* Function of no arguments returning new AVRo object */

static PyObject *
avr_new(PyObject *self, PyObject *args)
{
    AVRoObject *rv;

    if (!PyArg_ParseTuple(args, ":new"))
        return NULL;
    rv = newAVRoObject(args);
    if (rv == NULL)
        return NULL;
    return (PyObject *)rv;
}

/* ---------- */


/* List of functions defined in the module */

static PyMethodDef avr_methods[] = {
    {"new",             avr_new,         METH_VARARGS,           PyDoc_STR("new() -> new AVR object")},
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module is for emulating the AVR");


static int64_t
avr_exec(PyObject *m)
{
    /* Slot initialization is subject to the rules of initializing globals.
       C99 requires the initializers to be "address constants".  Function
       designators like 'PyType_GenericNew', with implicit conversion to
       a pointer, are valid C99 address constants.
       However, the unary '&' operator applied to a non-static variable
       like 'PyBaseObject_Type' is not required to produce an address
       constant.  Compilers may support this (gcc does), MSVC does not.
       Both compilers are strictly standard conforming in this particular
       behavior.
    */

    /* Finalize the type object including setting type of the new type
     * object; doing it here is required for portability, too. */
    if (PyType_Ready(&AVRo_Type) < 0)
        goto fail;

    return 0;
 fail:
    Py_XDECREF(m);
    return -1;
}

static struct PyModuleDef_Slot avr_slots[] = {
    {Py_mod_exec, avr_exec},
    {0, NULL},
};

static struct PyModuleDef avrmodule = {
    PyModuleDef_HEAD_INIT,
    "AVR",
    module_doc,
    0,
    avr_methods,
    avr_slots,
    NULL,
    NULL,
    NULL
};

/* Export function for the module (*must* be called PyInit_xx) */

PyMODINIT_FUNC
PyInit_avr(void)
{
    return PyModuleDef_Init(&avrmodule);
}