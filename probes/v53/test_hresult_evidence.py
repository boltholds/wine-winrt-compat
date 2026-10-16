#!/usr/bin/env python3
"""Evidence-only v53 HRESULT interpretation; no proprietary binaries required."""
import unittest

E_NOTIMPL = 0x80004001
ERROR_MOD_NOT_FOUND = 0x8007007E
EXCEPTION_NATIVE_FAILFAST = 0x302BFF64
TRACE = {
    "RoActivateInstance": 0,
    "QueryInterface": 0,
    "ICoreApplication_Run": E_NOTIMPL,
    "Shapr3D_HRESULT_at_projection": E_NOTIMPL,
    "SharedLibrary_import_target": "mrt100_app.dll!RhpThrowEx",
    "app_fully_works": False,
    "native_GetCurrentView_parity": "unverified",
}

class V53EvidenceTests(unittest.TestCase):
    def test_run_failure_propagated_unchanged(self):
        self.assertEqual(TRACE["ICoreApplication_Run"], TRACE["Shapr3D_HRESULT_at_projection"])
        self.assertEqual(TRACE["ICoreApplication_Run"], E_NOTIMPL)
    def test_activation_precedes_run(self):
        self.assertEqual(TRACE["RoActivateInstance"], 0)
        self.assertEqual(TRACE["QueryInterface"], 0)
    def test_distinct_missing_module_error_is_not_run_error(self):
        self.assertNotEqual(ERROR_MOD_NOT_FOUND, E_NOTIMPL)
        self.assertNotEqual(EXCEPTION_NATIVE_FAILFAST, E_NOTIMPL)
    def test_limitations_explicit(self):
        self.assertFalse(TRACE["app_fully_works"])
        self.assertEqual(TRACE["native_GetCurrentView_parity"], "unverified")

if __name__ == "__main__":
    unittest.main()
