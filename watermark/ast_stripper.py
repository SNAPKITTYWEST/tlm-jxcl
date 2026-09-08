"""
Deterministic AST Watermark Stripper
Renames identifiers via MD5-based hashes, removes docstrings.
SPDX-License-Identifier: BSL-1.1 OR AGPL-3.0-or-later OR MPL-2.0
"""

import ast
import hashlib
from typing import Dict, Set


class WatermarkStripper(ast.NodeTransformer):
    """
    AST-based watermark / fingerprint stripper + light name obfuscator.
    - Renames user-defined functions, classes, and arguments
    - Removes leading docstrings from functions and classes
    - Preserves builtins, dunders, and imports
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


def sanitize_code(source_code: str) -> str:
    tree = ast.parse(source_code)
    stripper = WatermarkStripper()
    transformed = stripper.visit(tree)
    ast.fix_missing_locations(transformed)
    return ast.unparse(transformed)


if __name__ == "__main__":
    sample = '''
def calculate_sum(a, b):
    """This is a watermark docstring."""
    return a + b

class Calculator:
    """Class-level watermark."""
    def multiply(self, x, y):
        return x * y

result = calculate_sum(3, 4)
print(result)
'''

    print("=== ORIGINAL ===")
    print(sample)
    print("\n=== STRIPPED / OBFUSCATED ===")
    print(sanitize_code(sample))
