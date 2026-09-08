/* ═══════════════════════════════════════════════════════════════════════════
 * TLM JXCL IMPLEMENTATION - Part 1: Bit/Byte/GF primitives
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef JXCL_IMPL_H
#define JXCL_IMPL_H
#include "jxcl_isa.h"

/* ── Bit Primitives ─────────────────────────────────────────────────────── */
static inline uint64_t jxcl_bit_get(uint64_t v, uint32_t p) { return (v>>p)&1; }
static inline uint64_t jxcl_bit_set(uint64_t v, uint32_t p) { return v|(1ULL<<p); }
static inline uint64_t jxcl_bit_clr(uint64_t v, uint32_t p) { return v&~(1ULL<<p); }
static inline uint64_t jxcl_bits_extract(uint64_t v, uint32_t hi, uint32_t lo) {
    return (v >> lo) & ((1ULL << (hi - lo + 1)) - 1);
}
static inline uint32_t jxcl_popcount64(uint64_t v) {
    v = v - ((v>>1) & 0x5555555555555555ULL);
    v = (v & 0x3333333333333333ULL) + ((v>>2) & 0x3333333333333333ULL);
    return (uint32_t)(((v + (v>>4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL >> 56);
}
static inline uint32_t jxcl_clz64(uint64_t v) {
    uint32_t n = 0;
    if (!v) return 64;
    if (!(v & 0xFFFFFFFF00000000ULL)) { n+=32; v<<=32; }
    if (!(v & 0xFFFF000000000000ULL)) { n+=16; v<<=16; }
    if (!(v & 0xFF00000000000000ULL)) { n+=8;  v<<=8;  }
    if (!(v & 0xF000000000000000ULL)) { n+=4;  v<<=4;  }
    if (!(v & 0xC000000000000000ULL)) { n+=2;  v<<=2;  }
    if (!(v & 0x8000000000000000ULL)) { n+=1; }
    return n;
}
static inline uint32_t jxcl_ctz64(uint64_t v) {
    uint32_t n = 0;
    if (!v) return 64;
    if (!(v & 0x00000000FFFFFFFFULL)) { n+=32; v>>=32; }
    if (!(v & 0x000000000000FFFFULL)) { n+=16; v>>=16; }
    if (!(v & 0x00000000000000FFULL)) { n+=8;  v>>=8;  }
    if (!(v & 0x000000000000000FULL)) { n+=4;  v>>=4;  }
    if (!(v & 0x0000000000000003ULL)) { n+=2;  v>>=2;  }
    if (!(v & 0x0000000000000001ULL)) { n+=1; }
    return n;
}
static inline uint64_t jxcl_bswap64(uint64_t v) {
    return ((v&0xFF)<<56)|((v&0xFF00)<<40)|((v&0xFF0000)<<24)|((v&0xFF000000)<<8)|
           ((v&0xFF00000000ULL)>>8)|((v&0xFF0000000000ULL)>>24)|
           ((v&0xFF000000000000ULL)>>40)|((v&0xFF00000000000000ULL)>>56);
}

/* ── Byte Primitives ────────────────────────────────────────────────────── */
static inline uint8_t  jxcl_byte_rol8(uint8_t a, uint32_t s) {
    s&=7; return s?((uint8_t)((a<<s)|(a>>(8-s)))):a;
}
static inline uint8_t  jxcl_byte_ror8(uint8_t a, uint32_t s) {
    s&=7; return s?((uint8_t)((a>>s)|(a<<(8-s)))):a;
}
static inline uint32_t jxcl_word32_byte(uint32_t w, uint32_t i) {
    return (uint8_t)(w>>((i&3)*8));
}
static inline uint32_t jxcl_word32_from_bytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    return (uint32_t)b0|((uint32_t)b1<<8)|((uint32_t)b2<<16)|((uint32_t)b3<<24);
}
static inline uint64_t jxcl_word64_lo32(uint64_t v) { return (uint32_t)v; }
static inline uint64_t jxcl_word64_hi32(uint64_t v) { return (uint32_t)(v>>32); }
static inline uint64_t jxcl_word64_from32(uint32_t hi, uint32_t lo) {
    return ((uint64_t)hi<<32)|(uint64_t)lo;
}

/* ── Rotation 64-bit ────────────────────────────────────────────────────── */
static inline uint64_t jxcl_rol64(uint64_t a, uint32_t s) {
    s&=63; return s?((a<<s)|(a>>(64-s))):a;
}
static inline uint64_t jxcl_ror64(uint64_t a, uint32_t s) {
    s&=63; return s?((a>>s)|(a<<(64-s))):a;
}
static inline uint32_t jxcl_rol32(uint32_t a, uint32_t s) {
    s&=31; return s?((a<<s)|(a>>(32-s))):a;
}
static inline uint32_t jxcl_ror32(uint32_t a, uint32_t s) {
    s&=31; return s?((a>>s)|(a<<(32-s))):a;
}

/* ── Flags ──────────────────────────────────────────────────────────────── */
static inline void jxcl_flags_upd(uint64_t *f, uint64_t r) {
    uint32_t fl = 0;
    if (r == 0) fl |= FLAG_Z;
    if (r & 0x8000000000000000ULL) fl |= FLAG_N;
    *f = (*f & 0xFFFFFFF0) | fl;
}
static inline void jxcl_flags_upd32(uint64_t *f, uint32_t r) {
    uint32_t fl = 0;
    if (r == 0) fl |= FLAG_Z;
    if (r & 0x80000000) fl |= FLAG_N;
    *f = (*f & 0xFFFFFFF0) | fl;
}
static inline void jxcl_flags_upd8(uint64_t *f, uint8_t r) {
    uint32_t fl = 0;
    if (r == 0) fl |= FLAG_Z;
    if (r & 0x80) fl |= FLAG_N;
    *f = (*f & 0xFFFFFFF0) | fl;
}

/* ── XTIME: GF(2^8) x {02} ─────────────────────────────────────────────── *
 * Standard xtime: shift left, reduce with 0x1B if overflow.               *
 * b7 b6 b5 b4 b3 b2 b1 b0                                                 *
 * x7 = b6                                                                  *
 * x6 = b5                                                                  *
 * x5 = b4 XOR b7                                                          *
 * x4 = b3 XOR b7                                                          *
 * x3 = b2                                                                  *
 * x2 = b1                                                                  *
 * x1 = b0 XOR b7                                                          *
 * x0 = b7                                                                  */
static inline uint8_t jxcl_xtime(uint8_t b) {
    uint8_t r = (uint8_t)((uint32_t)b << 1);
    if (b & 0x80) r ^= JXCL_REDUC_POLY;
    return r;
}

/* ── GF(2^8) multiply ──────────────────────────────────────────────────── */
static inline uint8_t jxcl_gf256_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    int i;
    for (i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi = a & 0x80;
        a = (uint8_t)(a << 1);
        if (hi) a ^= JXCL_REDUC_POLY;
        b >>= 1;
    }
    return p;
}

/* ── MIXCOLUMNS ─────────────────────────────────────────────────────────── */
static inline jxcl_mix_result_t jxcl_mix32(uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3) {
    jxcl_mix_result_t r;
    uint8_t x0=jxcl_xtime(a0), x1=jxcl_xtime(a1);
    uint8_t x2=jxcl_xtime(a2), x3=jxcl_xtime(a3);
    r.y0=x0^(x1^a1)^a2^a3;
    r.y1=a0^x1^(x2^a2)^a3;
    r.y2=a0^a1^x2^(x3^a3);
    r.y3=(x0^a0)^a1^a2^x3;
    return r;
}
static inline uint32_t jxcl_mix32_word(uint32_t c) {
    jxcl_mix_result_t r=jxcl_mix32(jxcl_word32_byte(c,0),jxcl_word32_byte(c,1),
                                    jxcl_word32_byte(c,2),jxcl_word32_byte(c,3));
    return jxcl_word32_from_bytes(r.y0,r.y1,r.y2,r.y3);
}
static inline uint64_t jxcl_mix64(uint64_t v) {
    return jxcl_word64_from32(jxcl_mix32_word(jxcl_word64_hi32(v)),
                              jxcl_mix32_word(jxcl_word64_lo32(v)));
}

/* ── Folding ────────────────────────────────────────────────────────────── */
static inline uint64_t jxcl_fold64(uint64_t q0, uint64_t q1, uint64_t q2, uint64_t q3) {
    uint64_t x = q0^q1^q2^q3;
    x^=x>>32; x^=x>>16; x^=x>>8; x^=x>>4; x^=x>>2; x^=x>>1;
    return x;
}
static inline uint32_t jxcl_fold32(uint32_t v) {
    v^=v>>16; v^=v>>8; v^=v>>4; v^=v>>2; v^=v>>1; return v;
}

/* ── State Machine ──────────────────────────────────────────────────────── */
static inline void jxcl_state_init(jxcl_state_t *s) {
    s->q0=s->q1=s->q2=s->q3=s->depth=s->epoch=0;
}
static inline void jxcl_state_advance(jxcl_state_t *s, uint64_t r) {
    s->q3=s->q2; s->q2=s->q1; s->q1=s->q0; s->q0=r; s->epoch++;
}
static inline uint64_t jxcl_state_result(const jxcl_state_t *s) {
    return jxcl_fold64(s->q0,s->q1,s->q2,s->q3);
}

/* ── Register File ──────────────────────────────────────────────────────── */
static inline uint64_t jxcl_reg_read(const jxcl_regfile_t *r, uint32_t i) {
    if (i < REG_COUNT) return r->gpr[i];
    if (i >= REG_PC && i <= REG_Q3) return r->special[i - REG_PC];
    return 0;
}
static inline void jxcl_reg_write(jxcl_regfile_t *r, uint32_t i, uint64_t v) {
    if (i < REG_COUNT) r->gpr[i] = v;
    else if (i >= REG_PC && i <= REG_Q3) r->special[i - REG_PC] = v;
}

/* ── Source Memory ──────────────────────────────────────────────────────── */
static const uint8_t jxcl_arecibo_source[JXCL_SOURCE_BYTES] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static inline uint8_t jxcl_source_bit(uint32_t idx) {
    if (idx >= JXCL_SOURCE_BITS) return 0;
    return (jxcl_arecibo_source[idx/8] >> (idx%8)) & 1;
}
static inline void jxcl_source_to_mem(uint64_t *dst) {
    uint32_t i, b;
    for (i = 0; i < 256; i++) {
        uint64_t w = 0;
        uint32_t base = i * 8;
        for (b = 0; b < 8 && (base+b) < JXCL_SOURCE_BYTES; b++)
            w |= ((uint64_t)jxcl_arecibo_source[base+b]) << (b*8);
        dst[i] = w;
    }
}

#endif /* JXCL_IMPL_H */
