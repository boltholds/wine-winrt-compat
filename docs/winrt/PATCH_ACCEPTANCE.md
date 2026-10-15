# Patch acceptance checklist

- [ ] Based on an identified Wine upstream revision; source provenance is recorded.
- [ ] Public/private ABI and any undocumented ordinal were verified on a specific Windows build.
- [ ] Expected success and error HRESULTs are measured and compared.
- [ ] Out parameters and COM AddRef/Release ownership are tested.
- [ ] Apartment/threading and callback lifetime behavior are covered where applicable.
- [ ] No fake success responses, hidden exception suppression, or security/licensing bypass.
- [ ] Affected Wine module builds with warnings reviewed.
- [ ] Existing COM/WinRT smoke tests pass.
- [ ] Application replay does not mutate installed applications or real prefixes.
- [ ] Experimental code is isolated from the upstream-tracking branch.
- [ ] Source licensing and third-party redistribution rights are respected.

A check box may only be checked when an actual test or review supplies evidence. Compilation alone is insufficient.