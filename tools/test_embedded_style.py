import unittest
from check_embedded_style import violations


class SingleReturnTests(unittest.TestCase):
    def test_final_return_and_void_functions(self):
        self.assertEqual(violations('int f() { int x=0; if (x) { x=1; } return x; }'), [])
        self.assertEqual(violations('void f() { work(); }'), [])

    def test_multiple_returns_are_rejected(self):
        self.assertTrue(violations('int f() { if (ready()) return 1; return 0; }'))

    def test_single_nested_return_is_not_a_final_exit(self):
        self.assertTrue(violations('int f() { if (ready()) { return 1; } }'))

    def test_work_after_return_is_rejected(self):
        self.assertTrue(violations('int f() { return 1; work(); }'))

    def test_comments_strings_and_inline_members(self):
        source = '''class A { public:
          A() : x(0) {} // return 1;
          int f() const override { log("return;"); return x; }
          void g() { /* return; */ if (x) { log("hi"); } }
        };'''
        self.assertEqual(violations(source), [])

    def test_conditional_branches_and_hidden_macro_returns(self):
        self.assertTrue(violations('#define EXIT return 0\nint f() { EXIT; }'))
        self.assertTrue(violations('int f() {\n#ifdef A\nreturn 1;\n#else\nreturn 0;\n#endif\n}'))


if __name__ == "__main__":
    unittest.main()
