# Binary-size review — 0.3.0 development

Date: 2026-10-19. Baseline source: `2b314302070f3e5a6a97798ce00af7e0996c764d`.
Local MSVC 14.51.36231, Release x64. No SDK/vendor changes or SMB tests.

## Result

| Module | Before, bytes | After, bytes | Saved, bytes |
| --- | ---: | ---: | ---: |
| `dirsync.spl` | 1,029,632 | 713,216 | 316,416 (30.7%) |
| `lang/english.slg` | 56,832 | 48,640 | 8,192 |
| `lang/czech.slg` | 57,856 | 50,176 | 7,680 |
| All three modules | 1,144,320 | 812,032 | 332,288 (29.0%) |

The original plugin was 1.030 decimal MB, or about 0.982 MiB; the optimized plugin is 696.5 KiB. These are actual file sizes, not compressed ZIP sizes, object-file sizes, mapped memory or PDB sizes. The language resources are separate modules, so their cleanup does not account for the SPL reduction.

## Main finding: disabled linker elimination in Release

`common.props` enabled external debug information (`/DEBUG`) for Release but did not explicitly set `/OPT:REF` or `/OPT:ICF`. The actual link command in the baseline build log confirmed this. With `/DEBUG`, Microsoft's linker defaults to `NOREF,NOICF`, retaining otherwise removable COMDATs. This was the main avoidable cost.

The Release-only property group now sets `OptimizeReferences=true` and `EnableCOMDATFolding=true`. Debug remains unchanged. This removes unreferenced packaged functions/data and merges identical COMDATs. It keeps `/O2`, `/MD`, external PDB generation, C++ exceptions, stack checks, ASLR and DEP. There is no executable compression, resource packing, stripped exception metadata or removed file-safety logic. See Microsoft's [OPT documentation](https://learn.microsoft.com/en-us/cpp/build/reference/opt-optimizations?view=msvc-170) and [DEBUG documentation](https://learn.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info?view=msvc-170).

PE section measurements (raw on-disk sizes) explain the change:

| Section | Before, bytes | After, bytes |
| --- | ---: | ---: |
| `.text` — code | 849,920 | 590,848 |
| `.rdata` — read-only data | 129,024 | 82,944 |
| `.data` — initialized writable data | 2,048 | 1,536 |
| `.pdata` — exception/unwind records | 30,720 | 19,968 |
| `.rsrc` — icon/version resources | 16,384 | 16,384 |
| `.reloc` | 512 | 512 |

Each file also has 1,024 bytes of PE headers. The smaller unwind section accompanies eliminated functions; exception handling is still enabled. Resources were only 1.6% of the original SPL, so degrading the icon or changing the native UI would not address the main cause.

The original import of `GetPrivateProfileStringW` disappears after elimination. The dark-mode library's INI support is disabled by the existing `_DARKMODELIB_NO_INI_CONFIG`, yet its source translation unit is still compiled. The linker now excludes the unreferenced functions without patching vendored sources or creating a divergent library configuration. SDK helpers used only by tests can likewise remain in source without being retained in the production image. Header-only code and an object file's size alone are not proof of duplication in the final DLL.

## Unused localized resources

Removed exactly 21 obsolete strings in both languages and their `resource.h` definitions. These were the former combined-text progress/confirmation/result messages, replaced by the current status panes and structured dialogs:

- `IDS_SCAN`, `IDS_DONE`, `IDS_SUMMARY`, `IDS_READING`, `IDS_VALIDATING`.
- `IDS_COPY_PROGRESS`, `IDS_COPY_AGAIN`, `IDS_COPY_CONFIRM`, `IDS_COPY_RESULT`.
- The corresponding `PROGRESS`, `AGAIN`, `CONFIRM`, `RESULT` messages under `IDS_SAVE`, `IDS_OVERWRITE` and `IDS_DELETE`.

Source and test references, menu templates, numeric ranges for columns/states/actions/reasons/colors, and localization-test ranges were checked. Live IDs were not renumbered. The removed UTF-16 text payload is 7,740 bytes in English and 7,850 in Czech; actual file savings differ because resource records and PE sections are aligned. Other suspected obsolete messages were left for a separate, explicit cleanup rather than widening this change.

## Experiments and changes deliberately deferred

- Adding `/Gw` on top of REF/ICF produced the same 713,216-byte SPL and identical raw section sizes. It was removed from the final configuration because it provided no measured saving. See [Gw](https://learn.microsoft.com/en-us/cpp/build/reference/gw-optimize-global-data?view=msvc-170).
- `/O2` already includes function-level packaging (`/Gy`) and string pooling (`/GF`), so adding those flags alone is redundant. `/O1` would favor smaller code with a different speed/inlining tradeoff. It was not enabled or measured here: the principal saving does not require changing runtime optimization. See [O1/O2](https://learn.microsoft.com/en-us/cpp/build/reference/o1-o2-minimize-size-maximize-speed?view=msvc-170).
- Whole-program optimization (`/GL` + `/LTCG`) remains a possible separate experiment, not a claimed saving. It would require measuring build time, performance and analysis compatibility with the pinned toolchain. See [GL](https://learn.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization?view=msvc-170).
- The CSV exporter creates a `map<wstring,wstring>` for each record and stores its fixed column names in a dynamic vector. Replacing that with fixed views/direct lookup might reduce allocation work and template instantiations. It is a behavior-bearing refactor and was not needed for the measured saving; any future change should benchmark large exports and preserve byte-for-byte CSV semantics.
- Test fault hooks are already excluded from production. Staging, verification, ADS/ACL/identity checks, recovery, Unicode paths, configuration, dark mode and exported integration callbacks are active features and were retained. No blanket deletion of safety checks or vendored UI components was made.

## Verification and repeatability

The before/after exports are the same three entry points: `SalamanderPluginEntry`, `SalamanderPluginGetReqVer`, and `SalamanderAllowDarkScrollbars`. The third entry is a live UI-library callback, even though it is not listed in the plugin's own DEF file. External PDBs remain available and are not added to the installable package. Release test executables and the lifecycle probe also use the optimized linker settings, exercising the real production callback code under those settings.

Use the repository scripts to reproduce a Release x64 build and package it. `tools/package.ps1` now logs the exact bytes of the SPL and both SLGs on every package run, including CI; it does not impose an arbitrary fixed size ceiling as features evolve. Compare the same compiler, architecture and configuration, and measure modules rather than ZIP compression or the much larger debug build. Compiler updates can change these exact sizes.

Intermediate binaries, original build logs, imports/exports and PE measurements are outside Git under `out/size-review`. The baseline SPL SHA-256 is `3586b181733bd781ef1f9d87e0b11cf216c770d3b98b5339711a24c784e8346c`. The REF/ICF experiment before resource cleanup is `52f9b14920c7d8c388f398ee02023fc60818b65a88d1ee58ca6fed11162f7cce`; the discarded Gw experiment is `fb265edbcfa813ac91f55ea1562cf373f658a4049f52b4a37a3d4e3b9d216caf`. Different PDB identifiers/build times mean equal section sizes need not produce equal hashes.

### Final local checks

All five x64 suites pass in Debug and Release: engine 2,216; copy/recovery/catalog/export 4,460; lifecycle 2,369; English resource/layout 5,075; Czech resource/layout 5,195. This includes copy/verification/recovery behavior, Stop/Close/forced release, the modeless-window keyboard/lifetime probe and EN/CS resource metrics at 96/120/144/192/240 DPI. The plugin matrix passes Debug Win32/x64/ARM64 and Release x64; ARM64 was compiled, not run.

The actual production SPL loads and unloads through `NativeLibrary` and resolves all three original exported names. This is a loader/ABI smoke check, not a live host GUI test. Final SPL SHA-256: `f8ebb31d399675f1743821d82437055aa9b4df849e86718ca76f1b9dcee6f39c`. The external PDB still exists. The package script verifies all 14 payload files and their extracted hashes; the local archive is `out/package-030-size-review/dirsync-0.3.0-x64.zip`, SHA-256 `c83e8d74a926b0d6873cf27fdb730f2d7de1dd40cab6cc4cf5db3b3462647953`.

Version remains 0.3.0. These optimization changes and their package are local and uncommitted; no new GitHub CI/CodeQL run or live host GUI test is claimed. The preceding 0.3.0 publication checks and GUI records remain separate evidence. The pinned SDK, vendored source files, transfer protocols and saved settings were not modified.
