"""v58 source contracts: IsMain alone returns true in guarded main view."""
from pathlib import Path
import re,unittest
ROOT=Path(__file__).resolve().parents[3]
SRC=(ROOT/'dlls/twinapi.appcore/core_application.c').read_text('utf-8')
SEG=SRC.split('static HRESULT WINAPI coreview_v54_get_IsMain(',1)[1].split(
 'static HRESULT WINAPI coreview_v54_get_IsHosted(',1)[0]
class V58IsMainContract(unittest.TestCase):
 def test_unique_impl(self):
  self.assertEqual(SRC.count('static HRESULT WINAPI coreview_v54_get_IsMain('),1)
  self.assertEqual(SRC.count('COREAPP58_ISMAIN_SUCCESS'),1)
 def test_opt_in_only(self):
  self.assertIn('WINE_COREAPP_MAINVIEW_V58',SEG)
  self.assertIn('!obj->window',SEG)
  self.assertIn('return E_NOTIMPL;',SEG)
 def test_safely_checks_window_and_thread(self):
  self.assertIn('RPC_E_WRONG_THREAD',SEG)
  self.assertIn('GetCurrentThreadId()!=obj->owner_tid',SEG)
  self.assertIn('IsWindow(obj->hwnd)',SEG)
 def test_returns_main_only_after_valid_checks(self):
  self.assertLess(SEG.index('IsWindow(obj->hwnd)'),SEG.index('*value=TRUE;'))
  self.assertLess(SEG.index('*value=TRUE;'),SEG.index('return S_OK;'))
 def test_no_change_to_other_winrt_methods(self):
  self.assertIn('COREAPP57_CALL_ADD_ACTIVATED',SRC)
  for name,next_name in [('add_Activated','remove_Activated'),('remove_Activated','get_IsMain')]:
   body=SRC.split('static HRESULT WINAPI coreview_v54_'+name+'(',1)[1].split(
    'static HRESULT WINAPI coreview_v54_'+next_name+'(',1)[0]
   self.assertIn('return E_NOTIMPL;',body)
   self.assertNotIn('return S_OK;',body)
 def test_lifecycle_cleanup_retained(self):
  self.assertIn('COREAPP56_LIFECYCLE_RETURN',SRC)
  self.assertIn('COREAPP56_INITIALIZE',SRC)
  self.assertIn('if (SUCCEEDED(hr) && FAILED(cleanup)) hr=cleanup;',SRC)
if __name__=='__main__':unittest.main()
