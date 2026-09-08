/* ═══════════════════════════════════════════════════════════════════════════
 * TLM JXCL PURE RAW DENSE ISA FORGE - HEADER
 * Architecture: P4 ALGOL -> JXCL ISA -> P3 VHDL -> P2 GF(2^8) -> P1 BOOLEAN
 * SPDX-License-Identifier: MIT
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef JXCL_ISA_H
#define JXCL_ISA_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define JXCL_VERSION_MAJOR    1
#define JXCL_VERSION_MINOR    0
#define JXCL_VERSION_PATCH    0
#define JXCL_ISA_VERSION      0x00000001
#define JXCL_REG_WIDTH        64
#define JXCL_MAX_DEPTH        64
#define JXCL_DEFAULT_DEPTH    3
#define JXCL_SOURCE_BITS      1679
#define JXCL_SOURCE_BYTES     210
#define JXCL_MATRIX_ROWS      73
#define JXCL_MATRIX_COLS      23
#define JXCL_MATRIX_TOTAL     1679
#define JXCL_INSTR_SLOTS      256
#define JXCL_STACK_SIZE       1024
#define JXCL_TRACE_SIZE       4096
#define JXCL_REDUC_POLY       0x1B
#define JXCL_OK               0
#define JXCL_ERR_ILLEGAL_OPCODE  1
#define JXCL_ERR_ILLEGAL_REG     2
#define JXCL_ERR_ILLEGAL_ADDR    3
#define JXCL_ERR_STACK_OVERFLOW  4
#define JXCL_ERR_STACK_UNDERFLOW 5
#define JXCL_ERR_DEPTH_EXCEEDED  6
#define JXCL_ERR_UNINIT_STATE    7
#define JXCL_ERR_SOURCE_BOUNDS   8
#define JXCL_ERR_SPIRAL_BOUNDS   9
#define JXCL_TRUE    1
#define JXCL_FALSE   0

/* Opcodes */
#define OP_NOP     0x00
#define OP_MOV     0x01
#define OP_LOAD    0x02
#define OP_STORE   0x03
#define OP_XOR     0x04
#define OP_AND     0x05
#define OP_OR      0x06
#define OP_NOT     0x07
#define OP_SHL     0x08
#define OP_SHR     0x09
#define OP_ROL     0x0A
#define OP_ROR     0x0B
#define OP_XTIME   0x0C
#define OP_MIX32   0x0D
#define OP_MIX64   0x0E
#define OP_FOLD    0x0F
#define OP_CMP     0x10
#define OP_SELECT  0x11
#define OP_XCHG    0x12
#define OP_ACC     0x13
#define OP_ROTATE  0x14
#define OP_RECURSE 0x15
#define OP_EMIT    0x16
#define OP_HALT    0x17
#define OP_LOAD64  0x18
#define OP_ADD     0x19
#define OP_SUB     0x1A
#define OP_MUL8    0x1B
#define OP_FOLD64  0x1C
#define OP_XOR64   0x1D
#define OP_SRC_INJ 0x1E
#define OP_TRAP    0x1F
#define OP_COUNT   32

/* Registers */
#define REG_R0   0
#define REG_R1   1
#define REG_R2   2
#define REG_R3   3
#define REG_R4   4
#define REG_R5   5
#define REG_R6   6
#define REG_R7   7
#define REG_R8   8
#define REG_R9   9
#define REG_R10  10
#define REG_R11  11
#define REG_R12  12
#define REG_R13  13
#define REG_R14  14
#define REG_R15  15
#define REG_R16  16
#define REG_R17  17
#define REG_R18  18
#define REG_R19  19
#define REG_R20  20
#define REG_R21  21
#define REG_R22  22
#define REG_R23  23
#define REG_R24  24
#define REG_R25  25
#define REG_R26  26
#define REG_R27  27
#define REG_R28  28
#define REG_R29  29
#define REG_R30  30
#define REG_R31  31
#define REG_COUNT 32
#define REG_PC      32
#define REG_SP      33
#define REG_FLAGS   34
#define REG_DEPTH   35
#define REG_EPOCH   36
#define REG_SRC_PTR 37
#define REG_SPI_PTR 38
#define REG_OUT_PTR 39
#define REG_ACC0    40
#define REG_ACC1    41
#define REG_Q0      42
#define REG_Q1      43
#define REG_Q2      44
#define REG_Q3      45
#define REG_SPECIAL_COUNT 14
#define JXCL_REG_NAME_COUNT 46

/* Flags */
#define FLAG_Z 0x01
#define FLAG_N 0x02
#define FLAG_C 0x04
#define FLAG_V 0x08

/* Instruction flags */
#define IF_WRITEBACK 0x01
#define IF_SET_FLAGS 0x02
#define IF_MEMORY    0x04
#define IF_BRANCH    0x08
#define IF_RECURSE   0x10
#define IF_EMIT      0x20

#define MODE_COMB 0
#define MODE_PIPE 1

static const char *jxcl_op_names[OP_COUNT] = {
    "NOP","MOV","LOAD","STORE","XOR","AND","OR","NOT",
    "SHL","SHR","ROL","ROR","XTIME","MIX32","MIX64","FOLD",
    "CMP","SELECT","XCHG","ACC","ROTATE","RECURSE","EMIT","HALT",
    "LOAD64","ADD","SUB","MUL8","FOLD64","XOR64","SRC_INJ","TRAP"
};

static const char *jxcl_reg_names[JXCL_REG_NAME_COUNT] = {
    "R0","R1","R2","R3","R4","R5","R6","R7",
    "R8","R9","R10","R11","R12","R13","R14","R15",
    "R16","R17","R18","R19","R20","R21","R22","R23",
    "R24","R25","R26","R27","R28","R29","R30","R31",
    "PC","SP","FLAGS","DEPTH","EPOCH","SRC_PTR",
    "SPI_PTR","OUT_PTR","ACC0","ACC1","Q0","Q1","Q2","Q3"
};

/* Instruction format */
typedef struct {
    uint32_t op;
    uint32_t dst;
    uint32_t src0;
    uint32_t src1;
    uint64_t imm;
    uint32_t flags;
} jxcl_instr_t;

/* Register file */
typedef struct {
    uint64_t gpr[REG_COUNT];
    uint64_t special[REG_SPECIAL_COUNT];
} jxcl_regfile_t;

/* Memory map */
typedef struct {
    uint64_t source[256];
    uint64_t spiral[256];
    uint64_t instr[JXCL_INSTR_SLOTS];
    uint64_t state[64];
    uint64_t output[256];
    struct {
        uint64_t pc; uint32_t op; uint32_t dst;
        uint32_t src0; uint32_t src1; uint64_t imm;
        uint64_t q0, q1, q2, q3; uint64_t epoch;
    } trace[JXCL_TRACE_SIZE];
    uint32_t trace_count;
} jxcl_mem_t;

/* State machine */
typedef struct {
    uint64_t q0, q1, q2, q3;
    uint64_t depth;
    uint64_t epoch;
} jxcl_state_t;

/* Spiral permutation */
typedef struct {
    uint32_t perm[JXCL_MATRIX_TOTAL];
    uint32_t inverse[JXCL_MATRIX_TOTAL];
} jxcl_spiral_t;

/* Spiral reader */
typedef struct {
    uint32_t position;
    uint32_t total_bits;
} jxcl_spiral_reader_t;

/* ISA frame */
typedef struct {
    jxcl_instr_t instrs[JXCL_INSTR_SLOTS];
    uint32_t count;
    uint32_t entry;
} jxcl_isa_frame_t;

/* Recursion frame */
typedef struct {
    uint64_t state_save[4];
    uint64_t depth_save;
} jxcl_recurse_frame_t;

/* Decode result */
typedef struct {
    uint32_t op, dst, src0, src1;
    uint64_t imm;
    uint32_t flags;
    int valid;
    uint32_t err;
} jxcl_decode_t;

/* MixColumns result */
typedef struct {
    uint8_t y0, y1, y2, y3;
} jxcl_mix_result_t;

/* Self-test counters */
typedef struct {
    int xtime_pass, xtime_fail;
    int mix_pass, mix_fail;
    int xor_pass, xor_fail;
    int rot_pass, rot_fail;
    int fold_pass, fold_fail;
    int spiral_pass, spiral_fail;
    int recurse_pass, recurse_fail;
    int total;
} jxcl_selftest_t;

/* Machine (top-level) */
typedef struct {
    jxcl_regfile_t      regs;
    jxcl_state_t        state;
    jxcl_mem_t          mem;
    jxcl_spiral_t       spiral;
    jxcl_spiral_reader_t spiral_reader;
    jxcl_isa_frame_t    frame;
    jxcl_recurse_frame_t stack[JXCL_STACK_SIZE];
    uint32_t stack_ptr;
    uint32_t exec_mode;
    uint32_t halted;
    uint32_t error;
} jxcl_machine_t;

#endif /* JXCL_ISA_H */
