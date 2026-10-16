"""Validate pinned observations of bounded Wine v56 lifecycle."""
from pathlib import Path
import hashlib,json,unittest

ROOT=Path(__file__).resolve().parents[1]
CASE_HRESULT={
 "v47_disabled":"0x80004001",
 "v54_fallback":"0x80004001",
 "missing_window_flag":"0x80004001",
 "complete":"0x00000000",
 "initialize_fail":"0x80004005",
 "setwindow_fail":"0x80070005",
 "load_fail":"0x80070057",
 "run_fail":"0x8000ffff",
 "uninit_fail":"0x80004004",
 "run_uninit_fail":"0x8000ffff",
}
COUNTS={
 "v47_disabled":"1,1,0,0,0,0,0",
 "v54_fallback":"1,1,1,0,0,0,1",
 "missing_window_flag":"1,1,0,0,0,0,0",
 "complete":"1,1,1,1,1,1,1",
 "initialize_fail":"1,1,1,0,0,0,0",
 "setwindow_fail":"1,1,1,1,0,0,1",
 "load_fail":"1,1,1,1,1,0,1",
 "run_fail":"1,1,1,1,1,1,1",
 "uninit_fail":"1,1,1,1,1,1,1",
 "run_uninit_fail":"1,1,1,1,1,1,1",
}

class V56Evidence(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.data=json.loads((ROOT/"observations.json").read_text())
 def markers(self,name):
  return "\n".join(self.data["results"][name]["markers"])
 def test_probe_source_sha(self):
  self.assertEqual(hashlib.sha256((ROOT/"full_frameworkview_mock.c").read_bytes()).hexdigest(),
   "6f257448d1d8dc67556ebb2f43e25cd2a03d55364bdc21fec145de98c4087275")
 def test_10_passes(self):
  self.assertEqual(set(self.data["results"]),set(CASE_HRESULT))
  for case in self.data["results"].values():
   self.assertEqual(case["status"],0)
   self.assertTrue(case["passed"])
 def test_hresult_fidelity(self):
  for mode,hr in CASE_HRESULT.items():
   self.assertIn("V56_COREAPP_RUN_HR="+hr,self.markers(mode))
 def test_exact_phase_sequence(self):
  for mode,value in COUNTS.items():
   self.assertIn("V56_COUNTS="+value,self.markers(mode))
 def test_reference_balance(self):
  for mode in CASE_HRESULT:
   self.assertIn("V56_SOURCE_REFS=1 VIEW_REFS=1",self.markers(mode))
 def test_real_window_dispatcher(self):
  s=self.markers("complete")
  self.assertIn("V56_GET_COREWINDOW_HR=0x00000000 NULL=0",s)
  self.assertIn("V56_STATIC_GET_CURRENT_HR=0x00000000",s)
  self.assertIn("V56_DISPATCHER_UNTIL_QUIT_HR=0x00000000",s)
  self.assertIn("V56_CHECK_AFTER_RUN_COREWINDOW_CLEARED=PASS",s)
 def test_first_hresult_preserved(self):
  self.assertIn("V56_COREAPP_RUN_HR=0x8000ffff",self.markers("run_uninit_fail"))
 def test_old_regression(self):
  reg=json.loads((ROOT/"v47_regression.json").read_text())
  self.assertEqual(reg["v47_regression"],"17/17 PASS")
  self.assertTrue(reg["laboratory_dll_exactly_restored"])
 def test_lab_restoration(self):
  self.assertTrue(self.data["exact_original_dll_restoration"])
  self.assertTrue(self.data["native_windows_parity_not_tested"])

if __name__=="__main__":unittest.main()
