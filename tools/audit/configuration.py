"""Strict configuration edits: a formatting change must never skip a matrix setting."""
import re


def set_define(text, name, value):
    pattern = rf'^(\s*#\s*define\s+{re.escape(name)}\s+)\d+([^\n]*)$'
    changed, count = re.subn(pattern, lambda m: m[1] + str(value) + m[2], text, flags=re.M)
    if count != 1:
        raise ValueError(f'{name}: expected exactly one active numeric definition, got {count}')
    return changed


def effective_define(text, name):
    matches = re.findall(rf'^#define\s+{re.escape(name)}\s+(\d+)\s*$', text, re.M)
    if len(matches) != 1:
        raise ValueError(f'{name}: compiler did not report one effective definition')
    return int(matches[0])
