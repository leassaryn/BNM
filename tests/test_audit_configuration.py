import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/audit'))
from configuration import set_define, effective_define


class ConfigurationTests(unittest.TestCase):
    def test_comment_and_whitespace(self):
        for suffix in ['\n', ' // patch\n', '  \n']:
            text = '// #define UNITY_PATCH_VER 99\n#define UNITY_PATCH_VER 32' + suffix
            changed = set_define(text, 'UNITY_PATCH_VER', 57)
            self.assertIn('#define UNITY_PATCH_VER 57', changed)
            self.assertIn('// #define UNITY_PATCH_VER 99', changed)

    def test_missing_or_duplicate_is_error(self):
        for text in ['', '// #define UNITY_VER 222\n', '#define UNITY_VER 222\n#define UNITY_VER 223\n']:
            with self.assertRaises(ValueError):
                set_define(text, 'UNITY_VER', 6000)

    def test_compiler_macros(self):
        self.assertEqual(effective_define('#define UNITY_VER 6000\n', 'UNITY_VER'), 6000)
        with self.assertRaises(ValueError):
            effective_define('#define UNITY_VERSION 6000\n', 'UNITY_VER')


if __name__ == '__main__':
    unittest.main()
