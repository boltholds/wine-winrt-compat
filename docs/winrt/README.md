# Wine WinRT Compatibility

This repository is a **downstream Wine compatibility research fork**, not a separate Wine distribution and not a claim that any particular application already works.

**Upstream:** [wine-mirror/wine](https://github.com/wine-mirror/wine) (a mirror of [WineHQ](https://gitlab.winehq.org/wine/wine)). The `master` branch stays aligned with upstream. Verified experiments are developed as small, reviewable patches in separate branches.

## Scope

- WinRT/COM activation, interface ABI and object lifetime.
- CoreApplication/CoreWindow/dispatcher behavior and application view lifecycle.
- AccessibilitySettings events, PropertySet metadata and SHCore interoperability.
- Reproducible native-Windows vs Wine parity probes for supported API contracts.

Development is based on **observable API contracts**, not on disabling exception handling, spoofing licensing or inventing success HRESULTs. We do not include Microsoft, Shapr3D or other proprietary binaries.

The validated historical Wine snapshot from the isolated laboratory is pinned at [`a312fc87`](https://github.com/wine-mirror/wine/commit/a312fc87a501a447576effe8b02444558f9bbf30) (Wine 11.19) on `baseline/wine-11.19-a312fc87`. Current upstream `master` is newer; experimental patches must be revalidated before being ported.

Read [status](STATUS.md), [development workflow](DEVELOPMENT.md) and the [patch acceptance checklist](PATCH_ACCEPTANCE.md). Machine-readable evidence labels live in [milestones.json](milestones.json).

## Release discipline

This fork is **experimental**. A passing compile or a recorded app launch is not equivalent to an end-to-end working GUI. Only code covered by ABI, negative-path, reference-lifetime and relevant application regression tests is eligible for integration. Unverified prototypes remain isolated.

Wine licensing remains governed by the upstream [LGPL license](../../LICENSE) and [COPYING.LIB](../../COPYING.LIB).