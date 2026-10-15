"""Negative-control tests for reviewed Wine source identity."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
P = ROOT / "tools" / "winrt" / "verify_port_source.py"
SPEC = importlib.util.spec_from_file_location("verify_port_source", P)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

class SourceIdentityTests(unittest.TestCase):
    def test_exact_ported_sources(self):
        self.assertEqual(MODULE.validate_expected(MODULE.EXPECTED_BLOBS), 11)
    def test_missing_source_rejected(self):
        d = dict(MODULE.EXPECTED_BLOBS)
        d.pop(next(iter(d)))
        with self.assertRaises(ValueError):
            MODULE.validate_expected(d)
    def test_unreviewed_blob_rejected(self):
        d = dict(MODULE.EXPECTED_BLOBS)
        key = next(iter(d))
        d[key] = "0" * 40
        with self.assertRaises(ValueError):
            MODULE.validate_expected(d)

if __name__ == "__main__":
    unittest.main()
