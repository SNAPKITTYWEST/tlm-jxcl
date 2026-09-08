"""
TLM Stealth-Mode Obfuscator
AST Stripping -> Fibonacci Braid -> Salted Unicode Rotation -> RSA Envelope -> VDF Time-Lock -> Shamir's Dead Man's Switch

Architecture: P4 ALGOL -> JXCL ISA -> P3 VHDL -> P2 GF(2^8) -> P1 BOOLEAN
SPDX-License-Identifier: BSL-1.1 OR AGPL-3.0-or-later OR MPL-2.0
"""

import ast
import hashlib
import base64
import struct
import zlib
import random
import time
from typing import Dict, Set, Tuple, List

PRIME = 2**31 - 1


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 1: AST Watermark Stripper
# ═══════════════════════════════════════════════════════════════════════════

class WatermarkStripper(ast.NodeTransformer):
    """
    AST-based watermark / fingerprint stripper + light name obfuscator.
    Renames user-defined functions, classes, and arguments.
    Removes leading docstrings from functions and classes.
    Preserves builtins, dunders, and imports.
    """

    def __init__(self, prefix: str = "_w"):
        self.prefix = prefix
        self.name_map: Dict[str, str] = {}
        self.counter = 0
        self.protected: Set[str] = {
            "print", "len", "range", "list", "dict", "set", "tuple",
            "str", "int", "float", "bool", "None", "True", "False",
            "self", "cls", "super", "object", "type", "Exception",
            "__init__", "__name__", "__main__", "__file__", "__doc__",
            "__all__", "__dict__", "__class__", "__module__",
        }

    def _get_obfuscated_name(self, original: str) -> str:
        if original in self.protected or original.startswith("__"):
            return original
        if original not in self.name_map:
            digest = hashlib.md5(original.encode()).hexdigest()[:6]
            self.name_map[original] = f"{self.prefix}{self.counter}_{digest}"
            self.counter += 1
        return self.name_map[original]

    def visit_FunctionDef(self, node: ast.FunctionDef):
        node.name = self._get_obfuscated_name(node.name)
        if (node.body and isinstance(node.body[0], ast.Expr)
                and isinstance(node.body[0].value, ast.Constant)
                and isinstance(node.body[0].value.value, str)):
            node.body.pop(0)
        return self.generic_visit(node)

    def visit_ClassDef(self, node: ast.ClassDef):
        node.name = self._get_obfuscated_name(node.name)
        if (node.body and isinstance(node.body[0], ast.Expr)
                and isinstance(node.body[0].value, ast.Constant)
                and isinstance(node.body[0].value.value, str)):
            node.body.pop(0)
        return self.generic_visit(node)

    def visit_arg(self, node: ast.arg):
        node.arg = self._get_obfuscated_name(node.arg)
        return self.generic_visit(node)

    def visit_Name(self, node: ast.Name):
        if isinstance(node.ctx, (ast.Load, ast.Store, ast.Del)):
            if node.id in self.name_map:
                node.id = self.name_map[node.id]
        return node

    def visit_Attribute(self, node: ast.Attribute):
        self.generic_visit(node)
        return node

    def strip(self, source_code: str) -> str:
        tree = ast.parse(source_code)
        transformed = self.visit(tree)
        ast.fix_missing_locations(transformed)
        return ast.unparse(transformed)


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 2: Lossless Fibonacci Braid
# ═══════════════════════════════════════════════════════════════════════════

def _fib(n: int) -> int:
    a, b = 0, 1
    for _ in range(n):
        a, b = b, a + b
    return a


def _substitute(text: str, kappa: int, decrypt: bool = False) -> str:
    shift = -kappa if decrypt else kappa
    return "".join(chr((ord(c) + shift) % 1114112) for c in text)


def _braid(text: str) -> str:
    left = [text[i] for i in range(0, len(text), 2)]
    right = [text[i] for i in range(1, len(text), 2)]
    output = []
    n = 1
    while left or right:
        len_l = _fib(n) % (len(left) + 1) if left else 0
        len_r = _fib(n + 1) % (len(right) + 1) if right else 0
        for _ in range(len_l):
            if left: output.append(left.pop())
        for _ in range(len_r):
            if right: output.append(right.pop())
        n += 1
    return "".join(output)


def _unbraid(braided: str, original_len: int) -> str:
    stream = list(braided)
    left, right = [], []
    cursor, n = 0, 1
    orig_l = original_len // 2
    orig_r = original_len - orig_l

    while cursor < len(stream):
        len_l = _fib(n) % (orig_l + 1)
        len_r = _fib(n + 1) % (orig_r + 1)
        for _ in range(len_l):
            if cursor < len(stream):
                left.append(stream[cursor]); cursor += 1
        for _ in range(len_r):
            if cursor < len(stream):
                right.append(stream[cursor]); cursor += 1
        n += 1

    left.reverse()
    right.reverse()
    result = []
    for i in range(max(len(left), len(right))):
        if i < len(left): result.append(left[i])
        if i < len(right): result.append(right[i])
    return "".join(result)


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 3: Stealth Braid with Key Envelope
# ═══════════════════════════════════════════════════════════════════════════

class StealthBraider:
    """
    Lossless Fibonacci Braid with Base64-encoded key envelope.
    Envelope format: [Version(B) | Kappa(B) | Length(I) | Checksum(I)]
    """

    VERSION = 1

    def encrypt(self, text: str) -> Tuple[str, str]:
        kappa = int(hashlib.sha256(text.encode()).hexdigest(), 16) % 256
        checksum = zlib.crc32(text.encode())
        original_len = len(text)

        substituted = _substitute(text, kappa)
        braided = _braid(substituted)

        envelope_bin = struct.pack(">BBII", self.VERSION, kappa, original_len, checksum)
        envelope_b64 = base64.b64encode(envelope_bin).decode()
        return braided, envelope_b64

    def decrypt(self, braided: str, envelope_b64: str) -> str:
        envelope_bin = base64.b64decode(envelope_b64)
        version, kappa, original_len, checksum = struct.unpack(">BBII", envelope_bin)

        restored = _unbraid(braided, original_len)
        final = _substitute(restored, kappa, decrypt=True)

        if zlib.crc32(final.encode()) != checksum:
            raise ValueError("Integrity check failed: Decrypted text is corrupted.")
        return final


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 4: RSA Encryption
# ═══════════════════════════════════════════════════════════════════════════

class RSAEncryptor:
    """
    RSA encryption of the key envelope for asymmetric key wrapping.
    Uses PKCS1_OAEP padding.
    """

    def __init__(self):
        self._pub = None
        self._priv = None

    def generate_keys(self):
        from cryptography.hazmat.primitives.asymmetric import rsa, padding
        from cryptography.hazmat.primitives import hashes
        self._priv = rsa.generate_private_key(65537, 2048)
        self._pub = self._priv.public_key()
        return self._pub, self._priv

    def set_public_key(self, pub):
        self._pub = pub

    def encrypt_envelope(self, envelope_b64: str) -> str:
        from cryptography.hazmat.primitives.asymmetric import padding
        from cryptography.hazmat.primitives import hashes
        encrypted = self._pub.encrypt(
            envelope_b64.encode(),
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None,
            )
        )
        return base64.b64encode(encrypted).decode()

    def decrypt_envelope(self, encrypted_b64: str) -> str:
        from cryptography.hazmat.primitives.asymmetric import padding
        from cryptography.hazmat.primitives import hashes
        decrypted = self._priv.decrypt(
            base64.b64decode(encrypted_b64),
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None,
            )
        )
        return decrypted.decode()


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 5: VDF Time-Lock
# ═══════════════════════════════════════════════════════════════════════════

class VDFTimeLock:
    """
    Wesolowski Verifiable Delay Function.
    y = g^(2^T) mod N via repeated squaring.
    """

    def __init__(self, difficulty: int = 10**5):
        self.T = difficulty
        from cryptography.hazmat.primitives.asymmetric import rsa
        self.N = rsa.generate_private_key(65537, 2048).public_key().public_numbers().n

    def lock(self, seed_bytes: bytes) -> Tuple[int, int]:
        g = int(hashlib.sha256(seed_bytes).hexdigest(), 16) % self.N
        y = g
        for _ in range(self.T):
            y = pow(y, 2, self.N)
        return g, y

    def verify(self, g: int, y: int) -> bool:
        current = g
        for _ in range(self.T):
            current = pow(current, 2, self.N)
        return current == y


# ═══════════════════════════════════════════════════════════════════════════
# LAYER 6: Shamir's Secret Sharing (Dead Man's Switch)
# ═══════════════════════════════════════════════════════════════════════════

class ShamirSecretSharing:
    """
    k-of-n threshold secret sharing over Z_p.
    Uses Lagrange interpolation for reconstruction.
    """

    def split(self, secret: int, k: int, n: int) -> List[Tuple[int, int]]:
        coeffs = [secret] + [random.randint(0, PRIME - 1) for _ in range(k - 1)]

        def eval_poly(x):
            res = 0
            for c in reversed(coeffs):
                res = (res * x + c) % PRIME
            return res

        return [(i, eval_poly(i)) for i in range(1, n + 1)]

    def recover(self, shards: List[Tuple[int, int]]) -> int:
        x_vals, y_vals = zip(*shards)
        total = 0
        for i in range(len(x_vals)):
            num, den = 1, 1
            for j in range(len(x_vals)):
                if i == j:
                    continue
                num = (num * -x_vals[j]) % PRIME
                den = (den * (x_vals[i] - x_vals[j])) % PRIME
            term = (y_vals[i] * num * pow(den, PRIME - 2, PRIME)) % PRIME
            total = (total + term) % PRIME
        return total


# ═══════════════════════════════════════════════════════════════════════════
# INTEGRATED PIPELINE: StealthWatermarkStripper
# ═══════════════════════════════════════════════════════════════════════════

class StealthWatermarkStripper:
    """
    Full pipeline:
    1. AST Strip (remove watermarks, rename identifiers)
    2. Fibonacci Braid + Unicode Rotation (lossless obfuscation)
    3. RSA Envelope (asymmetric key wrapping)
    4. VDF Time-Lock (sequential delay)
    5. Shamir's Secret Sharing (threshold dead man's switch)

    Output: (braided_text, rsa_encrypted_envelope, vdf_lock, shards)
    """

    def __init__(self, vdf_difficulty: int = 10**4):
        self.stripper = WatermarkStripper()
        self.braider = StealthBraider()
        self.rsa = RSAEncryptor()
        self.vdf = VDFTimeLock(difficulty=vdf_difficulty)
        self.sss = ShamirSecretSharing()

    def generate_keys(self):
        return self.rsa.generate_keys()

    def encrypt(self, source_code: str, k: int = 3, n: int = 5) -> dict:
        # 1. AST Strip
        stripped = self.stripper.strip(source_code)

        # 2. Braid
        braided, envelope_b64 = self.braider.encrypt(stripped)

        # 3. RSA-wrap the envelope
        rsa_envelope = self.rsa.encrypt_envelope(envelope_b64)

        # 4. VDF time-lock on the RSA envelope
        g, y = self.vdf.lock(rsa_envelope.encode())

        # 5. Shard the VDF seed g via Shamir
        shards = self.sss.split(g, k, n)

        return {
            "braided": braided,
            "rsa_envelope": rsa_envelope,
            "vdf_y": y,
            "vdf_T": self.vdf.T,
            "shards": shards,
            "k": k,
            "n": n,
            "original_len": len(stripped),
        }

    def decrypt(self, artifact: dict, shard_subset: List[Tuple[int, int]]) -> str:
        if len(shard_subset) < artifact["k"]:
            raise ValueError(f"Need {artifact['k']} shards, got {len(shard_subset)}")

        # 1. Reconstruct VDF seed from shards
        g = self.sss.recover(shard_subset)

        # 2. Verify VDF
        if not self.vdf.verify(g, artifact["vdf_y"]):
            raise ValueError("VDF verification failed")

        # 3. RSA decrypt the envelope
        envelope_b64 = self.rsa.decrypt_envelope(artifact["rsa_envelope"])

        # 4. Reverse braid
        return self.braider.decrypt(artifact["braided"], envelope_b64)


# ═══════════════════════════════════════════════════════════════════════════
# DEMO
# ═══════════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    source = '''def nuclear_codes():
    """Top secret launch codes"""
    return "12345"

class Commander:
    """Military command interface"""
    def authorize(self, code):
        return code == nuclear_codes()
'''

    print("=" * 60)
    print("TLM STEALTH-MODE OBFUSCATOR")
    print("=" * 60)
    print(f"\nOriginal ({len(source)} chars):")
    print(source)

    # Setup
    stripper = StealthWatermarkStripper(vdf_difficulty=100)
    pub, priv = stripper.generate_keys()

    # Encrypt: 3-of-5 threshold
    k, n = 3, 5
    print(f"\nEncrypting with {k}-of-{n} threshold...")
    artifact = stripper.encrypt(source, k=k, n=n)
    print(f"  Braided: {artifact['braided'][:60]}...")
    print(f"  Shards: {len(artifact['shards'])} generated")

    # Simulate 3 parties collaborating
    import random
    collaborating = random.sample(artifact["shards"], k)
    print(f"\n{k} shards collected. Attempting recovery...")

    try:
        recovered = stripper.decrypt(artifact, collaborating)
        print(f"\nRecovered ({len(recovered)} chars):")
        print(recovered)
        print(f"\nRound-trip integrity: {'PASS' if recovered.strip() == source.strip() else 'FAIL'}")
    except Exception as e:
        print(f"Error: {e}")
