# v56 — CoreApplicationView + real CoreWindow + bounded FrameworkView lifecycle

This is a **feature-flagged Wine 11.19 experiment**, not a claim of native
Windows/UWP compatibility or an interactive Shapr3D user interface.

v54 validated an ICoreApplicationView/Initialize ABI subset; v55 added a real
hidden Win32 HWND, thread-local CoreWindow, and a bounded CoreDispatcher
message loop. v56 connects these modules into ICoreApplication.Run.

## Lifecycle implemented

For a full experimental opt-in Run:

1. Query IFrameworkViewSource, CreateView, and validate IFrameworkView.
2. Load the v55 test-only WineCoreWindowV55_Create constructor from
   windows.ui.dll. Allocate CoreApplicationView, create a real hidden HWND
   and associate its ICoreWindow with the CoreApplicationView.
3. Register the view in thread-local storage. GetCurrentView returns the
   owned CoreApplicationView only within this thread/lifecycle.
4. Invoke **Initialize → SetWindow → Load → Run → Uninitialize** in order.
5. On return or failure, close any surviving HWND, clear TLS, release
   ICoreWindow/CoreApplicationView and all QueryInterface references.

The five-phase sequence returns S_OK **only when the test view completes
all phases and cleanup without failures**. Errors from Initialize, SetWindow,
Load, Run and Uninitialize are propagated; a later Uninitialize failure
does not replace an earlier failed Run HRESULT.

Test-only opt-in environment:

    WINE_COREAPP_EXPERIMENT=1
    WINE_COREAPP_RUN_PROBE=1
    WINE_COREAPP_VIEW_INIT_V54=1
    WINE_COREAPP_LIFECYCLE_V56=1
    WINE_COREWINDOW_V55=1

With flags off, old v47/v54 behavior is retained. When window support is
absent, Run returns E_NOTIMPL rather than claiming successful activation.

**Important limitations:** native WinRT activation event semantics, actual
package entrypoint Load value, CoreDispatcher async actions, keyboard/pointer
event delivery, DPI transforms and COM cross-apartment marshaling remain
unverified or unimplemented. The Load parameter is an empty HSTRING *for
this bounded mock*, not a verified native Windows contract. CoreApplication
GetCurrentView outside the active lifecycle retains E_NOTIMPL rather than
inventing a Windows-like result.

## Tests run in an isolated Wine prefix

All 10 process-level cases PASS, exit code 0, with mock interface and
source COM refcounts restored to 1:

| Case | Run HRESULT | Lifecycle |
| --- | --- | --- |
| v47_disabled | E_NOTIMPL | CreateView+QI, no Initialize |
| v54_fallback | E_NOTIMPL | Initialize+Uninitialize, no window |
| missing_window_flag | E_NOTIMPL | Constructor rejected, no Initialize |
| complete | S_OK | Initialize, SetWindow, Load, Run message loop, Uninitialize |
| initialize_fail | E_FAIL | No subsequent phase or Uninitialize |
| setwindow_fail | E_ACCESSDENIED | Uninitialize, no Load/Run |
| load_fail | E_INVALIDARG | Uninitialize, no Run |
| run_fail | E_UNEXPECTED | Uninitialize and window cleanup |
| uninit_fail | E_ABORT | All prior phases succeed |
| run_uninit_fail | E_UNEXPECTED | First Run failure preserved |

The complete case uses a real HWND, a separate thread to post WM_CLOSE,
CoreDispatcher.ProcessEvents(ProcessUntilQuit), and verifies TLS is cleared
after completion. Both experimental test PE DLLs were restored exactly:

- twinapi.appcore.dll SHA256
  63747669d8286097ddfc3effe3b36914c99da491124413fcc791276c1baa8a10
- windows.ui.dll SHA256
  7efd0e040d64a504306287cd64c8f67c726c797e09988b246ed335d6690e30e2

The v47 CoreApplication 17/17 regression was also run and passed.

## Release boundary

This code MUST remain on an experimental branch and must not be deployed as
a production Wine UWP host without Windows-native ABI, activation, event and
threading parity. Tests do not alter installed Shapr3D, Windows DLLs, input
security, licensing or package identity. The private test constructor is
not a Microsoft API. The original proprietary application is not included
in this branch or test package.
