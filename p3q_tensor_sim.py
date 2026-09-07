#!/usr/bin/env python3
"""
Tensor Network Simulator for P3Q Grover Circuits
Uses Matrix Product States (MPS) for 1D circuit geometry
"""

import numpy as np
from typing import List, Tuple, Dict, Optional
from dataclasses import dataclass

# ──────────────────────────────────────────────
# Tensor Network Primitives
# ──────────────────────────────────────────────

@dataclass
class Tensor:
    """Tensor with named indices for contraction tracking."""
    data: np.ndarray
    indices: List[str]

    def shape(self) -> Tuple[int, ...]:
        return self.data.shape

    def contract(self, other: 'Tensor', idx_pairs: List[Tuple[str, str]]) -> 'Tensor':
        """Contract this tensor with another over matching indices."""
        self_idx = self.indices
        other_idx = other.indices

        contracted = set()
        for a, b in idx_pairs:
            contracted.add(a)
            contracted.add(b)

        free_self = [i for i in self_idx if i not in contracted]
        free_other = [i for i in other_idx if i not in contracted]

        label_map = {}
        next_char = ord('a')
        for idx in free_self + free_other + list(contracted):
            if idx not in label_map:
                label_map[idx] = chr(next_char)
                next_char += 1

        self_labels = ''.join(label_map[i] for i in self_idx)
        other_labels = ''.join(label_map[i] for i in other_idx)
        out_labels = ''.join(label_map[i] for i in free_self + free_other)

        einsum_str = f"{self_labels},{other_labels}->{out_labels}"
        result_data = np.einsum(einsum_str, self.data, other.data, optimize='greedy')

        return Tensor(result_data, free_self + free_other)

# ──────────────────────────────────────────────
# Quantum Gates as Tensors
# ──────────────────────────────────────────────

class GateTensor:
    """Precomputed gate tensors."""

    I = Tensor(np.eye(2, dtype=complex), ['in', 'out'])
    X = Tensor(np.array([[0,1],[1,0]], dtype=complex), ['in', 'out'])
    Z = Tensor(np.array([[1,0],[0,-1]], dtype=complex), ['in', 'out'])
    H = Tensor(np.array([[1,1],[1,-1]], dtype=complex) / np.sqrt(2), ['in', 'out'])

    @staticmethod
    def CX() -> Tensor:
        data = np.zeros((2,2,2,2), dtype=complex)
        data[0,0,0,0] = 1
        data[0,1,0,1] = 1
        data[1,0,1,1] = 1
        data[1,1,1,0] = 1
        return Tensor(data, ['c_in', 't_in', 'c_out', 't_out'])

    @staticmethod
    def CZ() -> Tensor:
        data = np.zeros((2,2,2,2), dtype=complex)
        data[0,0,0,0] = 1
        data[0,1,0,1] = 1
        data[1,0,1,0] = 1
        data[1,1,1,1] = -1
        return Tensor(data, ['c_in', 't_in', 'c_out', 't_out'])

    @staticmethod
    def CCX() -> Tensor:
        data = np.zeros((2,2,2, 2,2,2), dtype=complex)
        for c1 in [0,1]:
            for c2 in [0,1]:
                for t in [0,1]:
                    if c1 == 1 and c2 == 1:
                        data[c1,c2,t, c1,c2,1-t] = 1
                    else:
                        data[c1,c2,t, c1,c2,t] = 1
        return Tensor(data, ['c1_in','c2_in','t_in', 'c1_out','c2_out','t_out'])

# ──────────────────────────────────────────────
# AES Tensor Blocks
# ──────────────────────────────────────────────

class AESTensorBlocks:
    """Concrete tensor representations for AES-128 finite field operations."""

    @staticmethod
    def xtime_gate_tensor() -> Tensor:
        """
        Exact 8x8 linear transformation matrix tensor for GF(2^8) xtime.
        """
        data = np.zeros((2,)*16, dtype=complex)
        for val in range(256):
            msb = (val >> 7) & 1
            shifted = ((val << 1) & 0xFF)
            res = (shifted ^ 0x1B) if msb else shifted

            in_bits = tuple((val >> i) & 1 for i in range(7, -1, -1))
            out_bits = tuple((res >> i) & 1 for i in range(7, -1, -1))
            data[in_bits + out_bits] = 1.0

        in_indices = [f'x_in_{i}' for i in range(8)]
        out_indices = [f'x_out_{i}' for i in range(8)]
        return Tensor(data, in_indices + out_indices)

    @staticmethod
    def mix_column_tensor() -> Tensor:
        """
        32-qubit column tensor block representing MixColumns linear diffusion.
        """
        # Placeholder for full 32-bit MixColumns tensor
        pass

# ──────────────────────────────────────────────
# MPS State Representation
# ──────────────────────────────────────────────

class MPSState:
    """Matrix Product State for 1D quantum systems."""

    def __init__(self, num_qubits: int, max_bond: int = 256):
        self.num_qubits = num_qubits
        self.max_bond = max_bond
        self.tensors: List[Tensor] = []

        for i in range(num_qubits):
            t = Tensor(np.array([1.0, 0.0], dtype=complex), [f'q{i}', f'b{i}'])
            if i > 0:
                t.indices[1] = f'b{i-1}'
            if i < num_qubits - 1:
                t.indices.append(f'b{i}')
            self.tensors.append(t)

    def apply_single(self, qubit: int, gate: Tensor):
        """Apply single-qubit gate to MPS."""
        t = self.tensors[qubit]
        contracted = gate.contract(t, [('in', 'q'+str(qubit))])
        self.tensors[qubit] = contracted

    def measure(self, qubit: int) -> Tuple[int, float]:
        """Measure qubit in computational basis."""
        t = self.tensors[qubit]
        probs = np.abs(t.data)**2
        probs = probs / np.sum(probs)
        outcome = np.random.choice(2, p=probs)
        return outcome, probs[outcome]

# ──────────────────────────────────────────────
# Grover Tensor Network
# ──────────────────────────────────────────────

class GroverTensorNetwork:
    """Tensor network representation of Grover iteration."""

    def __init__(self, num_key_qubits: int = 32,
                 num_state_qubits: int = 32,
                 num_ancilla: int = 64):
        self.n_key = num_key_qubits
        self.n_state = num_state_qubits
        self.n_anc = num_ancilla
        self.total = num_key_qubits + num_state_qubits + num_ancilla
        self.mps = MPSState(self.total, max_bond=128)

    def hadamard_layer(self):
        """Apply H to all key qubits."""
        for i in range(self.n_key):
            self.mps.apply_single(i, GateTensor.H)

    def oracle(self, known_pt: bytes, target_ct: bytes):
        """Apply Grover oracle for 4-round AES."""
        # Placeholder for full reversible AES oracle
        pass

    def diffusion(self):
        """Diffusion operator: H X CZ X H"""
        for i in range(self.n_key):
            self.mps.apply_single(i, GateTensor.H)
            self.mps.apply_single(i, GateTensor.X)
        for i in range(self.n_key - 1):
            self.mps.apply_two_qubit(i, i+1, GateTensor.CZ())
        for i in range(self.n_key):
            self.mps.apply_single(i, GateTensor.X)
            self.mps.apply_single(i, GateTensor.H)

    def grover_iteration(self, known_pt: bytes, target_ct: bytes):
        self.oracle(known_pt, target_ct)
        self.diffusion()

    def run(self, iterations: int, known_pt: bytes, target_ct: bytes) -> Dict[int, float]:
        """Run Grover and return measurement statistics."""
        self.hadamard_layer()

        for _ in range(iterations):
            self.grover_iteration(known_pt, target_ct)

        counts = {}
        for _ in range(1000):
            outcome = 0
            for i in range(self.n_key):
                bit, _ = self.mps.measure(i)
                outcome = (outcome << 1) | bit
            counts[outcome] = counts.get(outcome, 0) + 1

        return counts

# ──────────────────────────────────────────────
# Verification
# ──────────────────────────────────────────────

def verify_xtime():
    """Verify xtime tensor matches scalar xtime."""
    def xtime_scalar(b: int) -> int:
        shifted = (b << 1) & 0xFF
        if b & 0x80:
            return shifted ^ 0x1B
        return shifted

    tensor = AESTensorBlocks.xtime_gate_tensor()

    for val in range(256):
        expected = xtime_scalar(val)
        in_bits = tuple((val >> i) & 1 for i in range(7, -1, -1))
        out_bits = tuple((expected >> i) & 1 for i in range(7, -1, -1))

        if tensor.data[in_bits + out_bits] != 1.0:
            print(f"FAIL: xtime({val:#04x}) = {expected:#04x}")
            return False

    print("xtime tensor verification PASSED")
    return True

def verify_mixcolumn():
    """Verify MixColumns against AES test vector."""
    def xtime_scalar(b: int) -> int:
        shifted = (b << 1) & 0xFF
        if b & 0x80:
            return shifted ^ 0x1B
        return shifted

    def mix_column_scalar(a0, a1, a2, a3):
        t = a0 ^ a1 ^ a2 ^ a3
        x0 = xtime_scalar(a0 ^ a1)
        x1 = xtime_scalar(a1 ^ a2)
        x2 = xtime_scalar(a2 ^ a3)
        x3 = xtime_scalar(a3 ^ a0)
        return (a0 ^ t ^ x0, a1 ^ t ^ x1, a2 ^ t ^ x2, a3 ^ t ^ x3)

    y0, y1, y2, y3 = mix_column_scalar(0xD4, 0xBF, 0x5D, 0x30)

    if (y0, y1, y2, y3) != (0x04, 0x66, 0x81, 0xE5):
        print(f"FAIL: MixColumns(D4,BF,5D,30) = ({y0:#04x},{y1:#04x},{y2:#04x},{y3:#04x})")
        return False

    print("MixColumns verification PASSED")
    return True

if __name__ == "__main__":
    verify_xtime()
    verify_mixcolumn()
