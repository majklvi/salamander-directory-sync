# Czech Cancel terminology — 0.4.0 development

The Czech plugin consistently uses **Zrušit** for **Cancel**, by user request on 2026-10-21. This includes confirmation/results, Settings, profile naming and management, unsaved-profile changes, transfer progress, the speed-limit dialog and operation-error questions. The English captions and cancellation semantics remain unchanged. The operation-error mnemonic is **&Zrušit**; button geometry is unchanged.

The speed-limit explanation, immediate-deletion confirmation, current Czech guides and Cancel references in development records use the same wording. Historical reports carry a terminology note where their displayed captions were normalized; their test dates and recorded results are unchanged. This is a plugin terminology choice, rather than a claim that the host uses the same Czech label.

## Verification

- The resource-source audit matches all eight English Cancel dialogs to Czech **Zrušit** (ignoring mnemonic markers). **Close / Zavřít** remains distinct. No obsolete Cancel wording remains in tracked plugin sources or Markdown outside the unchanged vendor dependency.
- All five x64 suites pass in Debug and Release: engine 2,216 assertions; copy/recovery/overwrite/delete 6,730; lifecycle 2,451; English resources 5,516; Czech resources 5,656. Resource checks include caption metrics and geometry at 96, 120, 144, 192 and 240 DPI; lifecycle checks retain Stop/Close coverage.
- Plugin and both language modules build in Debug Win32/x64/ARM64 and Release x64. ARM64 is compilation coverage only.
- Initial sandboxed builds failed or timed out while linking PDB information (`MSPDB140.DLL`); running the same build scripts outside the sandbox succeeded without source or toolchain changes.
- `git diff --check` passes. No new live GUI, SMB or SDK testing is claimed.

The checkout also contains earlier, uncommitted progress-frame/path-presentation work. That work is kept outside the terminology commit. Cloud checks will verify the committed terminology changes independently.
