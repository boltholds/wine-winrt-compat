"""v66: static invariants for ICoreApplication2 diagnosis, NOT behavioral parity."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[3]
SRC = (ROOT / "dlls/twinapi.appcore/core_application.c").read_text("utf-8")
IDL = (ROOT / "include/windows.applicationmodel.core.idl").read_text("utf-8")
VTABLE = SRC.split("static const coreapp2_v66_vtbl coreapp2_v66_vtbl_instance =", 1)[1].split("};", 1)[0]
QI = SRC.split("static HRESULT WINAPI activation_factory_QueryInterface(", 1)[1].split(
    "static ULONG WINAPI activation_factory_AddRef(", 1
)[0]


class CoreApplication2Diagnostics(unittest.TestCase):
    def test_guid_grounded_in_existing_wine_idl(self):
        self.assertIn("uuid(998681fb-1ab6-4b7f-be4a-9a0645224c04)", IDL)
        self.assertIn("interface ICoreApplication2 : IInspectable", IDL)
        self.assertIn("&IID_ICoreApplication2", QI)

    def test_seven_domain_methods_and_correct_order(self):
        expected = [
            "coreapp2_v66_QueryInterface", "coreapp2_v66_AddRef",
            "coreapp2_v66_Release", "coreapp2_v66_GetIids",
            "coreapp2_v66_GetRuntimeClassName", "coreapp2_v66_GetTrustLevel",
            "coreapp2_v66_add_BackgroundActivated",
            "coreapp2_v66_remove_BackgroundActivated",
            "coreapp2_v66_add_LeavingBackground",
            "coreapp2_v66_remove_LeavingBackground",
            "coreapp2_v66_add_EnteredBackground",
            "coreapp2_v66_remove_EnteredBackground",
            "coreapp2_v66_EnablePrelaunch",
        ]
        entries = re.findall(r"^\s*(coreapp2_v66_\w+),?\s*$", VTABLE, re.M)
        self.assertEqual(entries, expected)
        self.assertEqual(len(entries), 13)

    def test_opt_in_only_and_unique_embedded_factory_interface(self):
        self.assertIn("WINE_COREAPP2_TRACE_V66", QI)
        self.assertIn("if (IsEqualGUID(iid, &IID_ICoreApplication2))", QI)
        self.assertIn("*out = &impl->coreapp2_v66_iface;", QI)
        self.assertIn("IActivationFactory_AddRef(&impl->IActivationFactory_iface);", QI)
        self.assertIn("COREAPP66_QI_COREAPP2_OK", QI)
        self.assertIn("coreapp2_v66 coreapp2_v66_iface;", SRC)
        self.assertIn("{&coreapp2_v66_vtbl_instance}", SRC)

    def test_no_accidental_v59_getiids_patch(self):
        self.assertNotIn("WINE_COREAPP_GETIIDS_V59", SRC)
        self.assertNotIn("COREAPP59_GETIIDS_SUCCESS", SRC)
        factory_getiids = SRC.split("static HRESULT WINAPI activation_factory_GetIids(", 1)[1].split(
            "static HRESULT WINAPI activation_factory_GetRuntimeClassName(", 1)[0]
        self.assertIn("return E_NOTIMPL;", factory_getiids)

    def test_domain_methods_do_not_fake_success(self):
        for method in ("remove_BackgroundActivated", "remove_LeavingBackground",
                       "remove_EnteredBackground", "EnablePrelaunch"):
            body = SRC.split("static HRESULT WINAPI coreapp2_v66_"+method+"(", 1)[1].split("}", 1)[0]
            self.assertIn("return E_NOTIMPL;", body)
            self.assertNotIn("return S_OK;", body)
        body = SRC.split("static HRESULT coreapp2_v66_reject_add(", 1)[1].split("}", 1)[0]
        self.assertIn("if (!token) return E_POINTER;", body)
        self.assertIn("token->value = 0;", body)
        self.assertIn("return E_NOTIMPL;", body)

    def test_all_calls_instrumented(self):
        self.assertIn("COREAPP66_CALL_%s handler=%p token=%p", SRC)
        for word in ("ADD_BACKGROUND_ACTIVATED", "ADD_LEAVING_BACKGROUND",
                     "ADD_ENTERED_BACKGROUND", "REMOVE_BACKGROUND_ACTIVATED",
                     "REMOVE_LEAVING_BACKGROUND", "REMOVE_ENTERED_BACKGROUND",
                     "ENABLE_PRELAUNCH"):
            self.assertIn(word, SRC)
        self.assertIn("COREAPP66_CALL_GETIIDS", SRC)

    def test_com_identity_refcounts_delegate_to_static_factory(self):
        seg=SRC.split("static HRESULT WINAPI coreapp2_v66_QueryInterface(", 1)[1].split(
            "static HRESULT WINAPI coreapp2_v66_GetIids(", 1
        )[0]
        for value in ("activation_factory_QueryInterface", "activation_factory_AddRef",
                      "activation_factory_Release", "impl_from_coreapp2_v66"):
            self.assertIn(value, seg)

    def test_existing_v65_event_and_cleanup_unchanged(self):
        for token in ("COREAPP65_UNHANDLED_ADD", "COREAPP65_UNHANDLED_REMOVE",
                      "COREAPP65_UNHANDLED_CLEAR", "COREAPP58_ISMAIN_SUCCESS",
                      "unhandled_v65_clear_all();"):
            self.assertIn(token, SRC)

    def test_inspectable_methods_explicitly_unimplemented(self):
        for method in ("GetIids", "GetRuntimeClassName", "GetTrustLevel"):
            segment=SRC.split("static HRESULT WINAPI coreapp2_v66_"+method+"(",1)[1].split("}",1)[0]
            self.assertIn("return E_NOTIMPL;",segment)
        self.assertIn("*iids = NULL;", SRC)
        self.assertIn("*count = 0;", SRC)

    def test_no_activation_or_exception_dispatch_claim(self):
        self.assertIn("NOT an implementation of", SRC)
        self.assertIn("COREAPP65_UNHANDLED_ADD", SRC)
        self.assertNotIn("COREAPP66_EVENT_INVOKED", SRC)


if __name__ == "__main__":
    unittest.main()
