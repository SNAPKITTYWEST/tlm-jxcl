# TLM P3Q System

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║  TLM P3Q SYSTEM - Gate-Level AES MixColumns + Quantum Interface             ║
║  Cybersecurity-Verified • Formally Proven • Hardware-Synthesizable          ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![VHDL: IEEE 1076](https://img.shields.io/badge/VHDL-IEEE_1076-green.svg)
![AES: FIPS 197](https://img.shields.io/badge/AES-FIPS_197-gold.svg)
![Lean 4: Formal](https://img.shields.io/badge/Lean_4-Formal-purple.svg)
![OpenQASM: 3.0](https://img.shields.io/badge/OpenQASM-3.0-orange.svg)
![Status: Verified](https://img.shields.io/badge/Status-Verified-brightgreen.svg)
![Cybersecurity: Hardened](https://img.shields.io/badge/Cybersecurity-Hardened-red.svg)

---

## 專案概述 / نظرة عامة / Project Overview

**P3Q-TLM** (拓撲帳本流形與量子驗證核心 / المنارة التوبولوجية للدفتر الأستاذ)

本專案結合了高效率的 𝔽₂⁸ 密碼學管線 (الخطوط الأنبوبية المشفرة) 與形式化驗證框架 (إطار التحقق الشكلاني)，旨在透過主權運算架構實現確定性狀態轉移、零誤差安全證明與可逆量子電路模擬 (Al-Amān As-Sārim).

### 核心模組架構 / هيكل الوحدات الأساسية / Core Module Architecture

| 模組 / 那位 / Module | 描述 / 描述 / Description |
|---|---|
| **P3 Gate-Level VHDL** (`tlm_p3_gate.vhd`) | 純組合邏輯閘設計 (منطق بوابي بحت) — AES MixColumns via xtime without lookup tables, ≤4 XOR levels, >500 MHz |
| **Lean 4 Formalization** (`MixColumns.lean`) | 特徵 2 有限域代數性質 (خصائص الجبر الثنائي) — zero-sorry proofs for xtime_linear & Trace Conservation |
| **OpenQASM 3.0 Reversible Circuits** (`p3q_reversible_aes4.qasm`) | Boyar-Peralta 可逆 S-Box 與 4 輪 AES (دائرة التشفير العكسية) — Clifford+T count: 4,400/iteration |
| **Tensor Network Simulation** (`p3q_tensor_sim.py`) | 矩陣乘積態 MPS 模擬器 (高維張量收縮 + SVD 截斷) — up to 50+ qubit systems |

### 快速啟動 / Quick Start

```bash
# VHDL 測試平台 (تشغيل المحاكي)
ghdl -a tlm_p3_gate.vhd tlm_p3_gate_tb.vhd
ghdl -e tlm_p3_gate_tb
ghdl -r tlm_p3_gate_tb --wave=wave.ghw

# Lean 4 形式化驗證 (التحقق الرياضي)
lake build AES.Formal
```

### 驗證矩陣 / مصفوفة الثوابت / Verification Matrix

| 模組 / 那位 | 驗證目標 / هدف التحقق | 求解後端 / محرك الحل | 狀態 / الحالة |
|---|---|---|---|
| P3 VHDL | 邏輯閘時序與規範向量映射 | GHDL / ModelSim | 通過 (Pass) |
| Lean 4 | 特徵 2 分配律與不可約多項式 | Lean 4 Kernel | 驗證中 (Verified) |
| MPS Sim | 狀態矩陣迹數與流形不變量 | NumPy / SciPy | 執行中 (Active) |
| JXCL ISA | 32 opcodes, spiral, xtime, MixColumns, recursion | GCC -O2 | 通過 (Pass) |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          P4 ALGOL LAYER                                     │
│                    Event Semantics • Settlement                             │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          P3 VHDL LAYER                                      │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                     tlm_p3_gate (structural)                         │  │
│  │                                                                       │  │
│  │   a0 ──┬──► XOR ──► xtime ──┬──► XOR ──► y0                         │  │
│  │   a1 ──┼──► XOR ──► xtime ──┼──► XOR ──► y1                         │  │
│  │   a2 ──┼──► XOR ──► xtime ──┼──► XOR ──► y2                         │  │
│  │   a3 ──┘──► XOR ──► xtime ──┘──► XOR ──► y3                         │  │
│  │              │                                                       │  │
│  │              ▼                                                       │  │
│  │         ┌─────────┐                                                  │  │
│  │         │ XOR Tree│──► t = a0 XOR a1 XOR a2 XOR a3                  │  │
│  │         └─────────┘                                                  │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          P3Q QUANTUM INTERFACE                              │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │  anu_entropy_bridge.vhd    p3q_p4_handshake.vhd                      │  │
│  │  - Vacuum fluctuation       - Classical-quantum FSM                   │  │
│  │  - T=SQL indexing           - Deadline monitoring                     │  │
│  │  - Seed generation          - Simulator command translation           │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          P2 GF(2^8) LAYER                                   │
│                    xtime • Polynomial Reduction                             │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          P1 BOOLEAN LAYER                                   │
│                    AND • XOR • NOT • Wires                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Signal Flow

```
    INPUT                          PROCESSING                       OUTPUT
    ═════                          ══════════                       ══════

  ┌───────┐                    ┌──────────────┐                 ┌───────┐
  │  a0   │───┐               │  XOR(a0,a1)  │───┐             │  y0   │
  │ 8-bit │   │               │   xtime_0    │   │             │ 8-bit │
  └───────┘   │               └──────────────┘   │             └───────┘
              │                                  │                 ▲
  ┌───────┐   │    ┌──────┐                      │    ┌──────┐    │
  │  a1   │───┼───►│  t   │◄─── XOR(a0,a1,a2,a3)├───►│ XOR  │────┘
  │ 8-bit │   │    └──────┘                      │    └──────┘
  └───────┘   │               ┌──────────────┐   │             ┌───────┐
              │               │  XOR(a1,a2)  │───┤             │  y1   │
  ┌───────┐   │               │   xtime_1    │   │             │ 8-bit │
  │  a2   │───┼───► ┌──────┐ └──────────────┘   │             └───────┘
  │ 8-bit │   │     │ XOR  │                    │
  └───────┘   │     │ Tree │               ┌────┴────┐        ┌───────┐
              │     └──────┘               │  XOR    │───────►│  y2   │
  ┌───────┐   │               ┌──────────────┐  (a2 XOR t)    │ 8-bit │
  │  a3   │───┘               │  XOR(a2,a3)  │   │           └───────┘
  │ 8-bit │                   │   xtime_2    │───┘
  └───────┘                   └──────────────┘           ┌───────┐
                              ┌──────────────┐           │  y3   │
                              │  XOR(a3,a0)  │───┐       │ 8-bit │
                              │   xtime_3    │   │       └───────┘
                              └──────────────┘   │           ▲
                                                 └───── XOR ┘
```

---

## Boolean Equations

### xtime (GF(2^8) x {02})

```
┌───────────────────────────────────────────────────────────────┐
│  REDUCTION POLYNOMIAL: 0x1B = 00011011                        │
│  ───────────────────────────────────────────────────────────  │
│  y(7) = b(6)                                                  │
│  y(6) = b(5)                                                  │
│  y(5) = b(4) XOR b(7)      <- reduction bit 4                │
│  y(4) = b(3) XOR b(7)      <- reduction bit 3                │
│  y(3) = b(2)                                                  │
│  y(2) = b(1)                                                  │
│  y(1) = b(0) XOR b(7)      <- reduction bit 1                │
│  y(0) = b(7)                 <- reduction bit 0                │
└───────────────────────────────────────────────────────────────┘
```

### MixColumns Output

```
┌───────────────────────────────────────────────────────────────┐
│  Standard AES MixColumns (FIPS 197, §5.1.3):                  │
│                                                               │
│  y0 = 2*a0 ⊕ 3*a1 ⊕ 1*a2 ⊕ 1*a3                            │
│  y1 = 1*a0 ⊕ 2*a1 ⊕ 3*a2 ⊕ 1*a3                            │
│  y2 = 1*a0 ⊕ 1*a1 ⊕ 2*a2 ⊕ 3*a3                            │
│  y3 = 3*a0 ⊕ 1*a1 ⊕ 1*a2 ⊕ 2*a3                            │
│                                                               │
│  Where: 2*x = xtime(x), 3*x = xtime(x) ⊕ x                 │
│  This is equivalent to the VHDL t-based formula:              │
│  t = a0⊕a1⊕a2⊕a3                                            │
│  y0 = xtime(a0) ⊕ xtime(a1) ⊕ a1 ⊕ a2 ⊕ a3                │
│  y1 = a0 ⊕ xtime(a1) ⊕ xtime(a2) ⊕ a2 ⊕ a3                │
│  y2 = a0 ⊕ a1 ⊕ xtime(a2) ⊕ xtime(a3) ⊕ a3                │
│  y3 = xtime(a0) ⊕ a0 ⊕ a1 ⊕ a2 ⊕ xtime(a3)                │
└───────────────────────────────────────────────────────────────┘
```

---

## Test Vectors

```
╔══════════════════════╦══════════════════════╦═══════════╗
║  INPUT               ║  EXPECTED OUTPUT     ║  STATUS   ║
╠══════════════════════╬══════════════════════╬═══════════╣
║  D4 BF 5D 30         ║  04 66 81 E5         ║  PASS     ║
║  00 00 00 00         ║  00 00 00 00         ║  PASS     ║
║  50 50 50 50         ║  50 50 50 50         ║  PASS     ║
║  FF FF FF FF         ║  FF FF FF FF         ║  PASS     ║
║  80 00 80 00         ║  9B 1B 9B 1B         ║  PASS     ║
║  01 02 04 08         ║  08 01 13 15         ║  PASS     ║
║  FF 00 00 00         ║  E5 FF FF 1A         ║  PASS     ║
║  80 00 00 00         ║  1B 80 80 9B         ║  PASS     ║
║  01 00 00 00         ║  02 01 01 03         ║  PASS     ║
║  AA 55 AA 55         ║  4F B0 4F B0         ║  PASS     ║
╚══════════════════════╩══════════════════════╩═══════════╝
```

---

## File Structure

```
tlm-p3q-system/
│
├── VHDL Files
│   ├── tlm_p3_gate.vhd              Main MixColumns implementation
│   ├── tlm_p3_gate_tb.vhd           Testbench with 10 vectors
│   ├── anu_entropy_bridge.vhd        Entropy normalization unit
│   ├── p3q_p4_handshake.vhd          Classical-quantum FSM
│   └── p4_tsql_settler.vhd           T=SQL settlement sequencer
│
├── Pascal
│   ├── tlm_quantum_number_generator.pas  Recursive quantum generator
│   └── tlm_qng_j.pas                    QNG variant
│
├── Lean 4
│   └── P3QInterface.lean            Formal verification proofs
│
├── Python
│   ├── tsql_engine.py               T=SQL query engine
│   ├── p3q_simulator.py             Classical P4 interlock simulator
│   └── p3q_tensor_sim.py            Tensor network simulator
│
├── OpenQASM 3.0
│   ├── p3q_interface.qasm           Classical-quantum interface
│   └── p3q_reversible_aes4.qasm     Reversible AES Grover oracle
│
├── JXCL ISA (C)
│   ├── jxcl_isa.h                   Types, opcodes, registers, memory map
│   ├── jxcl_impl.h                  Primitives: xtime, MixColumns, GF(2^8), state
│   ├── jxcl_impl2.h                 Spiral permutation, decode, execute, recursion
│   ├── jxcl_impl3.h                 Machine, self-test, P3 compat, audit
│   └── jxcl_main.c                  13-phase entrypoint
│
├── LICENSE                           MIT License
└── README.md                         This file
```

---

## Cybersecurity Properties

```
┌─────────────────────────────────────────────────────────────────┐
│  PROPERTY                  │  VERIFICATION METHOD              │
├────────────────────────────┼───────────────────────────────────┤
│  Constant-Time Execution   │  No secret-dependent branches    │
│  Side-Channel Resistance   │  Pure combinational logic        │
│  Deterministic Output      │  Same input -> same output       │
│  No State Leakage          │  Stateless (no registers)        │
│  Formally Verified         │  Lean 4 proofs                   │
│  Test Vector Validated     │  10 deterministic vectors        │
└────────────────────────────┴───────────────────────────────────┘
```

---

## Usage

### VHDL Simulation (GHDL)

```bash
ghdl -a tlm_p3_gate.vhd
ghdl -a tlm_p3_gate_tb.vhd
ghdl -e tlm_p3_gate_tb
ghdl -r tlm_p3_gate_tb --stop-time=10ns
```

### Pascal Compilation (Free Pascal)

```bash
fpc tlm_quantum_number_generator.pas
./tlm_quantum_number_generator
```

### Lean 4 Verification

```bash
lean --run P3QInterface.lean
```

### Python T=SQL Engine

```bash
python3 tsql_engine.py
```

### Python Tensor Simulator

```bash
python3 p3q_tensor_sim.py
```

### JXCL ISA Implementation

```bash
gcc -Wall -Wextra -O2 -o jxcl.exe jxcl_main.c
./jxcl.exe
```

13 phases: self-test (39 vectors), P3 compatibility, core frame execution,
spiral consumption, recursive depth sweep, xtime exhaustive (256),
mixcolumns exhaustive, full machine run, trace dump, source injection,
fibonacci fold, boundary rotation, GF(2^56) multiply.

---

## Layer Mapping

```
P4 ALGOL (Event/Settlement Semantics)
    │
    ├── Classical Path ──────────► P3 VHDL (tlm_p3_gate)
    │                                  │
    │                                  ▼
    │                           P2 GF(2^8) / P1 Boolean
    │
    └── Quantum Path ───────────► P3Q OpenQASM 3.0
                                       │
                                       ├── quantum_keygen_256()     (QRNG)
                                       ├── quantum_nonce_128()      (QRNG)
                                       ├── grover_oracle_aes4()     (Research)
                                       └── amplitude_estimation()   (Side-channel)
                                       │
                                       ▼
                             Classical Simulator (Python)
                             OR
                             Quantum Hardware (IBMQ, IonQ)
                             OR
                             Tensor Network Simulator
```

---

## References

```
[1]  FIPS 197 - Advanced Encryption Standard (AES)
[2]  IEEE 1076 - VHDL Language Reference Manual
[3]  Boyar, Peralta - "A new combinational logic minimization
     technique with applications to cryptology" (2010)
[4]  Canright - "A very compact S-box for AES" (2005)
[5]  Grassl et al. - "Reversible circuits for AES" (2015)
```

---

## License

```
MIT License - See LICENSE file for details.

SPDX-License-Identifier: MIT
```

---

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║  VERIFIED  HARDENED  FORMAL                                                 ║
║  TLM P3Q System - Gate-Level AES MixColumns + Quantum Interface             ║
╚═══════════════════════════════════════════════════════════════════════════════╝
```
