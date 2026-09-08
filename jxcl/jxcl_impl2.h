/* ═══════════════════════════════════════════════════════════════════════════
 * TLM JXCL IMPLEMENTATION - Part 2: Spiral, Matrix, Decode, Execute
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef JXCL_IMPL2_H
#define JXCL_IMPL2_H
#include "jxcl_impl.h"

/* ── Matrix ─────────────────────────────────────────────────────────────── */
static inline uint32_t jxcl_matrix_index(uint32_t r, uint32_t c) {
    return r * JXCL_MATRIX_COLS + c;
}

/* ── Spiral Permutation (fixed-point, no float) ─────────────────────────── */
#define SPIRAL_A  1024
#define SPIRAL_B  2340
#define SPIRAL_PI 205887
#define SPIRAL_2PI 411774
#define SPIRAL_HPI 102943
#define SPIRAL_SCALE 16

static inline int32_t jxcl_fixed_cos(uint32_t a) {
    uint32_t ang = a % SPIRAL_2PI;
    if (ang < SPIRAL_HPI)
        return (int32_t)(16384 - (ang*ang)/2048);
    else if (ang < SPIRAL_HPI*3) {
        uint32_t b = ang - SPIRAL_HPI;
        return -(int32_t)(16384 - (b*b)/2048);
    } else {
        uint32_t c = ang - SPIRAL_HPI*3;
        return (int32_t)((c*c)/2048 - 16384);
    }
}
static inline int32_t jxcl_fixed_sin(uint32_t a) {
    return jxcl_fixed_cos((a + SPIRAL_HPI) % SPIRAL_2PI);
}

static inline void jxcl_spiral_init(jxcl_spiral_t *s) {
    uint32_t k, used[JXCL_MATRIX_TOTAL], scan;
    uint32_t cr = JXCL_MATRIX_ROWS/2, cc = JXCL_MATRIX_COLS/2;
    for (k = 0; k < JXCL_MATRIX_TOTAL; k++) { used[k]=0; s->perm[k]=0; s->inverse[k]=0; }
    for (k = 0; k < JXCL_MATRIX_TOTAL; k++) {
        uint32_t ang = (k * SPIRAL_2PI) / JXCL_MATRIX_TOTAL;
        int32_t cv = jxcl_fixed_cos(ang), sv = jxcl_fixed_sin(ang);
        int32_t rf = SPIRAL_A + (int32_t)((int64_t)SPIRAL_B*k/JXCL_MATRIX_TOTAL);
        int32_t dr = (rf * cv) >> (SPIRAL_SCALE+7);
        int32_t dc = (rf * sv) >> (SPIRAL_SCALE+7);
        int32_t row = (int32_t)cr + dr, col = (int32_t)cc + dc;
        if (row<0) row=0;
        if (row>=JXCL_MATRIX_ROWS) row=JXCL_MATRIX_ROWS-1;
        if (col<0) col=0;
        if (col>=JXCL_MATRIX_COLS) col=JXCL_MATRIX_COLS-1;
        uint32_t idx = jxcl_matrix_index((uint32_t)row,(uint32_t)col);
        if (used[idx]) for (scan=0; scan<JXCL_MATRIX_TOTAL; scan++) if (!used[scan]) { idx=scan; break; }
        s->perm[k]=idx; used[idx]=1;
    }
    for (k=0; k<JXCL_MATRIX_TOTAL; k++) s->inverse[s->perm[k]]=k;
}

static inline int jxcl_spiral_validate(const jxcl_spiral_t *s) {
    uint32_t seen[JXCL_MATRIX_TOTAL], k;
    for (k=0; k<JXCL_MATRIX_TOTAL; k++) seen[k]=0;
    for (k=0; k<JXCL_MATRIX_TOTAL; k++) {
        if (s->perm[k]>=JXCL_MATRIX_TOTAL || seen[s->perm[k]]) return JXCL_FALSE;
        seen[s->perm[k]]=1;
    }
    for (k=0; k<JXCL_MATRIX_TOTAL; k++)
        if (s->inverse[k]>=JXCL_MATRIX_TOTAL || s->perm[s->inverse[k]]!=k) return JXCL_FALSE;
    return JXCL_TRUE;
}

/* ── Spiral Reader ──────────────────────────────────────────────────────── */
static inline void jxcl_spiral_reader_init(jxcl_spiral_reader_t *r) {
    r->position=0; r->total_bits=JXCL_MATRIX_TOTAL;
}
static inline uint64_t jxcl_spiral_read_word(jxcl_spiral_reader_t *r,
    const jxcl_spiral_t *s, const uint64_t *src, uint32_t width) {
    uint64_t result=0; uint32_t i;
    for (i=0; i<width; i++) {
        if (r->position >= r->total_bits) break;
        uint32_t si = s->perm[r->position];
        uint32_t bi = si/64, bw = si%64;
        if (bi < 256) result |= ((src[bi]>>bw)&1ULL) << i;
        r->position++;
    }
    return result;
}
static inline int jxcl_spiral_reader_done(const jxcl_spiral_reader_t *r) {
    return r->position >= r->total_bits;
}

/* ── Instruction constructors ───────────────────────────────────────────── */
static inline jxcl_instr_t jxcl_mk(uint32_t op, uint32_t dst,
    uint32_t s0, uint32_t s1, uint64_t imm) {
    jxcl_instr_t i; i.op=op; i.dst=dst; i.src0=s0; i.src1=s1; i.imm=imm; i.flags=0; return i;
}
static inline jxcl_instr_t jxcl_mkf(uint32_t op, uint32_t dst,
    uint32_t s0, uint32_t s1, uint64_t imm, uint32_t fl) {
    jxcl_instr_t i; i.op=op; i.dst=dst; i.src0=s0; i.src1=s1; i.imm=imm; i.flags=fl; return i;
}
static inline int jxcl_instr_valid(const jxcl_instr_t *i) {
    if (i->op>=OP_COUNT) return JXCL_FALSE;
    if (i->dst<JXCL_REG_NAME_COUNT || i->dst==0xFF) {}
    else return JXCL_FALSE;
    return JXCL_TRUE;
}

/* ── Decode ─────────────────────────────────────────────────────────────── */
static inline jxcl_decode_t jxcl_decode(jxcl_instr_t instr) {
    jxcl_decode_t d;
    d.op=instr.op; d.dst=instr.dst; d.src0=instr.src0; d.src1=instr.src1;
    d.imm=instr.imm; d.flags=instr.flags; d.valid=1; d.err=JXCL_OK;
    if (instr.op>=OP_COUNT) { d.valid=0; d.err=JXCL_ERR_ILLEGAL_OPCODE; }
    if (instr.dst!=0xFF && instr.dst>=JXCL_REG_NAME_COUNT) { d.valid=0; d.err=JXCL_ERR_ILLEGAL_REG; }
    if (instr.src0!=0xFF && instr.src0>=JXCL_REG_NAME_COUNT) { d.valid=0; d.err=JXCL_ERR_ILLEGAL_REG; }
    if (instr.src1!=0xFF && instr.src1>=JXCL_REG_NAME_COUNT) { d.valid=0; d.err=JXCL_ERR_ILLEGAL_REG; }
    return d;
}

/* ── Execute ────────────────────────────────────────────────────────────── */
static inline uint64_t jxcl_execute_op(uint32_t op, uint64_t a, uint64_t b,
    uint64_t imm, uint64_t *flags) {
    uint64_t r = 0;
    switch (op) {
    case OP_NOP:    r=0; break;
    case OP_MOV:    r=a; break;
    case OP_XOR:    r=a^b; jxcl_flags_upd(flags,r); break;
    case OP_AND:    r=a&b; jxcl_flags_upd(flags,r); break;
    case OP_OR:     r=a|b; jxcl_flags_upd(flags,r); break;
    case OP_NOT:    r=~a;  jxcl_flags_upd(flags,r); break;
    case OP_SHL:    r=a<<(b&63); jxcl_flags_upd(flags,r); break;
    case OP_SHR:    r=a>>(b&63); jxcl_flags_upd(flags,r); break;
    case OP_ROL:    r=jxcl_rol64(a,(uint32_t)b); jxcl_flags_upd(flags,r); break;
    case OP_ROR:    r=jxcl_ror64(a,(uint32_t)b); jxcl_flags_upd(flags,r); break;
    case OP_ADD:    r=a+b; jxcl_flags_upd(flags,r); break;
    case OP_SUB:    r=a-b; jxcl_flags_upd(flags,r); break;
    case OP_XTIME:  r=jxcl_xtime((uint8_t)(a&0xFF)); jxcl_flags_upd8(flags,(uint8_t)r); break;
    case OP_MIX32:  r=jxcl_mix32_word((uint32_t)a); jxcl_flags_upd32(flags,(uint32_t)r); break;
    case OP_MIX64:  r=jxcl_mix64(a); jxcl_flags_upd(flags,r); break;
    case OP_FOLD:   r=jxcl_fold64(a,b,imm,0); jxcl_flags_upd(flags,r); break;
    case OP_FOLD64: r=jxcl_fold64(a,b,imm,0); jxcl_flags_upd(flags,r); break;
    case OP_XOR64:  r=a^b; jxcl_flags_upd(flags,r); break;
    case OP_CMP:    r=(a==b)?0:(a>b)?1:2; jxcl_flags_upd(flags,a^b); break;
    case OP_SELECT: r=(flags && (*flags&FLAG_Z))?b:a; break;
    case OP_XCHG:   r=a; break;
    case OP_ACC:    r=a+b; jxcl_flags_upd(flags,r); break;
    case OP_ROTATE: r=jxcl_rol64(a,(uint32_t)b); break;
    case OP_MUL8:   r=jxcl_gf256_mul((uint8_t)(a&0xFF),(uint8_t)(b&0xFF)); jxcl_flags_upd8(flags,(uint8_t)r); break;
    case OP_TRAP:   r=0xDEAD; break;
    default: r=0; break;
    }
    return r;
}

/* ── Recursion ──────────────────────────────────────────────────────────── */
static inline int jxcl_recurse_push(jxcl_recurse_frame_t *stk, uint32_t *sp,
    const jxcl_state_t *s) {
    if (*sp >= JXCL_STACK_SIZE) return JXCL_ERR_STACK_OVERFLOW;
    stk[*sp].state_save[0]=s->q0; stk[*sp].state_save[1]=s->q1;
    stk[*sp].state_save[2]=s->q2; stk[*sp].state_save[3]=s->q3;
    stk[*sp].depth_save=s->depth; (*sp)++;
    return JXCL_OK;
}
static inline int jxcl_recurse_pop(jxcl_recurse_frame_t *stk, uint32_t *sp,
    jxcl_state_t *s) {
    if (!*sp) return JXCL_ERR_STACK_UNDERFLOW;
    (*sp)--;
    s->q0=stk[*sp].state_save[0]; s->q1=stk[*sp].state_save[1];
    s->q2=stk[*sp].state_save[2]; s->q3=stk[*sp].state_save[3];
    s->depth=stk[*sp].depth_save;
    return JXCL_OK;
}

/* ── ISA Frame ──────────────────────────────────────────────────────────── */
static inline void jxcl_frame_init(jxcl_isa_frame_t *f) {
    uint32_t i; f->count=0; f->entry=0;
    for (i=0; i<JXCL_INSTR_SLOTS; i++)
        f->instrs[i]=jxcl_mk(OP_NOP,0xFF,0xFF,0xFF,0);
}
static inline int jxcl_frame_add(jxcl_isa_frame_t *f, jxcl_instr_t i) {
    if (f->count>=JXCL_INSTR_SLOTS) return JXCL_ERR_ILLEGAL_OPCODE;
    f->instrs[f->count++]=i; return JXCL_OK;
}

/* ── Trace ──────────────────────────────────────────────────────────────── */
static inline void jxcl_trace_record(jxcl_mem_t *m, uint64_t pc,
    const jxcl_instr_t *i, const jxcl_state_t *s) {
    if (m->trace_count >= JXCL_TRACE_SIZE) return;
    void *e = &m->trace[m->trace_count];
    /* manually copy fields to avoid struct padding issues */
    ((uint64_t*)e)[0] = pc;
    ((uint32_t*)e)[2] = i->op; ((uint32_t*)e)[3] = i->dst;
    ((uint32_t*)e)[4] = i->src0; ((uint32_t*)e)[5] = i->src1;
    ((uint64_t*)e)[3] = i->imm;
    ((uint64_t*)e)[4] = s->q0; ((uint64_t*)e)[5] = s->q1;
    ((uint64_t*)e)[6] = s->q2; ((uint64_t*)e)[7] = s->q3;
    ((uint64_t*)e)[8] = s->epoch;
    m->trace_count++;
}

/* ── Error strings ──────────────────────────────────────────────────────── */
static inline const char *jxcl_error_string(uint32_t e) {
    switch(e) {
    case JXCL_OK: return "OK";
    case JXCL_ERR_ILLEGAL_OPCODE: return "ILLEGAL_OPCODE";
    case JXCL_ERR_ILLEGAL_REG: return "ILLEGAL_REGISTER";
    case JXCL_ERR_ILLEGAL_ADDR: return "ILLEGAL_ADDRESS";
    case JXCL_ERR_STACK_OVERFLOW: return "STACK_OVERFLOW";
    case JXCL_ERR_STACK_UNDERFLOW: return "STACK_UNDERFLOW";
    case JXCL_ERR_DEPTH_EXCEEDED: return "DEPTH_EXCEEDED";
    case JXCL_ERR_UNINIT_STATE: return "UNINIT_STATE";
    case JXCL_ERR_SOURCE_BOUNDS: return "SOURCE_BOUNDS";
    case JXCL_ERR_SPIRAL_BOUNDS: return "SPIRAL_BOUNDS";
    default: return "UNKNOWN";
    }
}

#endif /* JXCL_IMPL2_H */
