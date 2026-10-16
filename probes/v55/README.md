# v55: bounded real CoreWindow + CoreDispatcher prototype

This is experimental Wine 11.19 research, NOT a production UWP host.

- WINE_COREWINDOW_V55=1 enables a real hidden Win32 HWND on the creating thread.
- CoreWindow has an ABI-complete 58-entry COM vtable, with honest E_NOTIMPL
  for unsupported methods (including WinRT events and activation).
- CoreDispatcher has a ten-entry COM vtable, owner-thread access checking,
  and ProcessEvents support for nonblocking and until-quit message processing.
- Private test-only WineCoreWindowV55_Create(ICoreWindow **, HWND *) is NOT
  a Windows API and is never called unless the opt-in flag is set.
- Closing an HWND clears thread-local current CoreWindow, posts WM_QUIT,
  and frees resources once COM references have been released.
- Remote thread Close, get_Dispatcher and ProcessEvents return RPC_E_WRONG_THREAD.
- RunAsync, RunIdleAsync, input suppression, event tokens and full WinRT
  activation remain unavailable; unsupported calls never return fake S_OK.
- Not integrated into CoreApplication.Run or IFrameworkView.SetWindow.
- No real Shapr3D GUI claim; no proprietary binaries are included.

Five separately executed Wine 11.19 smoke cases were PASS:
disabled, basic HWND+COM identity, posted message pump, wrong-thread
rejection, and ProcessUntilQuit closed by cross-thread PostMessage(WM_CLOSE).
The captured markers and SHA of each log are in observations.json.

Original laboratory windows.ui.dll was restored with exact SHA256
7efd0e040d64a504306287cd64c8f67c726c797e09988b246ed335d6690e30e2.

GitHub Actions validates recorded evidence and cross-compiles Wine windows.ui.dll
and the standalone Win64 smoke executable; the CI Linux runner does not
run real Windows COM tests. WinRT CoreApplication integration remains future work.
