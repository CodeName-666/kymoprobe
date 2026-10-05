"""Structural source check for this project's C99/C++11 subset.

Enforces at most one return, at function-body depth and as the last statement.
This is a coding-rule check, not a complete C++ parser or safety certification.
Checks source (including conditional branches), not third-party/preprocessed code.
"""
from __future__ import annotations
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SCOPE = ("src", "include", "lib/PlotterLib/src", "lib/PlotterLib/test", "lib/PlotterLib/examples",
         "examples", "test/native")
EXTENSIONS = {".c", ".h", ".cpp", ".ino", ".hpp"}
LEXEME = re.compile(
    r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    r'|[A-Za-z_]\w*|[^\s]'
)
CONTROL = {"if", "for", "while", "switch", "catch"}
QUALIFIERS = {"const", "override", "final", "noexcept"}

def violations(source: str) -> list[str]:
    # Keep positions/newlines so diagnostics still refer to original source.
    directives = re.compile(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", re.MULTILINE)
    errors = []
    for directive in directives.finditer(source):
        tokens = [m.group() for m in LEXEME.finditer(directive.group())]
        if "return" in tokens:
            errors.append("return in a preprocessor directive is forbidden")
    source = directives.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), source)
    tokens = [(m.group(), m.start()) for m in LEXEME.finditer(source)
              if not m.group().startswith(("//", "/*"))]
    parens = []
    pairs = {}
    braces = []
    for i, (token, position) in enumerate(tokens):
        if token == "(":
            parens.append(i)
        elif token == ")" and parens:
            pairs[i] = parens.pop()
        elif token == "{":
            end = i - 1
            while end >= 0 and tokens[end][0] in QUALIFIERS:
                end -= 1
            opening = pairs.get(end)
            name = tokens[opening - 1][0] if opening is not None and opening else ""
            function = None
            if name and name not in CONTROL:
                function = {"name": name, "depth": len(braces), "returns": []}
            braces.append(function)
        elif token == "return":
            owners = [entry for entry in braces if entry is not None]
            if owners:
                owners[-1]["returns"].append((i, len(braces) - 1, position))
            else:
                errors.append(f"line {source.count(chr(10), 0, position) + 1}: unclassified return")
        elif token == "}" and braces:
            function = braces.pop()
            if function is not None:
                returns = function["returns"]
                for return_index, depth, return_position in returns:
                    line = source.count("\n", 0, return_position) + 1
                    semicolon = return_index + 1
                    while semicolon < i and tokens[semicolon][0] != ";":
                        semicolon += 1
                    if len(returns) > 1 or depth != function["depth"] or semicolon != i - 1:
                        errors.append(f"line {line}: {function['name']}: return must be unique and final")
    return errors

def main() -> int:
    errors = []
    files = set()
    for relative in SCOPE:
        files.update(p for p in (ROOT / relative).rglob("*")
                     if p.suffix in EXTENSIONS and ".pio" not in p.parts)
    for path in sorted(files):
        for error in violations(path.read_text(encoding="utf-8")):
            errors.append(f"{path.relative_to(ROOT)}: {error}")
    for error in errors:
        print(error)
    if not errors:
        print(f"Single-return rule passed: {len(files)} active C/C++ source files")
    return int(bool(errors))

if __name__ == "__main__":
    sys.exit(main())
