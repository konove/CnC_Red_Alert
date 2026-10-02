#!/usr/bin/env python3
"""Tests for tools/split_text_print.py. Run: tools/split_text_print_test.py"""

import unittest

import split_text_print as stp


def rewrite(text, callees=("Fancy_Text_Print",), game="ra"):
    return stp.rewrite_text(text, game, set(callees), "f.cc")


class RewriteTest(unittest.TestCase):
    def test_splits_font_from_flags(self):
        out, notes = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_6PT_GRAD | TPF_NOSHADOW);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::k6PointGradient, TPF_NOSHADOW);")
        self.assertEqual(notes, [])

    def test_font_alone_gets_tpf_none(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_EFNT);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::kEditor, TPF_NONE);")

    def test_flags_alone_get_the_eight_point_font(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, TPF_CENTER|TPF_NOSHADOW);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, FontType::k8Point, TPF_CENTER | TPF_NOSHADOW);")

    def test_presets_split_into_their_new_names(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0, kTpfText | TPF_CENTER);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0, kTextFont, kTextFlags | TPF_CENTER);")

    def test_multiline_and_casts(self):
        out, _ = rewrite("Fancy_Text_Print(v, t, x, y, &c, 0,\n    static_cast<TextPrintType>(TPF_8POINT | TPF_DROPSHADOW), a);")
        self.assertEqual(out, "Fancy_Text_Print(v, t, x, y, &c, 0,\n    FontType::k8Point, TPF_DROPSHADOW, a);")

    def test_declarations_and_new(self):
        out, _ = rewrite("TextButtonClass ok(1, TXT_OK, TPF_EFNT | TPF_CENTER, 2, 3);\n"
                         "p = new TextButtonClass(1, t, TPF_6POINT, 2, 3);",
                         callees=("TextButtonClass",))
        self.assertEqual(out, "TextButtonClass ok(1, TXT_OK, FontType::kEditor, TPF_CENTER, 2, 3);\n"
                              "p = new TextButtonClass(1, t, FontType::k6Point, TPF_NONE, 2, 3);")

    def test_reports_what_it_cannot_split(self):
        src = ("Fancy_Text_Print(v, t, x, y, &c, 0, flags | TPF_CENTER);\n"
               "Fancy_Text_Print(v, t, x, y, &c, 0, kTpfText | TPF_8POINT);\n"
               "Fancy_Text_Print(v, t, x, y, &c, 0, on ? TPF_6POINT : TPF_8POINT);")
        out, notes = rewrite(src)
        self.assertEqual(out, src)
        self.assertEqual([n.split(":")[1] for n in notes], ["1", "2", "3"])

    def test_leaves_other_callees_and_strings_alone(self):
        src = 'Other(TPF_6POINT); Fancy_Text_Print(v, "TPF_6POINT, x", x, y, &c, 0, flags);'
        out, notes = rewrite(src)
        self.assertEqual(out, src)
        self.assertEqual(notes, [])

    def test_skips_calls_in_comments(self):
        src = ("void F() {\n"
               "  // Fancy_Text_Print(txt, x,\n"
               "  //   y, TPF_8POINT);\n"
               "  /* Fancy_Text_Print(a, */\n"
               "  Fancy_Text_Print(v, t, x, y, &c, 0, TPF_6POINT);\n"
               "}\n")
        out, notes = rewrite(src)
        self.assertEqual(out, src.replace("TPF_6POINT)", "FontType::k6Point, TPF_NONE)"))
        self.assertEqual(notes, [])

    def test_td_fonts(self):
        out, _ = rewrite("Fancy_Text_Print(t, x, y, 5, 0, TPF_GREEN12_GRAD | TPF_FULLSHADOW);", game="td")
        self.assertEqual(out, "Fancy_Text_Print(t, x, y, 5, 0, FontType::kGreen12Gradient, TPF_FULLSHADOW);")


if __name__ == "__main__":
    unittest.main()
