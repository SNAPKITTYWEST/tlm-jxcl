/* ═══════════════════════════════════════════════════════════════════════════
 * TLM JXCL IMPLEMENTATION - Part 3: Machine, Self-Test, P3, Output
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef JXCL_IMPL3_H
#define JXCL_IMPL3_H
#include "jxcl_impl2.h"

/* ── Machine Init ───────────────────────────────────────────────────────── */
static inline void jxcl_machine_init(jxcl_machine_t *m) {
    uint32_t i;
    for (i=0; i<REG_COUNT; i++) m->regs.gpr[i]=0;
    for (i=0; i<REG_SPECIAL_COUNT; i++) m->regs.special[i]=0;
    jxcl_state_init(&m->state);
    memset(m->mem.source,0,sizeof(m->mem.source));
    memset(m->mem.spiral,0,sizeof(m->mem.spiral));
    memset(m->mem.instr,0,sizeof(m->mem.instr));
    memset(m->mem.state,0,sizeof(m->mem.state));
    memset(m->mem.output,0,sizeof(m->mem.output));
    m->mem.trace_count=0;
    jxcl_spiral_init(&m->spiral);
    jxcl_spiral_reader_init(&m->spiral_reader);
    jxcl_frame_init(&m->frame);
    m->stack_ptr=0; m->exec_mode=MODE_COMB; m->halted=0; m->error=JXCL_OK;
    jxcl_source_to_mem(m->mem.source);
    jxcl_reg_write(&m->regs,REG_SRC_PTR,0);
    jxcl_reg_write(&m->regs,REG_SPI_PTR,0);
    jxcl_reg_write(&m->regs,REG_OUT_PTR,0);
    jxcl_reg_write(&m->regs,REG_DEPTH,JXCL_DEFAULT_DEPTH);
    jxcl_reg_write(&m->regs,REG_FLAGS,0);
}

static inline void jxcl_machine_init_full(jxcl_machine_t *m, uint32_t depth) {
    jxcl_machine_init(m);
    if (depth > JXCL_MAX_DEPTH) depth = JXCL_MAX_DEPTH;
    jxcl_reg_write(&m->regs,REG_DEPTH,depth);
    m->state.depth = depth;
}

/* ── Machine Step ───────────────────────────────────────────────────────── */
static inline int jxcl_machine_step(jxcl_machine_t *m) {
    if (m->halted) return JXCL_OK;
    if (m->error != JXCL_OK) return m->error;
    uint64_t pc = jxcl_reg_read(&m->regs,REG_PC);
    if (pc >= m->frame.count) { m->halted=1; return JXCL_OK; }
    jxcl_instr_t raw = m->frame.instrs[pc];
    jxcl_decode_t dec = jxcl_decode(raw);
    if (!dec.valid) { m->error=dec.err; m->halted=1; return dec.err; }
    if (m->exec_mode==MODE_PIPE)
        jxcl_trace_record(&m->mem,pc,&raw,&m->state);
    if (dec.op==OP_HALT) { m->halted=1; return JXCL_OK; }
    if (dec.op==OP_TRAP) { m->error=JXCL_ERR_ILLEGAL_OPCODE; m->halted=1; return m->error; }
    uint64_t va = (dec.src0!=0xFF)?jxcl_reg_read(&m->regs,dec.src0):0;
    uint64_t vb = (dec.src1!=0xFF)?jxcl_reg_read(&m->regs,dec.src1):0;
    uint64_t result = jxcl_execute_op(dec.op,va,vb,dec.imm,
        &m->regs.special[REG_FLAGS-REG_PC]);
    if (dec.op==OP_XCHG) {
        if (dec.src0!=0xFF && dec.dst!=0xFF) {
            uint64_t tmp=jxcl_reg_read(&m->regs,dec.dst);
            jxcl_reg_write(&m->regs,dec.dst,va);
            jxcl_reg_write(&m->regs,dec.src0,tmp);
        }
        m->state.epoch++; return JXCL_OK;
    }
    if (dec.op==OP_SELECT) {
        uint64_t fl=m->regs.special[REG_FLAGS-REG_PC];
        result=(fl&FLAG_Z)?vb:va;
    }
    if (dec.op==OP_EMIT) {
        uint64_t em=jxcl_state_result(&m->state);
        uint32_t op=(uint32_t)jxcl_reg_read(&m->regs,REG_OUT_PTR);
        if (op<256) { m->mem.output[op]=em; jxcl_reg_write(&m->regs,REG_OUT_PTR,op+1); }
        return JXCL_OK;
    }
    if (dec.op==OP_RECURSE) {
        uint64_t d=m->regs.special[REG_DEPTH-REG_PC];
        if (d==0) { jxcl_state_advance(&m->state,result); return JXCL_OK; }
        if (d>JXCL_MAX_DEPTH) { m->error=JXCL_ERR_DEPTH_EXCEEDED; m->halted=1; return m->error; }
        int pe=jxcl_recurse_push(m->stack,&m->stack_ptr,&m->state);
        if (pe!=JXCL_OK) { m->error=(uint32_t)pe; m->halted=1; return pe; }
        m->regs.special[REG_DEPTH-REG_PC]=d-1;
        m->state.depth=d-1;
        jxcl_state_advance(&m->state,result);
        return JXCL_OK;
    }
    if (dec.flags&IF_WRITEBACK) jxcl_reg_write(&m->regs,dec.dst,result);
    if (dec.op==OP_ACC) {
        uint64_t old=jxcl_reg_read(&m->regs,REG_ACC0);
        jxcl_reg_write(&m->regs,REG_ACC0,old^result);
    }
    jxcl_state_advance(&m->state,result);
    return JXCL_OK;
}

/* ── Machine Run ────────────────────────────────────────────────────────── */
static inline uint64_t jxcl_machine_run(jxcl_machine_t *m, uint32_t max) {
    uint32_t s=0;
    while (!m->halted && s<max) { int e=jxcl_machine_step(m); if (e) return 0; s++; }
    return jxcl_state_result(&m->state);
}

/* ── Source Injection ───────────────────────────────────────────────────── */
static inline void jxcl_source_inject(jxcl_machine_t *m, uint64_t w, uint32_t i) {
    if (i<256) m->mem.source[i] ^= w;
}

/* ── Build Core Frame ───────────────────────────────────────────────────── */
static inline void jxcl_build_core_frame(jxcl_isa_frame_t *f) {
    jxcl_frame_init(f);
    jxcl_frame_add(f,jxcl_mkf(OP_LOAD64,REG_R0,0xFF,0xFF,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_LOAD64,REG_R1,0xFF,0xFF,1,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_LOAD64,REG_R2,0xFF,0xFF,2,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_LOAD64,REG_R3,0xFF,0xFF,3,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR,REG_R4,REG_R0,REG_R1,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR,REG_R5,REG_R2,REG_R3,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR,REG_R6,REG_R4,REG_R5,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_MIX32,REG_R7,REG_R0,0xFF,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_MIX32,REG_R8,REG_R1,0xFF,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR,REG_R9,REG_R7,REG_R8,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR,REG_R10,REG_R6,REG_R9,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_ROL,REG_R11,REG_R10,0xFF,13,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR64,REG_R12,REG_R10,REG_R11,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_ROL,REG_R13,REG_R12,0xFF,7,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR64,REG_R14,REG_R12,REG_R13,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_ROL,REG_R15,REG_R14,0xFF,37,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR64,REG_R16,REG_R14,REG_R15,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_ROL,REG_R17,REG_R16,0xFF,23,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR64,REG_R18,REG_R16,REG_R17,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_FOLD64,REG_R19,REG_R18,0xFF,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_XOR64,REG_R20,REG_R19,REG_R10,0,IF_WRITEBACK));
    jxcl_frame_add(f,jxcl_mkf(OP_RECURSE,REG_R20,0xFF,0xFF,0,IF_RECURSE));
    jxcl_frame_add(f,jxcl_mkf(OP_EMIT,0xFF,0xFF,0xFF,0,IF_EMIT));
    jxcl_frame_add(f,jxcl_mk(OP_HALT,0xFF,0xFF,0xFF,0));
}

/* ── Self-Test ──────────────────────────────────────────────────────────── */
static inline void jxcl_selftest_init(jxcl_selftest_t *t) {
    t->xtime_pass=t->xtime_fail=t->mix_pass=t->mix_fail=0;
    t->xor_pass=t->xor_fail=t->rot_pass=t->rot_fail=0;
    t->fold_pass=t->fold_fail=t->spiral_pass=t->spiral_fail=0;
    t->recurse_pass=t->recurse_fail=t->total=0;
}

static inline int jxcl_selftest_xtime(jxcl_selftest_t *t) {
    struct { uint8_t i,e; } v[] = {
        {0x00,0x00},{0x01,0x02},{0x7F,0xFE},{0x80,0x1B},{0xFF,0xE5},{0x57,0xAE},{0x83,0x1D}};
    int pass=1; uint32_t i;
    for (i=0; i<7; i++) {
        if (jxcl_xtime(v[i].i)==v[i].e) t->xtime_pass++; else {t->xtime_fail++;pass=0;}
        t->total++;
    }
    return pass;
}

static inline int jxcl_selftest_mix(jxcl_selftest_t *t) {
    struct { uint8_t a0,a1,a2,a3,e0,e1,e2,e3; } v[] = {
        {0xD4,0xBF,0x5D,0x30,0x04,0x66,0x81,0xE5},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50},
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
        {0x80,0x00,0x80,0x00,0x9B,0x1B,0x9B,0x1B},
        {0x01,0x02,0x04,0x08,0x08,0x01,0x13,0x15},
        {0xFF,0x00,0x00,0x00,0xE5,0xFF,0xFF,0x1A},
        {0x80,0x00,0x00,0x00,0x1B,0x80,0x80,0x9B},
        {0x01,0x00,0x00,0x00,0x02,0x01,0x01,0x03},
        {0xAA,0x55,0xAA,0x55,0x4F,0xB0,0x4F,0xB0}};
    int pass=1; uint32_t i;
    for (i=0; i<10; i++) {
        jxcl_mix_result_t r=jxcl_mix32(v[i].a0,v[i].a1,v[i].a2,v[i].a3);
        if (r.y0==v[i].e0&&r.y1==v[i].e1&&r.y2==v[i].e2&&r.y3==v[i].e3) t->mix_pass++;
        else {t->mix_fail++;pass=0;}
        t->total++;
    }
    return pass;
}

static inline int jxcl_selftest_xor(jxcl_selftest_t *t) {
    struct { uint64_t a,b,e; } v[] = {
        {0ULL,0ULL,0ULL},{~0ULL,0ULL,~0ULL},{~0ULL,~0ULL,0ULL},
        {0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL,~0ULL},
        {0xDEADBEEFCAFEBABEULL,0xDEADBEEFCAFEBABEULL,0ULL},
        {0x0123456789ABCDEFULL,0xFEDCBA9876543210ULL,~0ULL}};
    int pass=1; uint32_t i;
    for (i=0; i<6; i++) {
        if ((v[i].a^v[i].b)==v[i].e) t->xor_pass++; else {t->xor_fail++;pass=0;}
        t->total++;
    }
    return pass;
}

static inline int jxcl_selftest_rot(jxcl_selftest_t *t) {
    int pass=1;
    if (jxcl_rol64(1ULL,1)==2ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    if (jxcl_rol64(0x8000000000000000ULL,1)==1ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    if (jxcl_ror64(2ULL,1)==1ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    if (jxcl_ror64(1ULL,1)==0x8000000000000000ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    if (jxcl_rol64(~0ULL,64)==~0ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    if (jxcl_rol64(0xFFFFFFFFULL,32)==0xFFFFFFFF00000000ULL) t->rot_pass++; else {t->rot_fail++;pass=0;} t->total++;
    return pass;
}

static inline int jxcl_selftest_fold(jxcl_selftest_t *t) {
    int pass=1;
    if (jxcl_fold64(0,0,0,0)==0) t->fold_pass++; else {t->fold_fail++;pass=0;} t->total++;
    if (jxcl_fold64(1,0,0,0)==1) t->fold_pass++; else {t->fold_fail++;pass=0;} t->total++;
    if (jxcl_fold64(0xFF,0xFF,0xFF,0xFF)==0) t->fold_pass++; else {t->fold_fail++;pass=0;} t->total++;
    if (jxcl_fold64(0xDEADBEEF,0xDEADBEEF,0xDEADBEEF,0xDEADBEEF)==0) t->fold_pass++; else {t->fold_fail++;pass=0;} t->total++;
    return pass;
}

static inline int jxcl_selftest_spiral(jxcl_selftest_t *t, const jxcl_spiral_t *s) {
    int pass=1;
    if (jxcl_spiral_validate(s)) t->spiral_pass++; else {t->spiral_fail++;pass=0;} t->total++;
    uint32_t seen[JXCL_MATRIX_TOTAL], k, unique=1;
    for (k=0;k<JXCL_MATRIX_TOTAL;k++) seen[k]=0;
    for (k=0;k<JXCL_MATRIX_TOTAL;k++) {
        if (s->perm[k]>=JXCL_MATRIX_TOTAL||seen[s->perm[k]]) {unique=0;break;}
        seen[s->perm[k]]=1;
    }
    if (unique) t->spiral_pass++; else {t->spiral_fail++;pass=0;} t->total++;
    return pass;
}

static inline int jxcl_selftest_recurse(jxcl_selftest_t *t) {
    int pass=1;
    jxcl_state_t st; jxcl_recurse_frame_t stk[JXCL_STACK_SIZE]; uint32_t sp=0;
    jxcl_state_init(&st); st.q0=0xDEADBEEF; st.depth=3;
    if (jxcl_recurse_push(stk,&sp,&st)==JXCL_OK) t->recurse_pass++; else {t->recurse_fail++;pass=0;} t->total++;
    st.q0=0xCAFEBABE; st.depth=2;
    if (jxcl_recurse_push(stk,&sp,&st)==JXCL_OK) t->recurse_pass++; else {t->recurse_fail++;pass=0;} t->total++;
    jxcl_state_t rst; jxcl_state_init(&rst);
    if (jxcl_recurse_pop(stk,&sp,&rst)==JXCL_OK && rst.q0==0xCAFEBABE) t->recurse_pass++; else {t->recurse_fail++;pass=0;} t->total++;
    if (jxcl_recurse_pop(stk,&sp,&rst)==JXCL_OK && rst.q0==0xDEADBEEF) t->recurse_pass++; else {t->recurse_fail++;pass=0;} t->total++;
    return pass;
}

static inline int jxcl_selftest_run(jxcl_selftest_t *t, const jxcl_spiral_t *s) {
    jxcl_selftest_init(t);
    int p=1;
    p&=jxcl_selftest_xtime(t); p&=jxcl_selftest_mix(t);
    p&=jxcl_selftest_xor(t); p&=jxcl_selftest_rot(t);
    p&=jxcl_selftest_fold(t); p&=jxcl_selftest_spiral(t,s);
    p&=jxcl_selftest_recurse(t);
    return p;
}

static inline void jxcl_selftest_report(const jxcl_selftest_t *t) {
    printf("=======================================================\n");
    printf("  JXCL SELF TEST REPORT\n");
    printf("=======================================================\n");
    printf("  XTIME:    %d pass, %d fail\n", t->xtime_pass, t->xtime_fail);
    printf("  MIX:      %d pass, %d fail\n", t->mix_pass, t->mix_fail);
    printf("  XOR:      %d pass, %d fail\n", t->xor_pass, t->xor_fail);
    printf("  ROTATE:   %d pass, %d fail\n", t->rot_pass, t->rot_fail);
    printf("  FOLD:     %d pass, %d fail\n", t->fold_pass, t->fold_fail);
    printf("  SPIRAL:   %d pass, %d fail\n", t->spiral_pass, t->spiral_fail);
    printf("  RECURSE:  %d pass, %d fail\n", t->recurse_pass, t->recurse_fail);
    printf("  --------------------------------------------------------\n");
    printf("  TOTAL:    %d tests\n", t->total);
    int fails=t->xtime_fail+t->mix_fail+t->xor_fail+t->rot_fail+t->fold_fail+t->spiral_fail+t->recurse_fail;
    printf("  STATUS:   %s\n", fails==0 ? "ALL PASS" : "FAILURES DETECTED");
    printf("=======================================================\n");
}

/* ── P3 Compatibility ───────────────────────────────────────────────────── */
static inline void jxcl_p3_report(void) {
    printf("=======================================================\n");
    printf("  P3 COMPATIBILITY VERIFICATION\n");
    printf("=======================================================\n");
    struct { const char *n; uint8_t a0,a1,a2,a3,e0,e1,e2,e3; } t[] = {
        {"D4 BF 5D 30",0xD4,0xBF,0x5D,0x30,0x04,0x66,0x81,0xE5},
        {"00 00 00 00",0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {"50 50 50 50",0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50},
        {"FF FF FF FF",0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
        {"80 00 80 00",0x80,0x00,0x80,0x00,0x9B,0x1B,0x9B,0x1B},
        {"01 02 04 08",0x01,0x02,0x04,0x08,0x08,0x01,0x13,0x15},
        {"FF 00 00 00",0xFF,0x00,0x00,0x00,0xE5,0xFF,0xFF,0x1A},
        {"80 00 00 00",0x80,0x00,0x00,0x00,0x1B,0x80,0x80,0x9B},
        {"01 00 00 00",0x01,0x00,0x00,0x00,0x02,0x01,0x01,0x03},
        {"AA 55 AA 55",0xAA,0x55,0xAA,0x55,0x4F,0xB0,0x4F,0xB0}};
    int pass=1; uint32_t i;
    for (i=0; i<10; i++) {
        jxcl_mix_result_t r=jxcl_mix32(t[i].a0,t[i].a1,t[i].a2,t[i].a3);
        int ok=(r.y0==t[i].e0&&r.y1==t[i].e1&&r.y2==t[i].e2&&r.y3==t[i].e3);
        printf("  [%s] %s -> %02X %02X %02X %02X (expected %02X %02X %02X %02X)\n",
               ok?"PASS":"FAIL",t[i].n,r.y0,r.y1,r.y2,r.y3,t[i].e0,t[i].e1,t[i].e2,t[i].e3);
        if (!ok) pass=0;
    }
    printf("  --------------------------------------------------------\n");
    printf("  P3 STATUS: %s\n", pass?"ALL PASS":"FAILURES DETECTED");
    printf("=======================================================\n");
}

/* ── Machine Audit Output ───────────────────────────────────────────────── */
static inline void jxcl_audit(const jxcl_machine_t *m) {
    printf("=======================================================\n");
    printf("  JXCL MACHINE AUDIT\n");
    printf("=======================================================\n");
    printf("  VERSION:       %d.%d.%d\n",JXCL_VERSION_MAJOR,JXCL_VERSION_MINOR,JXCL_VERSION_PATCH);
    printf("  ISA_VERSION:   0x%08X\n",JXCL_ISA_VERSION);
    printf("  SOURCE_BITS:   %d\n",JXCL_SOURCE_BITS);
    printf("  MATRIX_ROWS:   %d\n",JXCL_MATRIX_ROWS);
    printf("  MATRIX_COLS:   %d\n",JXCL_MATRIX_COLS);
    printf("  SPIRAL_LENGTH: %d\n",JXCL_MATRIX_TOTAL);
    printf("  REGISTER_WIDTH:%d\n",JXCL_REG_WIDTH);
    printf("  RECURSION_DEPTH:%llu\n",(unsigned long long)m->state.depth);
    printf("  INSTR_COUNT:   %u\n",m->frame.count);
    printf("  EPOCH:         %llu\n",(unsigned long long)m->state.epoch);
    printf("  FINAL_Q0:      0x%016llX\n",(unsigned long long)m->state.q0);
    printf("  FINAL_Q1:      0x%016llX\n",(unsigned long long)m->state.q1);
    printf("  FINAL_Q2:      0x%016llX\n",(unsigned long long)m->state.q2);
    printf("  FINAL_Q3:      0x%016llX\n",(unsigned long long)m->state.q3);
    printf("  FINAL_RESULT:  0x%016llX\n",(unsigned long long)jxcl_state_result(&m->state));
    printf("  HALTED:        %s\n",m->halted?"YES":"NO");
    printf("  ERROR:         %s\n",jxcl_error_string(m->error));
    printf("=======================================================\n");
}

#endif /* JXCL_IMPL3_H */
