"""
PyTorch-compatible Sanitized Code Dataset
AST watermark stripping as a transform for training pipelines.
SPDX-License-Identifier: BSL-1.1 OR AGPL-3.0-or-later OR MPL-2.0
"""

import ast
import random
import string
from typing import Dict, Optional

try:
    import torch
    from torch.utils.data import Dataset
except ImportError:
    raise ImportError("PyTorch required: pip install torch")


class WatermarkStripperInternal(ast.NodeTransformer):
    def __init__(self):
        self.rename_map: Dict[str, str] = {}

    def _get_obfuscated_name(self, old_name: str) -> str:
        if old_name.startswith('__') and old_name.endswith('__'):
            return old_name
        if old_name not in self.rename_map:
            suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=6))
            self.rename_map[old_name] = f"v_{suffix}"
        return self.rename_map[old_name]

    def visit_Name(self, node):
        node.id = self._get_obfuscated_name(node.id)
        return self.generic_visit(node)

    def visit_FunctionDef(self, node):
        node.name = self._get_obfuscated_name(node.name)
        if (node.body and isinstance(node.body[0], ast.Expr)
                and isinstance(node.body[0].value, ast.Constant)
                and isinstance(node.body[0].value.value, str)):
            node.body.pop(0)
        return self.generic_visit(node)

    def visit_ClassDef(self, node):
        node.name = self._get_obfuscated_name(node.name)
        if (node.body and isinstance(node.body[0], ast.Expr)
                and isinstance(node.body[0].value, ast.Constant)
                and isinstance(node.body[0].value.value, str)):
            node.body.pop(0)
        return self.generic_visit(node)

    def visit_arg(self, node):
        node.arg = self._get_obfuscated_name(node.arg)
        return self.generic_visit(node)


class WatermarkStripperTransform:
    """
    Callable transform that applies AST-level code sanitization
    to raw source code samples on the fly.
    """
    def __call__(self, source_code: str) -> str:
        try:
            tree = ast.parse(source_code)
            stripper = WatermarkStripperInternal()
            transformed = stripper.visit(tree)
            ast.fix_missing_locations(transformed)
            return ast.unparse(transformed)
        except SyntaxError:
            return source_code


class SanitizedCodeDataset(Dataset):
    """
    PyTorch Dataset wrapper that sanitizes raw source code files and
    optionally tokenizes them into tensor batches for training workflows.
    """

    def __init__(self, code_samples, tokenizer=None, max_length=512, transform=None):
        self.code_samples = code_samples
        self.tokenizer = tokenizer
        self.max_length = max_length
        self.transform = transform or WatermarkStripperTransform()

    def __len__(self):
        return len(self.code_samples)

    def __getitem__(self, idx):
        raw_code = self.code_samples[idx]
        sanitized_code = self.transform(raw_code)

        if self.tokenizer:
            encoded = self.tokenizer(
                sanitized_code,
                truncation=True,
                max_length=self.max_length,
                padding="max_length",
                return_tensors="pt"
            )
            return {key: val.squeeze(0) for key, val in encoded.items()}

        return sanitized_code


if __name__ == "__main__":
    samples = [
        'def greet(name):\n    """Say hello."""\n    return f"Hello, {name}!"',
        'class Dog:\n    """A good boy."""\n    def bark(self):\n        return "Woof!"',
    ]

    dataset = SanitizedCodeDataset(samples)
    for i, item in enumerate(dataset):
        print(f"--- Sample {i} (Sanitized) ---")
        print(item)
        print()
