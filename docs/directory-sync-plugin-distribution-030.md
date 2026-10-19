# 0.3.0 x64 package: clean installation and upgrade

Verified on 2026-10-19. This prepares the final local package; it does not publish a GitHub release or tag. The public download remains 0.2.1 until publication is requested.

> [!CAUTION]
> **ALPHA SOFTWARE — RISK OF DATA LOSS. NOT FOR PRODUCTION USE.**
> No warranty is provided. The authors and contributors accept no responsibility or liability for loss or corruption of data. Use disposable copies and a separate verified backup. Recovery copies are not automatic rollback.

## Environment and isolation

- Host: existing custom Samandarin 0.16 x64 developer build, numeric version 5.0.0.197; `salamand.exe` SHA-256 `f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd`.
- Windows, local NTFS, existing system DPI and dark appearance. This is not a new cross-DPI or light-mode sweep.
- Private root: `C:\Samandarin\builds\dirsync-030-install-20261019`, with independent `clean/host` and `upgrade/host` directories, runtime DLLs and matching host English/Czech language modules.
- Each host uses its own `configstorage.ini` and file-backed `config.reg`. The clean host began with a zero-byte configuration, no plugin catalog and no saved plugin registration/settings. No installed user configuration or unrelated host instance was changed or closed.
- Installation was file extraction/copy followed by the host's automatic plugin discovery. The GUI Add command was not repeated. This verifies the unpack-and-register installation path, not a signed installer.
- Only comparison was started in the GUI. No synchronization, removal, SMB test or SDK change was performed in this distribution check. Existing automated suites retain their isolated file-operation coverage.

The private host reports that `salmon.exe` is absent at startup; its warning was acknowledged. As in the earlier clean-host check, the first empty-configuration start showed blank panels; after normal exit/restart the panels populated. Neither prevented plugin registration or use.

## Package contents and identity

Built through `tools/build.ps1 -Configuration Release -Platform x64`; packaged through `tools/package.ps1`. Every executable module is checked for x64 PE type and version **0.3.0 (x64)**. The package contains the SPL, both SLGs, English/Czech README and PILOT guides, licenses, provenance notice and integrity manifest. It contains no SDK, compiler requirement, PDB/import libraries or `docs` directory. The alpha/data-loss warning is retained in all four guides and the notice.

| Module | Bytes | SHA-256 |
| --- | ---: | --- |
| `dirsync.spl` | 713728 | `3c1af1c1aa4732c63fb0740bb6e155f2dc106dde0bbbb752a583e94b1582ec82` |
| `lang/english.slg` | 49664 | `948a66b85ac19598f37fa22b4b7e648e71517edc94cde573ae471c0d89376464` |
| `lang/czech.slg` | 50688 | `900bfc9a8a8036985782e41b7267782df1177ae61d8e6732bb68bad1ed5b0aac` |

The original candidate used the previous Czech SLG. Live verification found two English Cancel captions in the Czech profile manager/name dialog; these are now Storno. Both corrected dialogs were reopened and checked in the upgraded host. The SPL and English SLG did not change.

The corrected test archive is under `out/releases/0.3.0-20261019-install-verified`, SHA-256 `c5d8eccb57ad4a8f3d5a33b37b6acdfa97ab7d151b85aa63feeccb3338aeff2a`. It records the modified test worktree and is evidence, not the final deliverable. Both stopped test installations were brought to its complete extracted payload; all **15 installed files** were independently matched against the extraction (14 manifest entries plus the manifest itself).

The final deliverable is generated after committing this report and the CI timeout adjustment, from the clean source tree, into `out/releases/0.3.0-20261019-final-ci`. Its `package-sha256.txt` records the ZIP checksum; `NOTICE.txt` records the exact source commit. The final archive is extracted again and its three modules must match the GUI-tested hashes above. Documentation/provenance links are pinned to the final commit, so its ZIP hash differs from the test archive without changing the tested modules. The final checksum is kept beside the archive to avoid a self-referential documentation commit.

## Clean installation

1. Extracted the x64 package into `clean/host/plugins/dirsync`; started the private host with an empty file-backed configuration.
2. English Plugins Manager showed one installed/loaded plugin, **Directory Synchronization 0.3.0**, its short description, Michal Várady attribution and repository link. English Preferences opened with defaults: Keep originals on, relative values off, row coloring off.
3. Enabled Color row text and saved. Changed the host language through Configuration → Language to Czech, acknowledged the restart information and exited normally.
4. Restarted. The registered plugin remained available and loaded in Czech, with translated manager metadata and **Barvit text řádků** still on.
5. Compared `clean/left-žluťoučký` against `clean/right-漢字`. The single `new-ž.txt` row showed a right-copy proposal and Czech completion status; no operation was executed.
6. Created **Čistá instalace – 漢字** in profile management, selected it, closed the preview and exited the host normally.
7. Restarted with the corrected package payload. Opening the plugin restored that selected profile, both full Unicode paths and its mask/options. No comparison or operation started automatically. Closed normally again.

Independent parsing of the two saved configurations found all **13 plugin values identical** across the last restart. `ProfilesV1` has one selected profile (ID 1), flags 1, filters 31 and mask `*.*`; its binary SHA-256 is `ff302a9f0f95524adf5071f6a04f53ec6d8091e092bbcf3f429496602d858b53`.

## In-place update from published 0.2.1

Used the cached published [`dirsync-0.2.1-x64.zip`](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.2.1/dirsync-0.2.1-x64.zip). Its SHA-256 was checked against the GitHub release asset digest: `ce0befc50cafc54139efca0fbbfbb6046cfad90d8f3f7449245641fc55f5993b`.

The isolated upgrade configuration was seeded from the earlier native profile fixture, then **loaded and saved by the actual published 0.2.1 modules** before taking the baseline. It is not a fabricated 0.3.0 configuration relabeled as 0.2.1. Only its host configuration-storage path was redirected to the private test host.

| Profile | Stored mask | Options | Filters |
| --- | --- | --- | --- |
| `Pilot A – žluťoučký 漢字 😀` | `*.*` | Subfolders | all five on |
| `Pilot B – přejmenováno & 漢字` | `*.txt;*.md` | Subfolders + By content | all five on |

Both profiles retain their original Unicode local test roots under `dirsync-profiles-gui-20261018`; profile B (ID 2) is selected. In 0.2.1 Preferences, enabled relative sizes/times and disabled Keep originals, then saved and inspected both profile-menu entries. The loaded manager reported **0.2.1** before normal shutdown and the baseline snapshot.

Replaced the plugin modules in place while the host was stopped, leaving registration and configuration intact. The complete corrected package payload was subsequently installed and hash-checked as described above; no removal/re-add was used.

- **First 0.3.0 start, Czech:** selected profile B, Unicode roots, mask and checked Subfolders/By content restored. Preferences retained action labels on, row coloring off, both relative options on, Keep originals off and the stored recovery path. Both profiles and every option/filter were visible in the corrected Czech manager. A read-only comparison finished with one visible `alpha.txt` row, zero blocked rows.
- **Next start, English:** changed only the host language field in the private configuration while stopped, preserving its byte encoding and plugin values. The loaded manager reported Directory Synchronization **0.3.0** in English. Preferences retained all the same values. Main window, profile menu, profile manager and completed comparison were in English; Unicode profile names and paths were unchanged. No operation or profile load started a transfer automatically.
- The host can show the previous version/localized name in its unloaded catalog until first loading the new plugin. After loading, the metadata updated; normal shutdown persisted version 0.3.0 in `configstorage.ini`. This is cached host metadata, not a lost registration.

After each normal exit, independently parsed the plugin section (joining continued binary-value lines). All **13 name/value pairs exactly matched** the 0.2.1 baseline after both the Czech and English starts: KeepOriginals, RecoveryRootUtf16, ActionLabels, ColorRowsV1, RelativeSizesV1, RelativeTimesV1, SideNamesV1, VisibleColumnsV1, SortColumnV1, SortDescendingV1, LightColorsV1, DarkColorsV1 and ProfilesV1. This includes the existing custom dark color and both complete palette arrays. `ProfilesV1` binary SHA-256 remained `a9ef1fddaa29f0b3b7cb14fd8398ced8fe45a44fb227b33cd0a01c567b937346`.

Local evidence: `upgrade/before-upgrade-021.reg`, `after-upgrade-czech.reg`, `after-upgrade-english.reg`, `clean/after-first-profile.reg`, `after-profile-restart.reg`, `verify-settings.py` and `settings-verification.json` under the private root. These contain synthetic test configuration and are outside Git. The original `alpha.txt` hash still matches its earlier fixture record; the new clean right/recovery directories remain unused.

## Regression checks and limits

All four plugin builds pass: Debug Win32, Debug x64, Debug ARM64 and Release x64. ARM64 is compile-only. All five x64 suites pass in both Debug and Release: engine **2216**, copy/recovery/operation/export **4460**, lifecycle **2491**, English UI **5151**, Czech UI **5279** checks. The Czech resource edit does not change SDK/API or operation behavior.

The first cloud run for `926aa38` passed both test jobs and all Debug builds, but its Release build exceeded the script's 105-second limit while compiling vendored Darkmodelib; it reported no compiler error. Packaging was consequently skipped. The CI plugin-build and package-build calls now explicitly allow 300 seconds per project, within the existing 20-minute job bound. Local defaults and compiler/optimization settings are unchanged. Current-head CI must finish successfully before this distribution task is considered complete.

This is installation/upgrade/localization validation, not a fresh exhaustive synchronization run or a guarantee against data loss. It does not extend coverage to SMB, other host versions, a signed installer, registry-backed storage, another Windows account or a new system-DPI configuration. Existing operation/lifecycle reports remain the evidence for those already tested NTFS scenarios. The guide now explains updating in place, keeping configuration/recovery data and loading the plugin before trusting cached manager metadata.
