/* ═══════════════════════════════════════════════════════════════════════════
 * TLM JXCL PURE RAW DENSE ISA FORGE
 * MAIN ENTRYPOINT
 *
 * Architecture: P4 ALGOL -> JXCL ISA -> P3 VHDL -> P2 GF(2^8) -> P1 BOOLEAN
 * SPDX-License-Identifier: MIT
 * ═══════════════════════════════════════════════════════════════════════════ */
#include "jxcl_impl3.h"

int main(void) {
    jxcl_machine_t machine;
    jxcl_selftest_t st;
    uint32_t depth;

    printf("=======================================================\n");
    printf("  TLM JXCL PURE RAW DENSE ISA FORGE v%d.%d.%d\n", JXCL_VERSION_MAJOR, JXCL_VERSION_MINOR, JXCL_VERSION_PATCH);
    printf("  Architecture: P4->JXCL->P3->P2->P1\n");
    printf("=======================================================\n\n");

    /* ── Phase 1: Self-Test ──────────────────────────────────────────── */
    printf("[PHASE 1] SELF TEST\n");
    jxcl_machine_init(&machine);
    int st_pass = jxcl_selftest_run(&st, &machine.spiral);
    jxcl_selftest_report(&st);
    if (!st_pass) {
        printf("SELF TEST FAILED - HALTING\n");
        return 1;
    }

    /* ── Phase 2: P3 Compatibility ───────────────────────────────────── */
    printf("\n[PHASE 2] P3 COMPATIBILITY\n");
    jxcl_p3_report();

    /* ── Phase 3: Core Frame Execution ───────────────────────────────── */
    printf("\n[PHASE 3] CORE FRAME EXECUTION\n");
    for (depth = 0; depth <= 16; depth++) {
        jxcl_machine_init_full(&machine, depth);
        machine.exec_mode = MODE_PIPE;
        jxcl_build_core_frame(&machine.frame);
        uint64_t result = jxcl_machine_run(&machine, 1024);
        printf("  depth=%2u result=0x%016llX epoch=%llu\n",
               depth, (unsigned long long)result,
               (unsigned long long)machine.state.epoch);
    }

    /* ── Phase 4: Spiral Consumption ─────────────────────────────────── */
    printf("\n[PHASE 4] SPIRAL CONSUMPTION\n");
    jxcl_machine_init_full(&machine, JXCL_DEFAULT_DEPTH);
    machine.exec_mode = MODE_PIPE;
    uint32_t words_read = 0;
    uint64_t acc = 0;
    while (!jxcl_spiral_reader_done(&machine.spiral_reader)) {
        uint64_t w = jxcl_spiral_read_word(&machine.spiral_reader,
            &machine.spiral, machine.mem.source, 64);
        acc ^= w;
        words_read++;
    }
    printf("  spiral_words_read=%u\n", words_read);
    printf("  spiral_accumulator=0x%016llX\n", (unsigned long long)acc);
    printf("  spiral_bits_consumed=%u\n", JXCL_MATRIX_TOTAL);

    /* ── Phase 5: Recursive Depth Sweep ──────────────────────────────── */
    printf("\n[PHASE 5] RECURSIVE DEPTH SWEEP\n");
    uint32_t depths[] = {0, 1, 2, 3, 8, 16};
    uint32_t di;
    for (di = 0; di < 6; di++) {
        jxcl_machine_init_full(&machine, depths[di]);
        machine.exec_mode = MODE_PIPE;
        jxcl_build_core_frame(&machine.frame);
        uint64_t result = jxcl_machine_run(&machine, 4096);
        printf("  depth=%2u result=0x%016llX halted=%d epoch=%llu\n",
               depths[di], (unsigned long long)result,
               machine.halted, (unsigned long long)machine.state.epoch);
    }

    /* ── Phase 6: XTIME Exhaustive Verification ──────────────────────── */
    printf("\n[PHASE 6] XTIME EXHAUSTIVE (256 values)\n");
    int xt_pass = 1;
    uint32_t xt_fail_count = 0;
    for (uint32_t v = 0; v < 256; v++) {
        uint8_t x = jxcl_xtime((uint8_t)v);
        uint8_t b7 = (v >> 7) & 1;
        uint8_t shifted = (uint8_t)((v << 1) & 0xFF);
        uint8_t expected = b7 ? (shifted ^ JXCL_REDUC_POLY) : shifted;
        if (x != expected) {
            printf("  FAIL: xtime(0x%02X) = 0x%02X, expected 0x%02X\n", v, x, expected);
            xt_pass = 0;
            xt_fail_count++;
        }
    }
    printf("  XTIME: %s (%u failures)\n", xt_pass ? "ALL PASS" : "FAIL", xt_fail_count);

    /* ── Phase 7: MixColumns Exhaustive ───────────────────────────────── */
    printf("\n[PHASE 7] MIXCOLUMNS EXHAUSTIVE\n");
    uint32_t mix_test_vectors[][8] = {
        {0xD4,0xBF,0x5D,0x30, 0x04,0x66,0x81,0xE5},
        {0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00},
        {0x50,0x50,0x50,0x50, 0x50,0x50,0x50,0x50},
        {0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF},
        {0x80,0x00,0x80,0x00, 0x9B,0x1B,0x9B,0x1B},
        {0x01,0x02,0x04,0x08, 0x08,0x01,0x13,0x15},
        {0xFF,0x00,0x00,0x00, 0xE5,0xFF,0xFF,0x1A},
        {0x80,0x00,0x00,0x00, 0x1B,0x80,0x80,0x9B},
        {0x01,0x00,0x00,0x00, 0x02,0x01,0x01,0x03},
        {0xAA,0x55,0xAA,0x55, 0x4F,0xB0,0x4F,0xB0},
    };
    int mix_pass = 1;
    uint32_t tv;
    for (tv = 0; tv < 10; tv++) {
        jxcl_mix_result_t r = jxcl_mix32(
            (uint8_t)mix_test_vectors[tv][0], (uint8_t)mix_test_vectors[tv][1],
            (uint8_t)mix_test_vectors[tv][2], (uint8_t)mix_test_vectors[tv][3]);
        uint8_t e0=(uint8_t)mix_test_vectors[tv][4], e1=(uint8_t)mix_test_vectors[tv][5],
                e2=(uint8_t)mix_test_vectors[tv][6], e3=(uint8_t)mix_test_vectors[tv][7];
        int ok = (r.y0==e0 && r.y1==e1 && r.y2==e2 && r.y3==e3);
        if (!ok) {
            printf("  FAIL: Mix(%02X,%02X,%02X,%02X) = (%02X,%02X,%02X,%02X) expected (%02X,%02X,%02X,%02X)\n",
                mix_test_vectors[tv][0],mix_test_vectors[tv][1],
                mix_test_vectors[tv][2],mix_test_vectors[tv][3],
                r.y0,r.y1,r.y2,r.y3, e0,e1,e2,e3);
            mix_pass = 0;
        }
    }
    printf("  MIXCOLUMNS: %s\n", mix_pass ? "ALL PASS" : "FAIL");

    /* ── Phase 8: Full Run with Audit ────────────────────────────────── */
    printf("\n[PHASE 8] FULL MACHINE RUN\n");
    jxcl_machine_init_full(&machine, JXCL_DEFAULT_DEPTH);
    machine.exec_mode = MODE_PIPE;
    jxcl_build_core_frame(&machine.frame);
    uint64_t final_result = jxcl_machine_run(&machine, 8192);
    printf("  final_result = 0x%016llX\n", (unsigned long long)final_result);
    jxcl_audit(&machine);

    /* ── Phase 9: Trace Dump (first 20 entries) ──────────────────────── */
    printf("\n[PHASE 9] TRACE DUMP (first 20)\n");
    uint32_t trace_limit = machine.mem.trace_count < 20 ? machine.mem.trace_count : 20;
    uint32_t ti;
    for (ti = 0; ti < trace_limit; ti++) {
        void *e = &machine.mem.trace[ti];
        uint64_t epc   = ((uint64_t*)e)[0];
        uint32_t eop   = ((uint32_t*)e)[2];
        uint32_t edst  = ((uint32_t*)e)[3];
        uint32_t es0   = ((uint32_t*)e)[4];
        uint32_t es1   = ((uint32_t*)e)[5];
        uint64_t eimm  = ((uint64_t*)e)[3];
        uint64_t eq0   = ((uint64_t*)e)[4];
        uint64_t eq1   = ((uint64_t*)e)[5];
        uint64_t eq2   = ((uint64_t*)e)[6];
        uint64_t eq3   = ((uint64_t*)e)[7];
        uint64_t eep   = ((uint64_t*)e)[8];
        printf("  [%04u] PC=%04llu OP=%-8s DST=%-5s S0=%-5s S1=%-5s IMM=0x%016llX "
               "Q0=0x%016llX Q1=0x%016llX Q2=0x%016llX Q3=0x%016llX E=%llu\n",
            ti, (unsigned long long)epc,
            (eop<OP_COUNT)?jxcl_op_names[eop]:"???",
            (edst<JXCL_REG_NAME_COUNT)?jxcl_reg_names[edst]:"???",
            (es0<JXCL_REG_NAME_COUNT)?jxcl_reg_names[es0]:"???",
            (es1<JXCL_REG_NAME_COUNT)?jxcl_reg_names[es1]:"???",
            (unsigned long long)eimm, (unsigned long long)eq0,
            (unsigned long long)eq1, (unsigned long long)eq2,
            (unsigned long long)eq3, (unsigned long long)eep);
    }
    if (machine.mem.trace_count > 20)
        printf("  ... (%u more entries)\n", machine.mem.trace_count - 20);

    /* ── Phase 10: Source Injection Test ──────────────────────────────── */
    printf("\n[PHASE 10] SOURCE INJECTION\n");
    jxcl_machine_init_full(&machine, 2);
    machine.exec_mode = MODE_PIPE;
    jxcl_source_inject(&machine, 0xDEADBEEFCAFEBABEULL, 0);
    jxcl_source_inject(&machine, 0x0123456789ABCDEFULL, 1);
    jxcl_build_core_frame(&machine.frame);
    uint64_t inj_result = jxcl_machine_run(&machine, 2048);
    printf("  injected_result = 0x%016llX\n", (unsigned long long)inj_result);

    /* ── Phase 11: Fibonacci Fold Test ────────────────────────────────── */
    printf("\n[PHASE 11] FIBONACCI FOLD\n");
    uint64_t fib_a = 1, fib_b = 1, fib_xor = 0;
    uint32_t fi;
    for (fi = 0; fi < 64; fi++) {
        fib_xor ^= fib_a;
        uint64_t fib_c = fib_a + fib_b;
        fib_a = fib_b;
        fib_b = fib_c;
    }
    uint64_t folded = jxcl_fold64(fib_xor, fib_a, fib_b, 0);
    printf("  fib_xor=0x%016llX folded=0x%016llX\n",
           (unsigned long long)fib_xor, (unsigned long long)folded);

    /* ── Phase 12: Boundary Rotation ─────────────────────────────────── */
    printf("\n[PHASE 12] BOUNDARY ROTATION\n");
    uint64_t rot_tests[][2] = {
        {0x0000000000000001ULL, 1},   {0x8000000000000000ULL, 1},
        {0xFFFFFFFFFFFFFFFFULL, 0},    {0xFFFFFFFFFFFFFFFFULL, 64},
        {0x00000000FFFFFFFFULL, 32},   {0xDEADBEEFCAFEBABEULL, 17},
    };
    uint32_t ri;
    for (ri = 0; ri < 6; ri++) {
        uint64_t val = rot_tests[ri][0];
        uint32_t amt = (uint32_t)rot_tests[ri][1];
        uint64_t lr = jxcl_rol64(val, amt);
        uint64_t rr = jxcl_ror64(lr, amt);
        printf("  ROL(val, %2u) = 0x%016llX  ROR(result, %2u) = 0x%016llX  %s\n",
            amt, (unsigned long long)lr, amt, (unsigned long long)rr,
            rr==val ? "OK" : "MISMATCH");
    }

    /* ── Phase 13: GF(256) Multiply Test ─────────────────────────────── */
    printf("\n[PHASE 13] GF(256) MULTIPLY\n");
    printf("  gf256_mul(0x02, 0x87) = 0x%02X\n", jxcl_gf256_mul(0x02, 0x87));
    printf("  gf256_mul(0x03, 0x0E) = 0x%02X\n", jxcl_gf256_mul(0x03, 0x0E));
    printf("  gf256_mul(0xFF, 0xFF) = 0x%02X\n", jxcl_gf256_mul(0xFF, 0xFF));
    printf("  gf256_mul(0x01, 0x00) = 0x%02X (expected 0x00)\n",
        jxcl_gf256_mul(0x01, 0x00));

    /* ── Final Summary ───────────────────────────────────────────────── */
    printf("\n=======================================================\n");
    printf("  JXCL EXECUTION COMPLETE\n");
    printf("  All phases executed deterministically.\n");
    printf("  No floating point used.\n");
    printf("  No PRNG or external entropy.\n");
    printf("  No physical quantum claims.\n");
    printf("  No P4 settlement logic.\n");
    printf("  P3(input) = P2(input) verified.\n");
    printf("  Source bits: %d (Arecibo 73x23)\n", JXCL_SOURCE_BITS);
    printf("  Spiral length: %d\n", JXCL_MATRIX_TOTAL);
    printf("  ISA version: 0x%08X\n", JXCL_ISA_VERSION);
    printf("=======================================================\n");

    return 0;
}
