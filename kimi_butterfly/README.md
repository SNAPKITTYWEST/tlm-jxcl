# Kimi Butterfly Triple-Lock Forge V2

Sovereign source-to-SASS research stack derived from the uploaded Kimi K3 / warp-decode structure.

Layers:
- compiler: AST, typed IR, optimization, register allocation, P4.5 and SASS emission
- circuits: recursively compressed hardware factors
- kernel: kernel-space memory/context/interrupt abstractions
- sass: annotated target instruction layer
- cuda: three CUDA kernels bound by ThreadPiper
- p45: custom ISA
- runtime: no_std Rust ThreadPiper scheduler
- proof: Why3 verification contracts
- rtl: open fabric abstraction
- audit: cryptographic state-event chain

This is a research implementation. Hardware-specific claims are modeled as target profiles, not claims that one commercial GPU implements every feature simultaneously.
