"""v65 source/ABI guards; not a substitute for runtime COM tests."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[3]
SRC = (ROOT / "dlls/twinapi.appcore/core_application.c").read_text("utf-8")
IFACE = SRC.split("typedef struct ICoreApplicationPrivate_v50Vtbl", 1)[1].split(
    "} ICoreApplicationPrivate_v50Vtbl;", 1
)[0]
VTABLE = SRC.split("static const ICoreApplicationPrivate_v50Vtbl private_v50_vtbl =", 1)[1].split("};", 1)[0]
ADD = SRC.split("static HRESULT unhandled_v65_add(", 1)[1].split(
    "static HRESULT unhandled_v65_remove(", 1
)[0]
REMOVE = SRC.split("static HRESULT unhandled_v65_remove(", 1)[1].split(
    "static inline struct factory *impl_from_v50_private(", 1
)[0]


class V65UnhandledABI(unittest.TestCase):
    def test_source_based_on_safe_v58_not_v59(self):
        self.assertIn("COREAPP58_ISMAIN_SUCCESS", SRC)
        self.assertIn("COREAPP57_CALL_GET_ISMAIN", SRC)
        self.assertNotIn("WINE_COREAPP_GETIIDS_V59", SRC)
        self.assertIn("return activation_factory_GetIids(", SRC)
        self.assertIn("return E_NOTIMPL;", SRC.split(
            "static HRESULT WINAPI activation_factory_GetIids(", 1
        )[1].split("static HRESULT WINAPI activation_factory_GetRuntimeClassName(", 1)[0])

    def test_native_slot11_and_12_positions(self):
        # The first 6 IInspectable methods remain identical.
        entries = re.findall(r"^\s*private_v\d+_\w+,?\s*$", VTABLE, flags=re.M)
        self.assertEqual(len(entries), 13)
        entries = [x.strip().rstrip(",") for x in entries]
        self.assertEqual(entries[:8], [
            "private_v50_QueryInterface", "private_v50_AddRef", "private_v50_Release",
            "private_v50_GetIids", "private_v50_GetRuntimeClassName",
            "private_v50_GetTrustLevel", "private_v50_UnknownSlot6",
            "private_v50_get_PrivateProperties",
        ])
        self.assertEqual(entries[8:11], [
            "private_v65_reserved8", "private_v65_reserved9", "private_v65_reserved10"
        ])
        self.assertEqual(entries[11:], [
            "private_v65_add_UnhandledErrorDetected",
            "private_v65_remove_UnhandledErrorDetected",
        ])
        self.assertIn("IUnknown *handler, EventRegistrationToken *token", SRC)

    def test_unknown_slots_not_fabricated(self):
        for i in (8, 9, 10):
            fn = SRC.split(f"static HRESULT WINAPI private_v65_reserved{i}(", 1)[1].split("}", 1)[0]
            self.assertIn("return E_NOTIMPL;", fn)
            self.assertNotIn("return S_OK", fn)

    def test_guid_matches_real_xaml_callback(self):
        self.assertIn("0xf68bc421,0x6b54,0x559b", SRC)
        self.assertIn("0x9c,0xdd,0x48,0x9a,0xad,0x0b,0xd4,0x1d", SRC)

    def test_opt_in_and_invalid_pointers(self):
        self.assertIn('WINE_COREAPP_UNHANDLED_V65', SRC)
        self.assertIn("if (!token) return E_POINTER;", ADD)
        self.assertIn("token->value = 0;", ADD)
        self.assertIn("if (!handler) return E_POINTER;", ADD)
        self.assertIn("if (!unhandled_v65_enabled()) return E_NOTIMPL;", ADD)
        self.assertIn("if (!unhandled_v65_enabled()) return E_NOTIMPL;", REMOVE)

    def test_queryinterface_and_com_ref_ownership(self):
        self.assertIn("IUnknown_QueryInterface(handler, &IID_eventhandler_unhandled_v65", ADD)
        self.assertIn("IUnknown_Release(verified)", ADD)
        self.assertIn("entry->handler = verified;", ADD)
        self.assertIn("IUnknown_Release(entry->handler);", REMOVE)
        self.assertIn("free(entry);", REMOVE)

    def test_alloc_and_token_symmetry(self):
        self.assertIn("calloc(1, sizeof(*entry))", ADD)
        self.assertIn("entry->token.value = unhandled_v65_next_token++", ADD)
        self.assertIn("*token = entry->token;", ADD)
        self.assertIn("(*cursor)->token.value != token.value", REMOVE)
        self.assertIn("HRESULT_FROM_WIN32(ERROR_NOT_FOUND)", REMOVE)

    def test_thread_safe_storage(self):
        for method in (ADD, REMOVE):
            self.assertIn("AcquireSRWLockExclusive", method)
            self.assertIn("ReleaseSRWLockExclusive", method)
        self.assertIn("SRWLOCK_INIT", SRC)

    def test_does_not_forge_callback_invocation(self):
        self.assertNotIn("IEventHandler_Unhandled", SRC)
        for method in (ADD, REMOVE):
            self.assertNotIn("Invoke(", method)
        self.assertIn("does NOT", SRC)

    def test_existing_private_slot7_not_modified(self):
        self.assertIn("private_v50_get_PrivateProperties", SRC)
        self.assertIn("COREAPP50_PRIVATE_PROPERTIES", SRC)
        self.assertIn("COREAPP50_PRIVATE_QI_OK", SRC)

    def test_64bit_only_guard_is_documented(self):
        self.assertIn("Win64-only", SRC)


if __name__ == "__main__":
    unittest.main()
