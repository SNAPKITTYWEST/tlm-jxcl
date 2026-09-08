"""
Random AST Watermark Stripper (NLOPRA variant)
Randomized identifier renaming via alphanumeric suffixes.
SPDX-License-Identifier: BSL-1.1 OR AGPL-3.0-or-later OR MPL-2.0
"""

import ast
import random
import string
from typing import Dict


class WatermarkStripper(ast.NodeTransformer):
    """
    AST transformer that strips docstrings, comments (via AST omission),
    and randomizes identifiers to break LLM statistical watermarks.
    """

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


def sanitize_code(source_code: str) -> str:
    tree = ast.parse(source_code)
    stripper = WatermarkStripper()
    transformed_tree = stripper.visit(tree)
    ast.fix_missing_locations(transformed_tree)
    return ast.unparse(transformed_tree)


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
    print("\n=== STRIPPED / RANDOMIZED ===")
    print(sanitize_code(sample))
