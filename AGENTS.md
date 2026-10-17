# Standalone plugin development rules

- Preserve binary compatibility with the current Samandarin 0.16 SDK. Update the pinned SDK dependency only when explicitly requested; do not require new host interfaces.
- Use wide Win32 APIs and preserve long and Unicode paths. Do not introduce `MAX_PATH` buffers or ACP round trips where wide data is already available.
- Follow Salamander's compact native dialog layout: `MS Shell Dlg` 8, small standard gaps, OK/Cancel, English and Czech localization, and host-controlled appearance. Do not introduce a separate visual style.
- Preserve staging, data/metadata verification, and the remembered Keep originals preference. Directory deletion must be explicitly planned and limited to empty directories; never fall back to recursive deletion after an error.
- Build, test, and package through this repository's `tools/` scripts. Do not depend on the surrounding host checkout, shared host property sheets, a local certificate, or a global output directory.
- Verify all five x64 test suites in Debug and Release, together with the relevant build matrix. An ARM64 build is not a runtime test. Do not claim SMB, live GUI, or actual cloud CodeQL verification without performing the corresponding check.
- Preserve licenses and provenance for vendored files. Do not rewrite dated historical reports as statements about the current state.
- Active development is in `majklvi/salamander-directory-sync`, branch `main`. Check this repository's own CI; do not use the original fork's validation helper. Publish releases or make destructive history changes only when requested by the user.
