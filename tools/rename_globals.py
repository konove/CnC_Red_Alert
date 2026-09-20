#!/usr/bin/env python3
"""Rename namespace-scope identifiers to accessor calls.

Used by docs/GLOBALS_PLAN.md to move a global into a subsystem: every use
of the old name becomes a call on the subsystem's accessor.

Skips comments, string and character literals, and any use preceded by
'.', '->' or '::', which is a member access or an already-qualified name.
It cannot tell an unqualified member use inside the owning class from the
global of the same name, so list those files under "exclude" and do them by
hand.

Usage: tools/rename_globals.py spec.json, where spec.json is

  {
    "root": "src/ra",
    "mapping": {"OutList": "TheNetwork().out_list()"},
    "skip": ["src/ra/externs.h"],
    "exclude": {"LastMessage": ["src/ra/radio.cc"]}
  }
"""
import json
import pathlib
import re
import sys

IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


def code_spans(text):
    """Yield (start, end) spans of the text that are real code."""
    i = 0
    n = len(text)
    start = 0
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            yield (start, i)
            j = text.find('\n', i)
            i = n if j < 0 else j + 1
            start = i
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            yield (start, i)
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            start = i
        elif c in '"\'':
            # Skip over the literal; keep it inside the span so offsets stay
            # simple, but mark it so replacements avoid it.
            yield (start, i)
            quote = c
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == '\n' and quote == '"':
                    break
                i += 1
            start = i
        else:
            i += 1
    yield (start, n)


def rewrite(text, mapping):
    out = []
    last = 0
    for s, e in code_spans(text):
        out.append(text[last:s])
        chunk = text[s:e]
        pieces = []
        pos = 0
        for m in IDENT.finditer(chunk):
            name = m.group(0)
            repl = mapping.get(name)
            if repl is None:
                continue
            before = chunk[:m.start()].rstrip()
            if before.endswith('.') or before.endswith('->') or before.endswith('::'):
                continue
            pieces.append((m.start(), m.end(), repl))
        for a, b, repl in pieces:
            out.append(chunk[pos:a])
            out.append(repl)
            pos = b
        out.append(chunk[pos:])
        last = e
    out.append(text[last:])
    return ''.join(out)


def main():
    spec = json.load(open(sys.argv[1]))
    mapping = spec['mapping']
    root = pathlib.Path(spec['root'])
    skip = set(spec.get('skip', []))
    exclude = {k: set(v) for k, v in spec.get('exclude', {}).items()}
    changed = []
    for p in sorted(root.rglob('*')):
        f = str(p)
        if p.suffix not in ('.cc', '.h') or f in skip:
            continue
        local = {k: v for k, v in mapping.items() if f not in exclude.get(k, ())}
        s = p.read_text()
        new = rewrite(s, local)
        if new != s:
            p.write_text(new)
            changed.append(f)
    print(len(changed))
    print('\n'.join(changed))


if __name__ == '__main__':
    main()
