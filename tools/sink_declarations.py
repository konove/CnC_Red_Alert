#!/usr/bin/env python3
"""Sink uninitialized local declarations to their first unconditional assignment.

Built for enabling cppcoreguidelines-init-variables (docs/INIT_VARIABLES_PLAN.md).
Input is a JSON list of the check's findings, one object per declared name:
{"file": path, "off": byte offset of the name, "name": ..., "fix": " = 0" | null},
as merged from `clang-tidy --export-fixes` YAML (see the plan for the recipe).

For each site the declaration statement is located. If the first mention of the
name after it is a plain `name = expr;` statement in the same block, the
declaration moves there as `Type name = expr;`. If that statement sits in a
nested block and no later mention leaves the block, it moves into the block.
If every later mention is the init of a `for (name = ...)` header, each header
declares its own variable. Anything else is a fallback: with --fixits the
check's own initializer is inserted, and the site is written to the fallback
JSON either way for hand review. Multi-declarator statements are split into
one declaration per line. Run `git clang-format` afterwards.

Usage:
  sink_declarations.py --sites sites.json --fallback fallback.json [--dry] [--fixits] [file...]
"""
import argparse
import json
import re
import sys
from collections import defaultdict

IDENT = r'[A-Za-z_]\w*'


def blank_noncode(text):
    """Return text with comments, string and char literals replaced by spaces (same length)."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = i
            while j < n and text[j] != '\n':
                out[j] = ' '
                j += 1
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if text[k] != '\n':
                    out[k] = ' '
            i = j
        elif c == '"' or c == "'":
            q = c
            j = i + 1
            while j < n and text[j] != q:
                if text[j] == '\\':
                    j += 1
                if j < n and text[j] == '\n':
                    break
                j += 1
            for k in range(i + 1, min(j, n)):
                if text[k] != '\n':
                    out[k] = ' '
            i = j + 1
        else:
            i += 1
    # Blank preprocessor lines too, but mark them with '#'.
    res = ''.join(out)
    lines = res.split('\n')
    for idx, ln in enumerate(lines):
        if ln.lstrip().startswith('#'):
            lines[idx] = '#' + ' ' * (len(ln) - 1)
    return '\n'.join(lines)


def mention_re(name):
    # A mention of the local, not a member or qualified name of the same spelling.
    return re.compile(r'(?<![.\w])(?<!->)(?<!::)' + re.escape(name) + r'\b')


def stmt_bounds(code, off):
    """Bounds [start, end) of the declaration statement containing offset off (end includes ';')."""
    # Backwards to the previous ';', '{', '}' at depth 0 (parens/brackets), or a ':' label.
    i = off - 1
    depth = 0
    while i >= 0:
        c = code[i]
        if c in ')]':
            depth += 1
        elif c in '([':
            if depth == 0:
                return None  # inside a for-init or parameter list
            depth -= 1
        elif depth == 0 and c in ';{}#':
            break
        i -= 1
    start = i + 1
    if start > 0 and code[start - 1] == '#':
        # Skip to the end of the directive line.
        nl = code.find('\n', start)
        if nl < 0 or nl >= off:
            return None
        start = nl + 1
    # Forward to the ';' at depth 0.
    j = off
    depth = 0
    while j < len(code):
        c = code[j]
        if c in '([{':
            depth += 1
        elif c in ')]}':
            if depth == 0:
                return None
            depth -= 1
        elif c == ';' and depth == 0:
            return start, j + 1
        elif c == '#':
            return None
        j += 1
    return None


DECL_RE = re.compile(r'^\s*(?P<prefix>.*?)\s*(?P<ptr>[*&\s]*)(?P<name>' + IDENT + r')\s*(?P<init>=.*)?$', re.S)


def parse_decl(text):
    """Split a declaration statement (without ';') into (prefix, [(ptr, name, init)]) or None."""
    if any(ch in text for ch in '<>(){}[]'):
        return None
    parts = text.split(',')
    m = DECL_RE.match(parts[0])
    if not m:
        return None
    prefix = m.group('prefix').strip()
    if '#' in prefix or '\n' in prefix:
        return None
    if not prefix or prefix in ('return', 'goto', 'case', 'else', 'delete', 'throw'):
        return None
    if re.search(r'\b(static|extern|typedef|using|thread_local|constexpr)\b', prefix):
        return None
    decls = [(m.group('ptr').replace(' ', ''), m.group('name'), (m.group('init') or '').strip())]
    for p in parts[1:]:
        m2 = re.match(r'^\s*(?P<ptr>[*&\s]*)(?P<name>' + IDENT + r')\s*(?P<init>=.*)?$', p, re.S)
        if not m2:
            return None
        decls.append((m2.group('ptr').replace(' ', ''), m2.group('name'), (m2.group('init') or '').strip()))
    return prefix, decls


def has_label_at_depth0(text):
    depth = 0
    for ln in text.split('\n'):
        if depth == 0 and re.match(r'\s*(case\b[^:]*|default\s*):', ln):
            return True
        for c in ln:
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
    return False


def block_end(bl, open_pos):
    """Offset of the '}' matching the '{' at open_pos, or None."""
    depth = 0
    for j in range(open_pos, len(bl)):
        c = bl[j]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return j
    return None


def enclosing_block_end(bl, pos):
    """Offset of the '}' that closes the innermost block containing pos, or len(bl)."""
    depth = 0
    for j in range(pos, len(bl)):
        c = bl[j]
        if c == '{':
            depth += 1
        elif c == '}':
            if depth == 0:
                return j
            depth -= 1
    return len(bl)


def find_nested_sink(code, bl, name, from_off):
    """First mention is `name = expr;` at statement start in a nested block, and every later mention
    stays inside that block. Returns (start, end) of the assignment or (None, reason)."""
    pat = mention_re(name)
    m = pat.search(bl, from_off)
    if not m:
        return None, 'never used'
    pos = m.start()
    between = bl[from_off:pos]
    if '#' in between:
        return None, 'preprocessor between'
    if re.search(r'\bgoto\b', between):
        return None, 'goto between'
    stack = []
    for i, c in enumerate(between):
        if c == '{':
            stack.append(from_off + i)
        elif c == '}':
            if not stack:
                return None, 'left block'
            stack.pop()
    if not stack:
        return None, 'not nested'
    open_pos = stack[-1]
    k = pos - 1
    while k >= 0 and bl[k] in ' \t\n':
        k -= 1
    if k < 0 or bl[k] not in ';{}':
        return None, 'nested: not statement start'
    m2 = re.match(r'\s*=(?!=)', bl[m.end():])
    if not m2:
        return None, 'nested: first use not assignment'
    j = m.end() + m2.end()
    depth = 0
    while j < len(bl):
        c = bl[j]
        if c in '([{':
            depth += 1
        elif c in ')]}':
            if depth == 0:
                return None, 'unbalanced'
            depth -= 1
        elif c == ';' and depth == 0:
            break
        j += 1
    else:
        return None, 'no end'
    close = block_end(bl, open_pos)
    if close is None:
        return None, 'no block end'
    outer_end = enclosing_block_end(bl, from_off)
    if pat.search(bl, close, outer_end):
        return None, 'nested: used outside block'
    # The block must not be a switch body entered through case labels.
    if has_label_at_depth0(bl[open_pos + 1:pos]):
        return None, 'label between'
    if '#' in bl[open_pos:close]:
        return None, 'nested: preprocessor in block'
    if pat.search(bl, m.end(), j):
        return None, 'self-referencing assignment'
    return (pos, j + 1), None


FOR_RE = re.compile(r'\bfor\s*\(\s*$')


def for_extent(bl, header_open):
    """Given the offset of the '(' of a for header, return (header_close, body_end) or None."""
    depth = 0
    j = header_open
    while j < len(bl):
        c = bl[j]
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                break
        j += 1
    else:
        return None
    hc = j
    k = hc + 1
    while k < len(bl) and bl[k] in ' \t\n':
        k += 1
    if k >= len(bl):
        return None
    if bl[k] == '{':
        be = block_end(bl, k)
        return (hc, be + 1) if be is not None else None
    # Simple statement body: up to ';' at depth 0 with no '{' on the way.
    depth = 0
    while k < len(bl):
        c = bl[k]
        if c == '{':
            return None
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == ';' and depth == 0:
            return hc, k + 1
        k += 1
    return None


def find_for_sinks(code, bl, name, from_off):
    """Every mention of name after from_off is the init of a `for (name = ...; ...)` header.
    Returns list of (name_pos) to prefix with the type, or (None, reason)."""
    pat = mention_re(name)
    outer_end = enclosing_block_end(bl, from_off)
    pos = from_off
    sinks = []
    while True:
        m = pat.search(bl, pos, outer_end)
        if not m:
            break
        if '#' in bl[pos:m.start()] and not sinks:
            return None, 'preprocessor between'
        head = bl[max(0, m.start() - 40):m.start()]
        fm = FOR_RE.search(head)
        if not fm:
            return None, 'for: mention not a for-init'
        paren = m.start() - (len(head) - fm.end()) - 1
        while bl[paren] != '(':
            paren -= 1
        m2 = re.match(r'\s*=(?!=)', bl[m.end():])
        if not m2:
            return None, 'for: not assignment'
        # Init clause must end at ';' with no top-level comma.
        j = m.end() + m2.end()
        depth = 0
        while j < len(bl) and not (bl[j] == ';' and depth == 0):
            if bl[j] in '([{':
                depth += 1
            elif bl[j] in ')]}':
                depth -= 1
            elif bl[j] == ',' and depth == 0:
                return None, 'for: comma init'
            j += 1
        ext = for_extent(bl, paren)
        if ext is None:
            return None, 'for: extent unknown'
        hc, body_end = ext
        if re.search(r'\bgoto\b', bl[from_off:body_end]):
            return None, 'goto between'
        sinks.append(m.start())
        pos = body_end
    if not sinks:
        return None, 'never used'
    return sinks, None


def find_sink(code, bl, name, from_off):
    """Find `name = expr;` as the first mention of name at the same depth. Returns (start, end) or a reason."""
    pat = mention_re(name)
    m = pat.search(bl, from_off)
    if not m:
        return None, 'never used'
    pos = m.start()
    between = bl[from_off:pos]
    depth = 0
    for c in between:
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth < 0:
                return None, 'left block'
    if depth != 0:
        return None, 'nested first use'
    if '#' in between:
        return None, 'preprocessor between'
    if re.search(r'\bgoto\b', between):
        return None, 'goto between'
    if has_label_at_depth0(between):
        return None, 'label between'
    # Previous non-space char must end a statement.
    k = pos - 1
    while k >= 0 and bl[k] in ' \t\n':
        k -= 1
    if k < 0 or bl[k] not in ';{}':
        return None, 'not statement start'
    # Next token must be a single '='.
    after = bl[m.end():]
    m2 = re.match(r'\s*=(?!=)', after)
    if not m2:
        return None, 'first use not assignment'
    # Statement end at depth 0.
    j = m.end() + m2.end()
    depth = 0
    while j < len(bl):
        c = bl[j]
        if c in '([{':
            depth += 1
        elif c in ')]}':
            if depth == 0:
                return None, 'unbalanced'
            depth -= 1
        elif c == ';' and depth == 0:
            break
        j += 1
    else:
        return None, 'no end'
    if '\n' in bl[pos:j] and '#' in bl[pos:j]:
        return None, 'preprocessor in statement'
    if pat.search(bl, m.end(), j):
        return None, 'self-referencing assignment'
    if has_label_at_depth0(bl[j + 1:enclosing_block_end(bl, from_off)]):
        return None, 'case label after sink'
    return (pos, j + 1), None


def process_file(path, sites, dry, apply_fixits):
    code = open(path, encoding='latin-1').read()
    bl = blank_noncode(code)
    assert len(bl) == len(code)
    by_stmt = defaultdict(list)
    fallback = []
    for st in sites:
        b = stmt_bounds(bl, st['off'])
        if b is None:
            fallback.append((st, 'no statement bounds'))
            continue
        by_stmt[b].append(st)
    edits = []  # (start, end, replacement)
    moved = 0
    fixed = 0
    for (s, e), sts in sorted(by_stmt.items()):
        text = bl[s:e - 1]
        # Keep leading whitespace/newlines separate.
        lead = re.match(r'\s*', text).group(0)
        body = text[len(lead):]
        parsed = parse_decl(body)
        if parsed is None:
            for st in sts:
                fallback.append((st, 'unparsed declaration'))
            continue
        prefix, decls = parsed
        names = {st['name']: st for st in sts}
        if any(n not in [d[1] for d in decls] for n in names):
            for st in sts:
                fallback.append((st, 'name not in statement'))
            continue
        # Trailing comment on the same line as ';'.
        tc = re.match(r'[ \t]*(//[^\n]*|/\*[^\n]*\*/)', code[e:])
        trailing = tc.group(1) if tc else ''
        trailing_end = e + tc.end() if tc else e
        indent = re.match(r'[ \t]*', code[code.rfind('\n', 0, s) + 1:s + len(lead)].split('\n')[-1]).group(0)
        # Indentation of the statement line.
        line_start = code.rfind('\n', 0, s + len(lead)) + 1
        indent = re.match(r'[ \t]*', code[line_start:]).group(0)
        remaining = []
        sunk = []
        for ptr, name, init in decls:
            if name in names and not init:
                sink, why = find_sink(code, bl, name, e)
                if sink is None and why in ('nested first use',):
                    sink, why2 = find_nested_sink(code, bl, name, e)
                    why = why + ' / ' + why2 if sink is None else why
                if sink is None:
                    fors, why3 = find_for_sinks(code, bl, name, e)
                    if fors is not None:
                        sink = ('for', fors)
                    else:
                        why = why + ' / ' + why3
                if sink is None:
                    fallback.append((names[name], why))
                    remaining.append((ptr, name, init))
                else:
                    sunk.append((ptr, name, sink))
            else:
                remaining.append((ptr, name, init))
        if not sunk:
            if apply_fixits:
                for ptr, name, init in remaining:
                    st = names.get(name)
                    if st and not init and st['fix']:
                        p = st['off'] + len(name)
                        edits.append((p, p, st['fix']))
                        fixed += 1
            continue
        # Rewrite the declaration statement.
        new_lines = []
        for ptr, name, init in remaining:
            st = names.get(name)
            if apply_fixits and st and not init and st['fix']:
                init = st['fix'].strip()
                fixed += 1
            new_lines.append(f'{prefix} {ptr}{name}{(" " + init) if init else ""};')
        if new_lines:
            repl = ('\n' + indent).join(new_lines)
            if trailing:
                repl += '  ' + trailing
            edits.append((s + len(lead), trailing_end, repl))
        else:
            # Remove the whole statement line(s) including trailing comment and the newline.
            rm_start = line_start if code[line_start:s + len(lead)].strip() == '' else s + len(lead)
            rm_end = trailing_end
            if code[rm_end:rm_end + 1] == '\n' and rm_start == line_start:
                rm_end += 1
            edits.append((rm_start, rm_end, ''))
        for ptr, name, sink in sunk:
            moved += 1
            if sink[0] == 'for':
                for p in sink[1]:
                    edits.append((p, p, f'{prefix} {ptr}'))
                continue
            as_s, as_e = sink
            edits.append((as_s, as_s, f'{prefix} {ptr}'))
            if not new_lines and trailing and len(decls) == 1:
                # Keep the declaration's comment on the moved line.
                edits.append((as_e, as_e, '  ' + trailing))
    if not dry and edits:
        edits.sort(key=lambda x: (x[0], x[1]))
        # Check overlaps.
        last = -1
        for a, b, r in edits:
            if a < last:
                raise RuntimeError(f'overlap in {path} at {a}')
            last = b
        out = []
        pos = 0
        for a, b, r in edits:
            out.append(code[pos:a])
            out.append(r)
            pos = b
        out.append(code[pos:])
        open(path, 'w', encoding='latin-1').write(''.join(out))
    return moved, fixed, fallback


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sites', required=True, help='JSON list of findings')
    ap.add_argument('--fallback', required=True, help='where to write the unhandled sites')
    ap.add_argument('--dry', action='store_true', help='report only, change nothing')
    ap.add_argument('--fixits', action='store_true', help="apply the check's initializer to fallbacks")
    ap.add_argument('only', nargs='*', help='restrict to files ending with these paths')
    args = ap.parse_args()
    sites = json.load(open(args.sites))
    by_file = defaultdict(list)
    for st in sites:
        if args.only and not any(st['file'].endswith(o) for o in args.only):
            continue
        by_file[st['file']].append(st)
    total_moved = 0
    total_fixed = 0
    reasons = defaultdict(int)
    fb_all = []
    for f, sts in sorted(by_file.items()):
        moved, fixed, fb = process_file(f, sts, args.dry, args.fixits)
        total_moved += moved
        total_fixed += fixed
        for st, why in fb:
            reasons[why] += 1
            fb_all.append({**st, 'why': why})
    print('moved', total_moved, 'fixits', total_fixed, 'fallback', len(fb_all),
          'no-fixit', sum(1 for x in fb_all if not x['fix']))
    for k, v in sorted(reasons.items(), key=lambda x: -x[1]):
        print(f'  {v:5d} {k}')
    json.dump(fb_all, open(args.fallback, 'w'), indent=1)


if __name__ == '__main__':
    main()
