#!/usr/bin/env python3
import re
import sys
import os
from pathlib import Path

def to_snake_case(name):
    s1 = re.sub(r'(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s1)
    return s2.lower()

def scan_identifiers(files):
    identifiers = set()
    pattern = re.compile(r'\b[a-zA-Z_][a-zA-Z0-9_]*\b')
    for file in files:
        with open(file, 'r', encoding='utf-8') as f:
            for line in f:
                for match in pattern.findall(line):
                    if re.search(r'[a-z][A-Z]', match):  # likely camelCase
                        identifiers.add(match)
    return sorted(identifiers)

def apply_replacements(files, replacements):
    for file in files:
        with open(file, 'r', encoding='utf-8') as f:
            content = f.read()
        for orig, new in replacements.items():
            content = re.sub(rf'\b{re.escape(orig)}\b', new, content)
        with open(file, 'w', encoding='utf-8') as f:
            f.write(content)

def main():
    root = Path('.')
    files = list(root.rglob('*.c')) + list(root.rglob('*.h'))

    print(f"Scanning {len(files)} source files...\n")
    identifiers = scan_identifiers(files)

    replacements = {}
    for ident in identifiers:
        snake = to_snake_case(ident)
        if snake != ident:
            print(f"{ident} -> {snake}")
            replacements[ident] = snake

    confirm = input("\nApply these replacements? [y/N]: ")
    if confirm.lower() == 'y':
        apply_replacements(files, replacements)
        print("✔ Done.")
    else:
        print("✖ Aborted.")

if __name__ == "__main__":
    main()
