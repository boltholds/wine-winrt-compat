"""Static negative controls for diagnostic-only CoreApplicationView tracing."""
from pathlib import Path
import re,unittest
ROOT=Path(__file__).resolve().parents[3]
SRC=(ROOT/"dlls/twinapi.appcore/core_application.c").read_text(encoding="utf-8")
METHODS={
 "QueryInterface":"QI","GetIids":"GETIIDS",
 "GetRuntimeClassName":"GETCLASS","GetTrustLevel":"GETTRUST",
 "get_CoreWindow":"GETWINDOW","add_Activated":"ADD_ACTIVATED",
 "remove_Activated":"REMOVE_ACTIVATED",
 "get_IsMain":"GET_ISMAIN","get_IsHosted":"GET_ISHOSTED"
}
class TraceContract(unittest.TestCase):
 def test_all_nine_vtable_calls_traced(self):
  for method,marker in METHODS.items():
   self.assertEqual(SRC.count("COREAPP57_CALL_"+marker),1)
   self.assertEqual(len(re.findall(r"static HRESULT WINAPI coreview_v54_"+method+r"\(",SRC)),1)
 def test_optin_flag(self):
  self.assertIn('WINE_COREAPP_INIT_TRACE_V57',SRC)
  self.assertIn('if (v57_trace_enabled()) TRACE(',SRC)
 def test_no_fabricated_activation_success(self):
  for name,nextname in (("add_Activated","remove_Activated"),("remove_Activated","get_IsMain")):
   fragment=SRC.split("static HRESULT WINAPI coreview_v54_"+name,1)[1].split("static HRESULT WINAPI coreview_v54_"+nextname,1)[0]
   self.assertIn("return E_NOTIMPL;",fragment)
   self.assertNotIn("return S_OK;",fragment)
 def test_full_lifecycle_keeps_failure_semantics(self):
  self.assertIn("COREAPP56_INITIALIZE",SRC)
  self.assertIn("COREAPP56_LIFECYCLE_RETURN",SRC)
  self.assertIn("if (SUCCEEDED(hr) && FAILED(cleanup)) hr=cleanup;",SRC)
if __name__ == "__main__":unittest.main()
