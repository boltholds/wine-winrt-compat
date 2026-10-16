import hashlib,json
from pathlib import Path
import unittest
HERE=Path(__file__).resolve().parents[1]
class Evidence(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.record=json.loads((HERE/'observations.json').read_text())
 def test_exact_probe(self):
  self.assertEqual(hashlib.sha256((HERE/'corewindow_smoke.c').read_bytes()).hexdigest(),
   'b2411be9fde9d65f80726d372d2701ad65a1a40e753a2a05cef4c4fe81367c5b')
 def test_five_cases(self):
  cases=self.record['cases']
  self.assertEqual(set(cases),{'disabled','basic','pump','wrong_thread','until_quit'})
  for item in cases.values():
   self.assertEqual(item['exit_code'],0)
   self.assertTrue(item['pass'])
 def test_com_and_real_window(self):
  for mode in ('basic','pump','wrong_thread','until_quit'):
   markers='\n'.join(self.record['cases'][mode]['markers'])
   for check in ('REAL_HWND_CREATED','COM_LIFETIME_COMPLETE','NO_FAKE_INPUT_SUPPRESSION','ASYNC_NOT_FAKED'):
    self.assertIn('V55_CHECK_'+check+'=PASS',markers)
 def test_wrong_thread(self):
  m='\n'.join(self.record['cases']['wrong_thread']['markers'])
  for check in ('WRONG_THREAD_NOT_OWNER','WRONG_THREAD_CANNOT_GET_DISPATCHER',
                'WRONG_THREAD_CANNOT_PUMP','WRONG_THREAD_CANNOT_CLOSE'):
   self.assertIn('V55_CHECK_'+check+'=PASS',m)
 def test_messages_and_close(self):
  m='\n'.join(self.record['cases']['pump']['markers'])
  self.assertIn('COREWINDOW55_APP_MESSAGE_RECEIVED',m)
  self.assertIn('V55_CHECK_MESSAGE_REMOVED=PASS',m)
  m='\n'.join(self.record['cases']['until_quit']['markers'])
  self.assertIn('COREDISPATCHER55_UNTIL_QUIT_COMPLETE',m)
 def test_disabled(self):
  self.assertIn('V55_CHECK_DISABLED_NO_WINDOW=PASS',
                self.record['cases']['disabled']['markers'])
 def test_restored(self):
  self.assertTrue(self.record['baseline_dll_restored_exactly'])
  self.assertEqual(self.record['baseline_sha256'],
        '7efd0e040d64a504306287cd64c8f67c726c797e09988b246ed335d6690e30e2')
if __name__=='__main__':unittest.main()
