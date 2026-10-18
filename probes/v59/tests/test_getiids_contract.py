"""Static v59 ABI/source contracts; live Shapr3D results require a guarded replay."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SOURCE = (ROOT / "dlls/twinapi.appcore/core_application.c").read_text("utf-8")
SEGMENT = SOURCE.split(
    "static HRESULT WINAPI activation_factory_GetIids(", 1
)[1].split("static HRESULT WINAPI activation_factory_GetRuntimeClassName(", 1)[0]
QI = SOURCE.split(
    "static HRESULT WINAPI activation_factory_QueryInterface(", 1
)[1].split("static ULONG WINAPI activation_factory_AddRef(", 1)[0]


class GuardedFactoryGetIids(unittest.TestCase):
    def test_one_correct_entrypoint(self):
        self.assertEqual(SOURCE.count("static HRESULT WINAPI activation_factory_GetIids("), 1)
        self.assertEqual(SOURCE.count("COREAPP59_GETIIDS_SUCCESS"), 1)
        self.assertIn("activation_factory_GetIids,", SOURCE)
        self.assertIn("return activation_factory_GetIids(&impl->IActivationFactory_iface, count, iids);", SOURCE)

    def test_no_opt_in_no_change(self):
        self.assertIn("WINE_COREAPP_GETIIDS_V59", SEGMENT)
        self.assertIn("return E_NOTIMPL;", SEGMENT)
        self.assertLess(SEGMENT.index("return E_NOTIMPL;"), SEGMENT.index("CoTaskMemAlloc("))

    def test_outputs_are_validated_and_initialized(self):
        for expected in ("if (!iid_count || !iids) return E_POINTER;",
                         "*iid_count = 0;", "*iids = NULL;"):
            self.assertIn(expected, SEGMENT)
        self.assertLess(SEGMENT.index("*iids = NULL;"), SEGMENT.index("CoTaskMemAlloc("))

    def test_allocation_and_ownership(self):
        self.assertIn('#include "objbase.h"', SOURCE)
        self.assertIn("CoTaskMemAlloc(count * sizeof(*result))", SEGMENT)
        self.assertIn("if (!result) return E_OUTOFMEMORY;", SEGMENT)
        self.assertIn("CoTaskMemFree", SOURCE)
        self.assertLess(SEGMENT.index("result[0] = "), SEGMENT.index("*iids = result;"))
        self.assertLess(SEGMENT.index("*iids = result;"), SEGMENT.index("return S_OK;"))

    def test_advertised_iids_are_supported_by_queryinterface(self):
        self.assertIn("result[0] = IID_IActivationFactory;", SEGMENT)
        self.assertIn("if (include_statics) result[1] = IID_ICoreApplication;", SEGMENT)
        self.assertIn("&IID_IActivationFactory", QI)
        self.assertIn("&IID_ICoreApplication", QI)
        self.assertIn("WINE_COREAPP_EXPERIMENT", QI)
        self.assertIn("WINE_COREAPP_EXPERIMENT", SEGMENT)
        self.assertIn("count = include_statics ? 2 : 1;", SEGMENT)

    def test_metadata_is_not_fabricated(self):
        self.assertNotIn("result[", SEGMENT.split("IID *result;", 1)[0])
        self.assertNotIn("IID_ICoreApplicationPrivate_v50", SEGMENT)
        self.assertNotIn("IID_IUnknown", SEGMENT)
        self.assertNotIn("IID_IInspectable", SEGMENT)
        self.assertNotIn("IID_IAgileObject", SEGMENT)

    def test_no_false_activation_or_classname(self):
        tail = SOURCE.split("static HRESULT WINAPI activation_factory_GetRuntimeClassName(", 1)[1].split(
            "static const struct IActivationFactoryVtbl", 1
        )[0]
        self.assertEqual(tail.count("return E_NOTIMPL;"), 3)
        self.assertIn("COREAPP58_ISMAIN_SUCCESS", SOURCE)
        self.assertIn("COREAPP57_CALL_GET_ISMAIN", SOURCE)

    def test_old_private_getiids_stays_unimplemented(self):
        tail = SOURCE.split("static HRESULT WINAPI private_v50_GetIids(", 1)[1].split(
            "static HRESULT WINAPI private_v50_GetRuntimeClassName(", 1
        )[0]
        self.assertIn("return E_NOTIMPL;", tail)
        self.assertNotIn("COREAPP59_GETIIDS_SUCCESS", tail)

    def test_v56_rollback_path_is_unchanged(self):
        self.assertIn("COREAPP56_LIFECYCLE_RETURN", SOURCE)
        self.assertIn("if (SUCCEEDED(hr) && FAILED(cleanup)) hr=cleanup;", SOURCE)


if __name__ == "__main__":
    unittest.main()
