# v67 — bounded ICoreApplication::Suspending subscription ABI

## Why v67 exists

The user's real guarded v66 replay `replay-v66-20261018T214807Z-92907.zip`
showed:
- `COREAPP58_ISMAIN_SUCCESS value=1`;
- `COREAPP65_UNHANDLED_ADD token=0x1`;
- `COREAPP66_QI_COREAPP2_OK` twice and **no** `COREAPP66_CALL_*` methods;
- actual first failing stub:
  `coreapp_add_Suspending ... handler=0x194BBEB0 token=0x194B8458:
  not implemented`;
- XAML fail-fast `0x80004001`, exception `c0000602`;
- **exact DLL restoration** and no override or journal errors;
- no successful `COREAPP56_INITIALIZE`.

The missing method belongs to the **base** `ICoreApplication` (not
`ICoreApplication2`), as already described by the public
`include/windows.applicationmodel.core.idl` contract.

## Implementation

Layered **directly from v66**, preserving the v58 IsMain, v65 private
unhandled-error subscription, and v66 diagnostic ICoreApplication2.

When **WINE_COREAPP_SUSPENDING_V67=1** only, the public
`ICoreApplication::add_Suspending`:
1. validates and zeroes `EventRegistrationToken *`;
2. validates that the typed handler provides the canonical `IUnknown`
   reference and retains it with COM lifetime ownership;
3. allocates a bounded in-process subscription (maximum 32 for this
   disposable single-view experiment);
4. publishes a positive monotonic registration token only on success;
5. emits `COREAPP67_SUSPENDING_ADD`.

`remove_Suspending` removes exactly the matching token and releases its
owned COM reference outside the subscription lock; it emits
`COREAPP67_SUSPENDING_REMOVE` on success. Any leftover subscriptions
are released on normal bounded Run teardown with
`COREAPP67_SUSPENDING_CLEAR`. A fatal process-level fail-fast may
prevent this teardown from running; the operating system then reclaims
the process. No subprocess outside the owned disposable prefix is changed.

## Important limitations

**No real suspension notifications or delivery of application lifecycle
events.** A successful `add_Suspending` is a tightly scoped initialization
probe and does not establish native WinRT behavior. Nothing changes to
app binaries, licensing, package state, event delivery or Wine master.
The unsafe v59 `GetIids` experiment remains fully excluded.

CI checks v67 ownership/token rules, rechecks v66/v65/v58 contracts, and
PE64-compiles Wine 11.19 `twinapi.appcore.dll`. The next legitimate test
is a separate SHA-pinned worktree, guarded preflight, brief opt-in replay
and exact automatic rollback. It should identify the first method after
the observed `Suspending` subscription; do not guess the next WinRT
contract or claim a working Shapr3D UI.

The code remains Draft and is not production-ready.
