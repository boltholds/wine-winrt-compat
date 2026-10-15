"""Negative controls for the observed v52 CoreApplication.Run root cause."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
PROOF = ROOT / "coreapp_run_root_cause.json"

class CoreApplicationRunEvidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.evidence = json.loads(PROOF.read_text(encoding="utf-8"))

    def test_fatal_hresult_is_run_not_activation(self):
        e = self.evidence["exact_observations"]["after_repair"]
        self.assertEqual(e["framework_view_source_activation_hresult"], "0x00000000")
        self.assertEqual(e["core_application_queryinterface_hresult"], "0x00000000")
        self.assertEqual(e["core_application_run_hresult"], "0x80004001")
        self.assertEqual(e["shapr3d_exception_projection_hresult"], "0x80004001")

    def test_view_creation_succeeded_after_repair(self):
        e = self.evidence["exact_observations"]["after_repair"]
        self.assertEqual(e["view_source_create_view_hresult"], "0x00000000")
        self.assertEqual(e["framework_view_queryinterface_hresult"], "0x00000000")

    def test_stale_module_path_not_classified_as_wine_runtime_failure(self):
        e = self.evidence["exact_observations"]["first_incomplete_cleanup_run"]
        self.assertEqual(e["create_view_hresult"], "0x8007007E")
        self.assertEqual(e["classification"], "test-environment-artifact")

    def test_abi_vtable_is_exact(self):
        x = self.evidence["static_abi"]
        self.assertEqual(int(x["nested_xaml_vtable_call_byte_offset"], 16), 13 * 8)
        self.assertEqual(x["coreapplication_run_vtable_index"], 13)

    def test_no_fake_gui_completion(self):
        x = self.evidence["limits"]
        self.assertFalse(x["run_lifecycle_implemented"])
        self.assertFalse(x["actual_corewindow_created"])
        self.assertFalse(x["interactive_shapr3d_ui_demonstrated"])
        self.assertTrue(x["no_proprietary_application_binaries_committed"])

if __name__ == "__main__":
    unittest.main()
