#!/usr/bin/env python3
"""Splits literal TextPrintType arguments into a FontType and style flags.

docs/TEXT_PRINT_SPLIT_PLAN.md moves the font out of TextPrintType. In each
call to, or construction of, a named callee, every argument made only of
TPF_ constants and presets joined by '|' (in parentheses or a
static_cast<TextPrintType>(...) or not) becomes "FontType::kX, <flags>".
Arguments that mention TPF_ but are not that simple are reported, not
touched; the compiler finds the rest.

Usage:
  tools/split_text_print.py --game ra --callees Fancy_Text_Print,... FILE...
"""

import argparse
import re
import sys

FONTS = {
    "ra": {
        "TPF_LASTPOINT": "k8Point", "TPF_6POINT": "k6Point", "TPF_8POINT": "k8Point",
        "TPF_3POINT": "k3Point", "TPF_LED": "kLed", "TPF_VCR": "kVcr",
        "TPF_6PT_GRAD": "k6PointGradient", "TPF_MAP": "kMap", "TPF_METAL12": "kMetal12",
        "TPF_EFNT": "kEditor", "TPF_TYPE": "kType", "TPF_SCORE": "kScore",
    },
    "td": {
        "TPF_LASTPOINT": "k8Point", "TPF_6POINT": "k6Point", "TPF_8POINT": "k8Point",
        "TPF_3POINT": "k3Point", "TPF_LED": "kLed", "TPF_VCR": "kVcr",
        "TPF_6PT_GRAD": "k6PointGradient", "TPF_MAP": "kMap",
        "TPF_GREEN12": "kGreen12", "TPF_GREEN12_GRAD": "kGreen12Gradient",
    },
}

# Combined presets and the font and flags constants that replace them.
PRESETS = {
    "ra": {
        "kTpfButton": ("kButtonFont", "kButtonFlags"),
        "kTpfEButton": ("kEButtonFont", "kEButtonFlags"),
        "kTpfText": ("kTextFont", "kTextFlags"),
    },
    "td": {},
}

STYLE = {"TPF_NONE", "TPF_NOSHADOW", "TPF_DROPSHADOW", "TPF_FULLSHADOW",
         "TPF_LIGHTSHADOW", "TPF_CENTER", "TPF_RIGHT", "TPF_MEDIUM_COLOR",
         "TPF_BRIGHT_COLOR", "TPF_USE_GRAD_PAL", "TPF_LASTSHADOW"}

CAST = re.compile(r"static_cast\s*<\s*TextPrintType\s*>")
SIMPLE = re.compile(r"^[\s\w|()]*$")
SPLIT = re.compile(r"\bFontType::|\bk\w*Font\b")


def skip_literal(text, i):
    """Returns the index after the string or char literal starting at i."""
    quote = text[i]
    i += 1
    while i < len(text) and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def skip_comment(text, i):
    if text.startswith("//", i):
        end = text.find("\n", i)
        return len(text) if end < 0 else end
    end = text.find("*/", i + 2)
    return len(text) if end < 0 else end + 2


def code_only(text):
    """Returns text with its string and char literals and comments replaced by
    spaces, keeping every offset and newline."""
    out, i = [], 0
    while i < len(text):
        if text[i] in "\"'":
            end = skip_literal(text, i)
        elif text.startswith("//", i) or text.startswith("/*", i):
            end = skip_comment(text, i)
        else:
            out.append(text[i])
            i += 1
            continue
        out.append(re.sub(r"[^\n]", " ", text[i:end]))
        i = end
    return "".join(out)


def split_args(text, open_index):
    """Returns (close_index, [(start, end), ...]) of the top-level arguments
    of the bracket at open_index, or None if it never closes."""
    pairs = {"(": ")", "{": "}", "[": "]"}
    stack = [pairs[text[open_index]]]
    args, start, i = [], open_index + 1, open_index + 1
    while i < len(text):
        c = text[i]
        if c in "\"'":
            i = skip_literal(text, i)
            continue
        if text.startswith("//", i) or text.startswith("/*", i):
            i = skip_comment(text, i)
            continue
        if c in pairs:
            stack.append(pairs[c])
        elif c in ")}]":
            stack.pop()
            if not stack:
                args.append((start, i))
                return i, args
        elif c == "," and len(stack) == 1:
            args.append((start, i))
            start = i + 1
        i += 1
    return None


def split_arg(arg, game):
    """Returns "FontType::kX, flags" for a simple TPF argument, "" for an
    argument without TPF_ or presets, or None if it cannot be split."""
    fonts, presets = FONTS[game], PRESETS[game]
    words = set(re.findall(r"\w+", code_only(arg)))
    if not any(w.startswith("TPF_") or w in presets for w in words):
        return ""
    body = CAST.sub("", arg)
    if not SIMPLE.match(body):
        return None
    tokens = re.findall(r"\w+", body)
    font, flags = None, []
    for token in tokens:
        if token in fonts:
            new = "FontType::" + fonts[token]
        elif token in presets:
            new = presets[token][0]
            flags.append(presets[token][1])
        elif token in STYLE:
            flags.append("TPF_NONE" if token == "TPF_LASTSHADOW" else token)
            continue
        else:
            return None
        if font is not None and font != new:
            return None
        font = new
    if font is None:
        font = "FontType::k8Point"
    flags = [f for f in flags if f != "TPF_NONE"] or ["TPF_NONE"]
    return f"{font}, {' | '.join(flags)}"


def rewrite_text(text, game, callees, path):
    names = "|".join(sorted(map(re.escape, callees), key=len, reverse=True))
    call = re.compile(r"\b(?:" + names + r")\b(?:\s*<[^<>;()]*>)?(?:\s*>|\s*[*&]?\s*[A-Za-z_]\w*)?\s*([({])")
    # Calls are found in the code alone, so a commented-out call is skipped;
    # the offsets are the same in both texts.
    code = code_only(text)
    out, notes, pos = [], [], 0
    for match in call.finditer(code):
        if match.start() < pos:
            continue
        parsed = split_args(code, match.start(1))
        if parsed is None:
            continue
        close, args = parsed
        if SPLIT.search(code[match.end(1):close]):
            continue  # Already split.
        out.append(text[pos:match.end(1)])
        cursor = match.end(1)
        for start, end in args:
            arg = text[start:end]
            stripped = arg.strip()
            new = split_arg(stripped, game)
            if new is None:
                line = text.count("\n", 0, start) + 1
                notes.append(f"{path}:{line}: not rewritten: {' '.join(stripped.split())}")
                new = ""
            if new:
                lead = arg[: len(arg) - len(arg.lstrip())]
                trail = arg[len(arg.rstrip()):]
                out.append(text[cursor:start] + lead + new + trail)
            else:
                out.append(text[cursor:end])
            cursor = end
        out.append(text[cursor:close])
        pos = close
    out.append(text[pos:])
    return "".join(out), notes


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game", choices=sorted(FONTS), required=True)
    parser.add_argument("--callees", required=True)
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()
    callees = set(args.callees.split(","))
    for path in args.files:
        with open(path, encoding="utf-8") as f:
            text = f.read()
        new, notes = rewrite_text(text, args.game, callees, path)
        for note in notes:
            print(note)
        if new != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(new)
    return 0


if __name__ == "__main__":
    sys.exit(main())
