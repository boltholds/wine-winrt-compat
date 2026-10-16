"""Ensure v52 reports represent the corrected dispatcher and real negative paths."""
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "v52_contract_probe.c"
REPORT = ROOT / "v52_native_wine_observations.json"

class V52ContractEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.doc = json.loads(REPORT.read_text(encoding="utf-8"))

    def test_corrected_source(self):
        raw = SOURCE.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(),
            "d77949eb568ddfdd6501cbce15c8214e01a98912beba4f4bb2c8dff0c1490459")
        self.assertIn(b'code=strcmp(argv[1],"corewindow")==0 ?', raw)

    def test_exact_four_cases(self):
        self.assertEqual(set(self.doc["observations"]), {
            "corewindow-sta", "corewindow-mta", "accessibility-sta", "accessibility-mta"})
        self.assertFalse(self.doc["originating_hresult_captured"])

    def test_no_corewindow_matches(self):
        for apartment in ("sta", "mta"):
            pair = self.doc["observations"]["corewindow-" + apartment]
            for platform in ("native", "wine"):
                d = pair[platform]
                self.assertEqual(d["V52_COREWINDOW_GET_CURRENT_HR"], "0x00000000")
                self.assertEqual(d["V52_COREWINDOW_GET_CURRENT_NULL"], "1")

    def test_event_registration_fails_without_reference_leak(self):
        for apartment in ("sta", "mta"):
            pair = self.doc["observations"]["accessibility-" + apartment]
            native, wine = pair["native"], pair["wine"]
            for field in ("V52_ACCESSIBILITY_ADD1_HR",
                          "V52_ACCESSIBILITY_ADD_NULL_HANDLER_HR"):
                self.assertEqual(native[field], "0x80070490")
                self.assertEqual(wine[field], "0x80004001")
            self.assertEqual(native["V52_HANDLER_REFS_FINAL"], "1")
            self.assertEqual(wine["V52_HANDLER_REFS_FINAL"], "1")
            self.assertEqual(native["V52_HANDLER_INVOKED"], "0")
            self.assertEqual(wine["V52_HANDLER_INVOKED"], "0")

if __name__ == "__main__":
    unittest.main()
