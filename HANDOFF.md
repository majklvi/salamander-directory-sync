# Directory Synchronization: project handoff

Snapshot: **22 October 2026**, after publication of **0.4.0 ALPHA**. This is the entry point for continuing development in the dedicated Codex project. Update the current-state sections as work advances; preserve dated test reports as evidence for their own revisions.

## Project identity and reading order

| Item | Value |
| --- | --- |
| Product | Directory Synchronization for Samandarin |
| Repository / default branch | [majklvi/salamander-directory-sync](https://github.com/majklvi/salamander-directory-sync) / `main` |
| Active checkout on this computer | `C:\Samandarin\salamander-directory-sync` |
| Public release | [0.4.0 ALPHA](https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.4.0) |
| Release source commit | `9b767c48cfe32347d00e751b54bc9466be8864c6` |
| Historical archive | `C:\Samandarin\directory-sync-plugin`, `majklvi/salamander:feature/directory-sync-plugin`; not the active development checkout |
| Original Codex project | Samandarin, rooted at `C:\Samandarin` |
| Continuity chat | “Zdokumentuj synchronizační plugin”, thread `01a13c28-188f-7a80-86b5-295334f680f1` |

Read [AGENTS.md](AGENTS.md), this document, [README.md](README.md), the [current English guide](docs/directory-sync-plugin.en.md), [CHANGELOG.md](CHANGELOG.md) and [ROADMAP.md](ROADMAP.md). Use [README.cs.md](README.cs.md) and the [Czech guide](docs/directory-sync-plugin.md) for localized wording. Instructions from the human user take precedence; this snapshot does not authorize future implementation, publication or testing beyond the requested scope.

Current guides and release notes describe current behavior. Older design/test documents deliberately retain superseded defaults, earlier restrictions and smaller test counts. For example, directory deletion is now implemented, and recovery defaults to OFF for new configurations despite older reports saying otherwise. Check dates, source revisions and subsequent decisions before treating an old statement as a current requirement.

## Released behavior

- Unicode/long-path scan, metadata or main-stream content comparison, grouped symmetric results, sorting, combinable filters and editable proposals. A one-sided item's name appears only on its existing side.
- New-file and physically empty-directory copies in both directions; existing-file overwrites preserving destination permissions; asymmetric deletion of right-only files and explicitly planned empty directories.
- Combined synchronization: new items, then overwrites, then deletions, with files before empty directories and directories deepest first. Compact confirmation restricts directions, overwrites and asymmetric deletion; Details is read-only. Separate commands remain on the Synchronize arrow menu.
- Optional verified copies of originals, a read-only stored-originals session browser, and CSV/text exports of comparison and recorded operation outcomes. No automatic restoration, rollback or backup cleanup.
- Modeless main window, native-looking EN/CS dialogs, transfer progress, Pause/Resume, a persistent speed limit, direct Stop and confirmation before cancelling through progress/main-window Close during file operations. Completed changes remain.
- Named profiles, import/export, last-profile selection and Save / Don't save / Cancel for modified profiles. Loading/importing never starts comparison or synchronization. Profiles contain roots, masks, comparison options and result filters; recovery, appearance, error policy, speed limit and directory history remain global.
- Host-controlled light/dark appearance, configurable colors, optional row coloring, optional relative sizes/timestamps and host Show sizes/Show names formatting. Relative display changes presentation, not comparison or sorting semantics.
- Shared persistent history of 20 directories for both root fields. Manual proposal loss is confirmed; comparison checkbox changes apply to the next Compare without discarding existing results.

The existing SDK exposes the host's ordinary timestamp tolerance, read at each Compare; the fallback is three seconds if unavailable. Do not confuse this with hourly/DST difference handling, which is not inherited. Additional host APIs and shared transfer-queue/window integration remain deferred until an appropriate SDK is released and their use is requested.

## Approved decisions and invariants

### Original scope, D1–D5

| Decision | Continuing interpretation |
| --- | --- |
| D1 | Physical local/SMB directory trees were the original target, excluding archives and other plugin filesystems. **Current verified scope is local NTFS x64; SMB testing remains explicitly excluded.** |
| D2 | No synchronization history/database to infer past deletions. Saved profiles, recovery sessions and exported reports are not such a history. |
| D3 | Left-root asymmetric synchronization; no separate exact-mirror mode or automatic deletion inferred by ordinary two-way comparison. |
| D4 | Private recovery directory for originals before overwrites/deletions when enabled. The later user decision makes recovery optional and persistent; new configurations start OFF, and enabling it requires a valid absolute recovery path outside both compared trees and not an ancestor of either. Existing saved choices remain. |
| D5 | Ordinary data and selected supported metadata/ADS, not a complete NTFS clone. Preserve destination permissions on overwrite, verify copied data/metadata and honor explicit special-file limitations. |

The [original design](docs/directory-sync-plugin-design.md), [execution protocol](docs/directory-sync-plugin-execution-protocol.md) and [minimal core adapter mapping](docs/directory-sync-plugin-core-adapter.md) explain the rationale. The user explicitly selected the reliability level of the host's ordinary file operations: retain practical path/identity checks, staging and verification, without building a transactional filesystem backend or promising protection against every concurrent filesystem mutation.

### Interactive execution errors, R1–R5

| Decision | Approved behavior, shipped in 0.4.0 |
| --- | --- |
| R1 | Ask by default for supported recoverable errors; remember a global Stop at first error preference. |
| R2 | Offer Retry / Skip / Skip all / Cancel. Skip all matches operation, phase, site/side and Windows error for this command only; it does not suppress fatal failures or persist into another command. |
| R3 | After Skip, continue independent confirmed operations and later stages. Retained children protect every planned ancestor from deletion. |
| R4 | Recoverable per-item saving errors permit decisions; skipping an original blocks its dependent overwrite/deletion. Shared recovery-manifest or operation-journal write failure ends the whole batch. |
| R5 | Disk shortage before target mutation can be recoverable. Verification mismatch, stale plans, failures after publication, uncertain removal, shared-protocol failures and staging residue are fatal. Never silently adopt changed data or repeat a published operation. |

Retry revalidates the remaining confirmed plan and restarts only the affected unpublished item with fresh staging. Keep originals is frozen for the executing batch. See the [approved policy and research](docs/directory-sync-plugin-error-handling.md) and [combined-command implementation](docs/directory-sync-plugin-combined-errors-040.md).

Additional invariants:

- Execution acts on **visible eligible proposals**, not only highlighted rows. Hidden, manual-Skip, blocked and unresolved rows do not authorize operations. Keep confirmation exclusions distinct from execution skips and failures.
- Directory deletion must find the directory actually empty; never fall back to recursive removal. Reparse points and unsupported entries remain blocked. Hidden/System files are included when matching the mask/scope even when host panels hide them.
- The separate immediate **Remove selected…** command uses highlighted items: normally the Windows Recycle Bin, permanently with Shift at side-command activation. It does not use synchronization recovery. Do not conflate it with asymmetric planned deletion.
- No synchronization rollback, crash replay, automatic restore, cleanup, automatic elevation or implicit administrator retry. Read-only targets are not forcibly changed. By content compares the main data stream, not ADS.
- User-facing dialogs stay compact and native, follow the host theme and support EN/CS. Translate directory as **adresář**, Cancel as **Zrušit**, recovery run/session as **relace**. Use neutral descriptions of external inspiration in repository/release/issue text; preserve third-party attribution and licenses.

## Development and verification

Windows prerequisites: PowerShell 7.3+, Visual Studio 2026 C++ desktop tools (v145, target architecture components) and Windows SDK 10. Build/test/package inputs are self-contained here; the parent host checkout, certificates and dependency downloads are not build prerequisites.

Run from the repository root:

```powershell
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Platform x64
pwsh -NoProfile -File tools/build.ps1 -Configuration Debug -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Debug
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Release
pwsh -NoProfile -File tools/package.ps1 -OutputDirectory out/package-handoff-01
```

The package output directory must be new. Plugin modules go to `out/bin/Release_x64/dirsync`; logs, test results and packages stay under ignored `out/`. The five runtime suites cover engine, copying/operations, lifecycle and EN/CS UI resources. Tests touching files must use isolated fixtures; live GUI claims require actual Computer Use, not just resource tests or shell probes.

Own CI: [build/tests/package](.github/workflows/ci.yml) builds Debug Win32/x64/ARM64 and Release x64, runs all five x64 suites in Debug/Release, then packages; [CodeQL](.github/workflows/codeql.yml) analyzes C/C++ security-extended. Win32/ARM64 compilation is not runtime verification. Check the exact pushed commit, inspect failures and report pending checks honestly. Do not use the original fork's internal-draft helper for this standalone repository. A successful CodeQL workflow does not mean zero open alerts; see the [reviewed migration findings](docs/migration/repository-migration.md).

Useful source map:

| Files | Responsibility |
| --- | --- |
| `src/engine.cpp`, `engine.h` | Scan, comparison, proposals and plan checks |
| `src/dirsync.cpp`, `previewlayout.h`, `displayformat.h` | Plugin/host lifecycle, native list, configuration, layout and display |
| `src/copy.cpp`, `copy.h`, `recovery.h`, `overwrite.h`, `delete.h`, `sync.h` | Transfer adapter, preservation and operation stages |
| `src/operation_error.h`, `operation_error_ui.inc` | Decision/outcome policy and question UI |
| `src/sync_confirmation.inc`, `transfer_progress.inc`, `transfer_speed.inc` | Confirmation and transfer controls |
| `src/report.cpp`, `report.h`, `report_ui.inc` | Recorded outcomes, summaries and exports |
| `src/recovery_catalog.cpp`, `recovery_catalog.h`, `recovery_browser.inc` | Read-only recovery records/browser |
| `src/profiles.h`, `src/lang/`, `tests/`, `build-support/`, `vendor/` | Profiles, localization, tests, standalone projects, pinned dependencies |

## Release and acceptance evidence

[Release notes](docs/releases/0.4.0.md), [changelog](CHANGELOG.md), [compiled x64 ZIP](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.4.0/dirsync-0.4.0-x64.zip), [checksum file](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.4.0/package-sha256.txt).

Published ZIP SHA-256: `344468a1275bbc5e8ed7317f418921cfc6a97a567cfc0b5edac6dc70172f719d`. It contains the GUI-tested SPL and both SLGs, four EN/CS user guides, licenses, NOTICE and `files.json`; 15 files total, with no development `docs/` tree or SDK. Publication refreshed guides/provenance after distribution acceptance; therefore the published ZIP differs from the earlier candidate ZIPs, while binary module hashes remain identical.

Both release-tag workflows completed successfully: [build/tests/package](https://github.com/majklvi/salamander-directory-sync/actions/runs/39689428569), [CodeQL](https://github.com/majklvi/salamander-directory-sync/actions/runs/39689428585). These are release evidence, not results for later commits. Public assets were downloaded again and verified against the checksum, manifest and tested module hashes. Keep the prominent **ALPHA / risk of data loss / no warranty** warnings in all release materials; use disposable copies with an independent backup.

Selected completed campaigns; do not repeat them wholesale for unrelated work:

| Evidence | Scope |
| --- | --- |
| [Distribution 0.4.0](docs/directory-sync-plugin-distribution-040.md) | Clean x64 installation and update from published 0.3.0, EN/CS, restart registration, both profiles and all 13 old / 17 new settings; 86 independent checks |
| [Mixed outcomes GUI](docs/directory-sync-plugin-outcomes-gui-040.md) | Summaries, actual CSV/text exports, recovery records and disk state; 201 independent checks |
| [Real NTFS disk exhaustion](docs/directory-sync-plugin-vhd-buffered-040.md) | Current buffered main/ADS adapter, cleanup and interactive Retry with actual full VHD; 149 independent checks; image subsequently detached |
| [Combined-error GUI](docs/directory-sync-plugin-combined-errors-gui-040.md) | Retry/Skip/Skip all, independent stages, parent protection, Stop/Close and recovery ON/OFF |
| [Native error GUI](docs/directory-sync-plugin-native-errors-gui-040.md) | EN/CS light/dark, pending-question host shutdown and actual forced unload/reload |

Known limits include untested SMB, hourly/DST tolerance not inherited, no live cross-monitor DPI acceptance, and large-list responsiveness. User testing covers host 0.15.1; AI-automated GUI tests use an unreleased 0.16 custom developer build. Do not claim a public 0.16 release or support guarantees beyond this evidence. The upstream announcement [issue #778](https://github.com/KRtkovo-eu-AI/salamander/issues/778) was updated to 0.4.0 with package links and the alpha warning; edit it only when requested.

## Local resources and project transition

These paths are machine-local, not dependencies supplied by Git:

- `C:\Samandarin\builds`: isolated fixtures, private host installations/configurations, probes and GUI evidence. Particularly `dirsync-040-install-20261022`, `dirsync-040-release-20261022`, `dirsync-outcomes-gui-20261021` and `dirsync-vhd-040-20261021`. VHD testing requires user-assisted administrator preparation/detachment; never assume a previously used drive letter is still the test volume.
- `out/releases/0.4.0-publish-20261022`: published ZIP/checksum and release body. `C:\Samandarin\builds\dirsync-040-release-20261022` also retains package/publication verification JSON and CI evidence.
- `out/list-analysis-20261020`: baseline harness/results from **0.3.0**, not current 0.4.0 performance evidence. At 100k synthetic rows it measured roughly 7–8 seconds rebuilding the native list and 10 seconds matching successful copy results. These are isolated baseline operations, not total synchronization times or measured optimization gains.
- Three pre-existing **untracked** local analysis reports: `docs/directory-sync-plugin-performance-analysis.md`, `docs/directory-sync-plugin-lazarus.md`, `docs/directory-sync-plugin-lazarus-host.md`. They are not part of a fresh clone or this documentation commit. Preserve them; do not silently add, discard or treat them as published evidence. Essential performance context is summarized here so this handoff does not depend on those files being available.

To complete the Codex organizational move, add a local project **Directory Synchronization** with this checkout as its primary folder, then move the existing continuity chat through the sidebar. This document does not perform that UI move. Verify the resulting working directory, origin, branch and discovered AGENTS instructions before development. Keep the original Samandarin project and its other chats. Attach `C:\Samandarin\builds` as a secondary folder if the new project's access scope needs it; retain files in place.

Git preserves file history, the existing chat preserves discussion history, and this document plus AGENTS/current docs preserve context for future chats. Moving a chat is not a promise that every earlier message is simultaneously in the model's active context. Do not edit Codex internal databases or session files to relocate it. The [migration record and commit map](docs/migration/repository-migration.md) already connect the plugin's exported history to the original fork; no new export/rewrite is needed.

## Recommended next task; not implemented by this handoff

First refresh the large-list baseline on released **0.4.0**, then optimize report-result indexing in `src/report.cpp` before larger native-list changes. Compare repeated timings at 1k/10k/50k/100k rows and preserve exact outcomes for completed/skipped/failed/cancelled batches. Next consider reusing sorted model order and avoiding complete native item/group reconstruction for order-only changes; preserve grouping, filters, selection, focus, scroll position and manual proposals. Never remove path checks, staging or verification for speed. No current 0.4.0 speedup has been measured or implemented.

Individual stored-original restoration is another planned feature; agree destination/collision rules before coding. Batch/automatic restore and backup cleanup remain deferred. Shared host transfers and extra configuration APIs require a suitable released SDK and explicit authorization. See [ROADMAP.md](ROADMAP.md). No new version, performance implementation, SMB campaign, SDK change or release is authorized merely by preparing this handoff.
