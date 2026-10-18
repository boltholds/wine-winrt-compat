# v59 — guarded CoreApplication factory IInspectable::GetIids

## Evidence from the real application's v58 replay (2026-10-18)

The bounded replay `replay-v58-20261018T101257Z-40826.zip` has an independently verified `rollback.json` with `exact_restoration: true`. Its Wine log shows:

```text
18366 COREWINDOW55_CREATED hwnd=... thread=36
18367 COREAPP56_WINDOW_CREATE hr=0
18368 COREAPP57_CALL_GET_ISMAIN
18369 COREAPP58_ISMAIN_SUCCESS ... value=1
18384 COREAPP50_PRIVATE_QI_OK
18396 activation_factory_GetIids ... stub!
18410 RoFailFastWithErrorContextInternal2 0x80004001
18411 RaiseFailFastException
18412 Unhandled exception code c0000602
```

The first v58 main-view incompatibility was resolved, but the real XAML
`IFrameworkView.Initialize` did not return. Unlike v57, this run fails *after*
`IsMain` succeeds. The only observed GetIids stub immediately preceding the
`E_NOTIMPL` fail-fast is the CoreApplication factory's GetIids. This is
a causally strong **candidate**; the follow-up v59 application replay is needed
to establish whether the failure clears.

## Implementation

With `WINE_COREAPP_GETIIDS_V59=1`, `activation_factory_GetIids`:
- validates both out pointers and initializes them to `0` and `NULL`;
- returns a newly allocated, caller-owned IID array via `CoTaskMemAlloc`;
- advertises `IID_IActivationFactory` (always queryable), plus
  `IID_ICoreApplication` only if `WINE_COREAPP_EXPERIMENT=1`, matching the
  existing QueryInterface opt-in;
- excludes `IUnknown`, `IInspectable`, the COM-only `IAgileObject`, and
  the private unmetadataed v50 interface;
- emits `COREAPP59_GETIIDS_SUCCESS` only for actual success;
- preserves `E_POINTER` for missing output pointers, `E_OUTOFMEMORY` on
  failed allocation, and the original `E_NOTIMPL` when the v59 flag is off.

The caller must release the array with `CoTaskMemFree`.
This matches the published WinRT `IInspectable::GetIids` ownership contract:
https://learn.microsoft.com/en-us/windows/win32/api/inspectable/nf-inspectable-iinspectable-getiids

Other methods remain untouched: there is **no** fake GetRuntimeClassName,
GetTrustLevel, activation callback, license/package spoofing, native parity
claim, or assertion that the Shapr3D UI now launches.

## Verification

GitHub Actions: static contracts for v58 and v59, Wine 11.19 PE64 cross-build
for `twinapi.appcore.dll`. Live WSL testing requires an additional SHA-pinned
runner which creates a separate detached Wine worktree and uses the existing
bounded v56 transaction/rollback before interpreting a diagnostic result.
The v58 PR remains open and this v59 change is layered on top, not merged
into Wine master. No bundled Microsoft/Shapr3D binaries.
