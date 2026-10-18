"""v67 bounded Suspending registration source-contract tests.

These are static checks. PE64 cross-compilation and a guarded actual replay
are separate requirements; successful subscription does not deliver events.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SRC = (ROOT / "dlls/twinapi.appcore/core_application.c").read_text("utf-8")
IDL = (ROOT / "include/windows.applicationmodel.core.idl").read_text("utf-8")
STORAGE = SRC.split("struct suspending_v67_entry", 1)[1].split(
    "static HRESULT WINAPI coreapp_add_Suspending(", 1
)[0]
ADD = SRC.split("static HRESULT WINAPI coreapp_add_Suspending(", 1)[1].split(
    "static HRESULT WINAPI coreapp_remove_Suspending(", 1
)[0]
REMOVE = SRC.split("static HRESULT WINAPI coreapp_remove_Suspending(", 1)[1].split(
    "static void suspending_v67_clear_all(", 1
)[0]
CLEAR = SRC.split("static void suspending_v67_clear_all(void)\n{", 1)[1].split(
    "static HRESULT WINAPI coreapp_add_Resuming(", 1
)[0]
LIFECYCLE = SRC.split("static HRESULT coreview_v56_run_lifecycle(", 1)[1].split(
    "static HRESULT WINAPI coreapp_Run(", 1
)[0]

class SuspendingV67Contracts(unittest.TestCase):
    def test_source_is_based_on_safe_v66(self):
        self.assertIn("COREAPP58_ISMAIN_SUCCESS", SRC)
        self.assertIn("COREAPP65_UNHANDLED_ADD", SRC)
        self.assertIn("COREAPP66_QI_COREAPP2_OK", SRC)
        self.assertIn("COREAPP66_CALL_", SRC)
        self.assertNotIn("COREAPP59_GETIIDS_SUCCESS", SRC)
        self.assertNotIn("WINE_COREAPP_GETIIDS_V59", SRC)

    def test_public_idl_matches_signature(self):
        self.assertIn("interface ICoreApplication : IInspectable", IDL)
        self.assertIn("HRESULT Suspending([in] Windows.Foundation.EventHandler", IDL)
        self.assertIn("IEventHandler_SuspendingEventArgs *handler, EventRegistrationToken *token", SRC)
        self.assertIn("coreapp_add_Suspending,", SRC)
        self.assertIn("coreapp_remove_Suspending,", SRC)

    def test_add_is_opt_in_only(self):
        self.assertIn("WINE_COREAPP_SUSPENDING_V67", STORAGE)
        self.assertIn("if (!suspending_v67_enabled()) return E_NOTIMPL;", ADD)
        self.assertLess(ADD.index("if (!suspending_v67_enabled())"), ADD.index("IUnknown_QueryInterface"))

    def test_invalid_input_zeroes_out_token(self):
        self.assertIn("if (!token) return E_INVALIDARG;", ADD)
        self.assertIn("token->value = 0;", ADD)
        self.assertIn("if (!handler) return E_INVALIDARG;", ADD)
        self.assertLess(ADD.index("token->value = 0;"), ADD.index("IUnknown_QueryInterface"))

    def test_queries_canonical_com_identity_and_releases(self):
        self.assertIn("IUnknown_QueryInterface((IUnknown *)handler, &IID_IUnknown", ADD)
        self.assertIn("entry->handler = canonical;", ADD)
        self.assertIn("IUnknown_Release(canonical);", ADD)
        self.assertIn("IUnknown_Release(entry->handler);", REMOVE)
        self.assertIn("IUnknown_Release(entry->handler);", CLEAR)

    def test_allocation_errors_and_bounded_registrations(self):
        self.assertIn("calloc(1, sizeof(*entry))", ADD)
        self.assertIn("return E_OUTOFMEMORY;", ADD)
        self.assertIn("suspending_v67_entry_count >= 32", ADD)
        self.assertIn("suspending_v67_entry_count++", ADD)
        self.assertIn("suspending_v67_entry_count--", REMOVE)
        self.assertIn("suspending_v67_entry_count = 0;", CLEAR)

    def test_token_creation_unique_and_removal(self):
        self.assertIn("suspending_v67_next_token++", ADD)
        self.assertIn("*token = entry->token;", ADD)
        self.assertIn("(*cursor)->token.value != token.value", REMOVE)
        self.assertIn("HRESULT_FROM_WIN32(ERROR_NOT_FOUND)", REMOVE)
        self.assertIn("COREAPP67_SUSPENDING_ADD", ADD)
        self.assertIn("COREAPP67_SUSPENDING_REMOVE", REMOVE)

    def test_lock_balancing_and_release_outside_lock(self):
        for method in (ADD, REMOVE, CLEAR):
            self.assertIn("AcquireSRWLockExclusive", method)
            self.assertIn("ReleaseSRWLockExclusive", method)
        self.assertLess(REMOVE.index("ReleaseSRWLockExclusive"), REMOVE.index("IUnknown_Release(entry->handler);"))
        self.assertLess(CLEAR.index("ReleaseSRWLockExclusive"), CLEAR.index("IUnknown_Release(entry->handler);"))

    def test_run_teardown_drains_registration(self):
        self.assertIn("suspending_v67_clear_all();", LIFECYCLE)
        self.assertIn("unhandled_v65_clear_all();", LIFECYCLE)
        self.assertLess(LIFECYCLE.index("COREAPP56_UNINITIALIZE"),
                        LIFECYCLE.index("suspending_v67_clear_all();"))
        self.assertIn("COREAPP67_SUSPENDING_CLEAR", CLEAR)

    def test_no_callback_delivery_claim(self):
        self.assertIn("No OS suspension notification is dispatched", SRC)
        for method in (ADD, REMOVE, CLEAR):
            self.assertNotIn("Invoke(", method)
        self.assertIn("if (!suspending_v67_enabled()) return E_NOTIMPL;", REMOVE)

    def test_other_lifecycle_stubs_unchanged(self):
        seg = SRC.split("static HRESULT WINAPI coreapp_add_Resuming(", 1)[1].split(
            "static HRESULT WINAPI coreapp_get_Properties(", 1
        )[0]
        self.assertEqual(seg.count("return E_NOTIMPL;"), 2)
        self.assertNotIn("return S_OK;", seg)

if __name__ == "__main__":
    unittest.main()
