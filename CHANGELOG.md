# Changelog

Notable changes to **Directory Synchronization for Samandarin**, newest first.
This history was reconstructed from Git commits, published release notes and dated development records.
Dates use `YYYY-MM-DD`; public release dates are the GitHub publication dates.

> [!CAUTION]
> **All releases listed here are ALPHA software with a risk of data loss.**
> No warranty is provided. Test only on disposable copies with an independent, verified backup.
> Recovery copies do not provide automatic rollback. Runtime validation covers local NTFS x64; SMB remains untested.

## [Unreleased]

### 0.4.0 development — interactive errors, stages 1–2

- Add a shared operation/phase/error model, recoverable/fatal classification, batch-local Skip all matching and explicit execution-skipped/dependency-blocked outcome codes. The latter is reserved for subsequent dependent adapters.
- The separate **Copy visible new items…** command offers **Retry / Skip / Skip all / Cancel** for supported unpublished failures. Retry revalidates the remaining plan and restarts only the current file/empty directory with fresh staging; completed copies are not repeated.
- Add a remembered global **Stop at first error** preference, off by default. Recovery, overwrite, deletion and combined synchronization retain stop-first behavior until their later implementation stages.
- Add a compact native English/Czech error question, asynchronous worker/UI decision channel and cancellation while awaiting an answer. Keep host panels and Stop/Close available; exclude decision waits from transfer-rate accounting.
- Summaries distinguish execution skips and incomplete batches. CSV/text operation exports add attempt count, last error and automatic-skip fields for new copies without falsely marking skipped items complete.
- Keep verification mismatch, stale plans, published failures and staging residue fatal. Preserve existing staging, metadata/data checks and the SDK. See the [implementation and tests](docs/directory-sync-plugin-errors-040.md).

## [0.3.0] — 2026-10-19

- Complete local x64 0.3.0 package installation checks in an isolated Samandarin 0.16 developer build: clean configuration, English/Czech loading, registration after restart, a new Unicode profile, and an in-place update from the published 0.2.1 package. All 13 saved plugin values, including both old profiles and their selection, remain identical after Czech and English restarts. See the [distribution report](docs/directory-sync-plugin-distribution-030.md).
- Translate the remaining Cancel buttons in the Czech profile manager and profile-name dialog to Storno.

### Added

- Use **Sessions / Relace** for groups of stored-originals records. Update the session menu, counts and current English/Czech guides; preserve existing `run-*` paths and manifest schema. See the [terminology analysis](docs/directory-sync-plugin-terminology.md) for the distinction from profiles, window lifetime and complete synchronizations.
- Match the host's native speed-limit dialog: compact two-radio layout, B/s through GB/s units, OK/Cancel/Help, and exact byte-rate loading. Retain the plugin's per-batch 64-bit limit and avoid overlapping Czech labels. See the [source comparison and verification](docs/directory-sync-plugin-speed-dialog.md).

- Open the synchronization preview as an independent modeless window. Keep the host panels available while the plugin is open, comparing or transferring files; reopening the command activates the existing preview. Local confirmations/settings are owned by the plugin window. Keep captured paths fixed, retain safe Close/Stop and join workers before forced unload. Route dialog keyboard navigation on the host UI thread without requiring an SDK extension.

- A compact native transfer-progress window for synchronization, copies, overwrites, saved originals and deletions. Shows source/target, current phase, file transfer/verification progress, completed steps including recovery saves, transferred/read bytes and elapsed time. Stop, Escape or closing this window requests safe cancellation and waits for the worker.
- Match the host copying dialog's compact 355 × 110 layout and use its actual SDK progress controls with centered percentages and host colors. Consolidate transfer details into one status line with a complete tooltip; use the native Cancel/Storno caption.
- Use the host's native split button for **Pause/Resume**, beside **Minimize** and **Cancel**. Its menu provides Pause/Resume and a per-batch speed limit for copied main data, named streams and recovery copies; verification reads remain unthrottled. Cooperative pause and cancellation retain the existing safe completion/journal boundaries. Minimize gives the progress window its own taskbar entry; the preview remains locked and the plugin does not join the host's queue.
- Show copied data, speed and estimated remaining time with the host's transfer-size formatting and local decimal separator. Weight total progress by copying and verification work, with a 4 KiB minimum per step. Named streams are not scanned in advance, so totals are approximate and remain below 100% until the batch finishes; detailed counts, verification reads and elapsed time remain in the tooltip.
- Stored-originals item menus explicitly distinguish copying paths to the clipboard from opening directories in Windows Explorer.
- A read-only **Stored originals…** browser in Settings, available even when Keep originals is off. Select a recovery run to inspect original/saved paths, recorded sizes, copy availability and recorded overwrite/deletion results. Double-click for full details; copy paths or open their directories. Refresh reads the current recovery directory again.
- Keep incomplete, damaged and unsupported records visible with warnings. Missing or differently sized copies are distinct from recorded operation results. Reading records does not verify file contents, restore files, repair logs or delete backups.
- Accept ordinary 8.3 aliases of the recovery directory by resolving the root consistently with the manifest writer; retain reparse checks. A regression reproduces the short TEMP-path case discovered by cloud CI.
- Export all comparison rows and the last confirmed operation/combined batch as UTF-8 CSV or text. Include hidden/Skip rows, actual scan options, current and execution-time filters, sources/targets, saved originals, errors, and distinct recorded-completion/filesystem-change fields. Keep unreported steps explicitly unknown and export available after a plan becomes stale. Stage report writes outside the comparison/recovery trees; confirm replacing existing reports through the native Save dialog.

### Verification

- Complete the remaining Stored originals menu and F5 checks with user-operated GUI input after reproducing the automation's disabled-owner focus limitation. Verify session/manifest clipboard paths, all three directory-open menu entries and F5 after an external copy change. Add a native owned-dialog keyboard-routing regression; do not alter production ownership for the automation helper. See the [completion report](docs/directory-sync-plugin-stored-originals.md#user-assisted-completion-and-terminology--2026-10-19).
- Live Stored originals follow-up in the Czech/light host: item-menu copying of full Unicode paths, directory-open handlers and missing-location feedback, Refresh after missing/restored copies and an added incomplete run, maximize/restore, and Close during visible catalog loading followed by successful reopening. Baseline recovery files remain unchanged. The [follow-up report](docs/directory-sync-plugin-stored-originals.md#live-gui-follow-up--2026-10-19-030-development) explicitly retains the narrow menu/F5 input checks limited by automation focus.
- Live modeless synchronization on isolated local NTFS: host-panel navigation during copying, Pause/Resume, changing the active speed limit from 4 to 8 MB/s, successful verification of all three targets, Stop from a paused copy, and preview Close during copying. Completed targets remain; unpublished staging and subsequent work are absent after cancellation. See the [modeless-window report](docs/directory-sync-plugin-modeless.md#live-modeless-transfer-verification).
- Suppress the misleading time estimate while speed-limited copying remains: unrestricted verification cannot be extrapolated from the limited copy rate. Keep byte counts, speed and progress visible; cover the estimate policy in both language dialog probes.

- Verify the 0.3.0 modeless lifetime, dialog keyboard routing, host responsiveness, monitor bounds and safe Close/forced unload. A live Czech/dark host remained usable while the preview was open and while switching away from content comparison. See the [modeless-window report](docs/directory-sync-plugin-modeless.md) for exact coverage and limitations.

- Verify the native transfer window with English/Czech resources, long Unicode paths/tooltips, copy/verification bars, Stop/Escape/close and worker teardown. All five x64 suites and the plugin matrix pass; see the [progress-window report](docs/directory-sync-plugin-transfer-progress.md) for automated coverage and remaining live-host checks.
- Live Czech GUI checks in host-controlled light/dark modes confirm the progress window and active-transfer cancellation through Stop and its close button. All four cases preserve the completed file, leave subsequent work unstarted and clean staging. Record the separate large-list automation limitation and preliminary disk-space error in the [progress-window report](docs/directory-sync-plugin-transfer-progress.md).
- Test the 0.2.2 recovery catalog against genuine manifests/journals, incomplete and unavailable records, cancellation, junctions and long Unicode paths. All five x64 suites pass in Debug/Release and the plugin build matrix passes. The [stored-originals report](docs/directory-sync-plugin-stored-originals.md) separates the successful live GUI/path-copy checks from remaining checks after Computer Use state-capture timeouts.
- Verify combined new-copy/overwrite/deletion batches in the live GUI with recovery ON and OFF, plus Stop (ON) and Close (OFF) during a large-file transfer. Confirm later phases remain unstarted, completed copies remain, staging is cleaned, and recovery contents match the selected mode. Record the separate large-list automation limitation and exact coverage in the [GUI verification report](docs/directory-sync-plugin-combined-gui.md). No production code or SDK change was required.

### Build size

- Enable Release linker removal of unused code/data and folding of identical COMDATs while preserving external PDBs, speed optimization and runtime safety checks. The x64 plugin shrinks from 1,029,632 to 713,216 bytes (30.7%) with unchanged exports.
- Remove 21 obsolete messages from each language module without renumbering live resource IDs. The three binary modules together shrink from 1,144,320 to 812,032 bytes. Packaging logs now show individual module sizes. See the [size review](docs/directory-sync-plugin-binary-size.md) for measurements and retained safeguards.

## [0.2.1] — 2026-10-19

### Added

- Copy physically empty directories in either direction through **Copy visible new items…**, alongside new files. Visible filters and Skip apply; progress and Stop use the existing transfer worker.
- Import and export named profiles through **Manage profiles…**, using versioned `.dsprofiles` files. Import adds profiles with new local IDs and numeric suffixes for duplicate names; it preserves existing profiles and the active selection and never starts an operation.
- **Execute visible plan…** combines new copies, overwrites and asymmetric deletions under one confirmation. The summary shows counts, directions, data sizes and the frozen recovery preference.
- Ask whether to **Save / Don't save / Cancel** when closing a modified saved profile. Invalid inputs or a failed save keep the window open; unchanged profiles close without a question.

### Changed

- Combined execution creates new targets first, overwrites existing files second, and deletes right-only files followed by empty directories deepest first. The first error or Stop ends all remaining phases; completed changes stay in place.
- Validate pending source/target identities and metadata between phases without treating completed writes as external changes or adopting new proposals. Reject an invalid recovery-directory location before any writes when destructive phases need recovery.
- Empty-directory copies use staging and verification, inherit destination permissions, and preserve supported attributes and modification time. Existing targets are never replaced by the new-copy command; directory ADS are refused.
- Extend English/Czech guides, operation explanations and regression coverage for profile transfer, directory copies and combined execution.

### Fixed

- Correct the architecture suffix in Windows file-version resources for x64 builds; packaging now verifies both the actual PE architecture and the x64 version label.

Published as an **ALPHA** prerelease with a ready-to-install, unsigned x64 package and English/Czech language modules and guides. Test only on disposable copies with an independent backup; completed operations are not rolled back.

## [0.2.0] — 2026-10-18

### Added

- Named synchronization profiles storing directory pairs, masks, comparison options and result filters.
- A compact profile manager with a complete saved-settings preview, Save current, Save as, Rename and Delete, plus a card/star menu button beside the directory inputs.
- Remember the last selected profile across plugin openings and host restarts. Loading a profile never starts comparison or synchronization; recovery and appearance remain global.
- A dirty-profile marker in the window title and confirmation before switching away from unsaved inputs or current results.

### Changed

- Read the host's enabled/disabled timestamp tolerance and configured seconds for each comparison. Enabled tolerance uses inclusive whole-second comparisons; disabled tolerance requires exact timestamps. Retain the legacy three-second fallback when the host setting is unavailable.

The host's separate daylight-saving-time adjustment is not exposed by the current SDK and is not implemented by this change.

## [0.1.6] — 2026-10-18

### Changed

- Disable proposal-menu commands that cannot change any selected eligible row, including restoring an already-default proposal or marking an already-selected copy direction. Mixed selections retain useful commands.
- Display an item's name only on the side where it exists; paired entries show both names. File/directory conflicts retain the correct type on each side.
- **Package refresh, 2026-10-18:** add Michal Várady, the project URL and shorter English/Czech descriptions to Plugin Manager/About and author details to binary properties. The release package was refreshed from commit [`1b156f5`](https://github.com/majklvi/salamander-directory-sync/commit/1b156f5410bec508f15f3dae8a47be34e9bf61a0); the original release tag was retained.

## [0.1.5] — 2026-10-18

### Added

- Optional **Color row text** using the configured operation colors, remembered across sessions. Skipped rows remain gray and highlighted rows retain native selection colors.
- Context-sensitive explanations in the left status pane while choosing synchronization operations, including visible-plan scope and the current recovery choice.

### Changed

- Update English/Czech user documentation for row colors and operation help.

## [0.1.4] — 2026-10-17

**First public alpha release**, including the features developed in 0.1.0–0.1.3 below.

### Added

- Separate native status-bar panes for row counts and transfer/comparison metrics instead of one long delimiter-separated string.
- A standalone development repository with preserved plugin history, documentation and migration provenance; pinned SDK/support snapshots; independent build, test and package scripts; and build/test/package and CodeQL workflows.
- Ready-to-install, unsigned x64 binaries with English/Czech language modules, installation instructions, pilot guidance and SHA-256 checksums.
- An English project README and development rules, retaining the Czech README as `README.cs.md`.

### Changed

- **Package documentation refresh, 2026-10-17:** bundle English and Czech full/pilot guides, keep development records online instead of including a `docs` directory, and verify extracted package hashes and guide links. The refreshed package records commit [`4a09d45`](https://github.com/majklvi/salamander-directory-sync/commit/4a09d45e65a2e4183db99c9998072a2955152d78); the original release tag was retained.

## Before the first public release

The versions below were development milestones, not separate published GitHub releases. Their dates come from the commits that introduced the version numbers. Later development within a milestone is included under that version.

## 0.1.3 — 2026-10-17 (development)

### Added

- Independently optional relative sizes and timestamps for paired files: larger/smaller byte differences and newer/older elapsed-time differences. Absolute display remains the default; comparison and sorting still use actual values.
- Symmetric columns: left name, size and date; operation; right date, size and name. Side names in headings are optional.
- Direct removal of highlighted visible items on the left, right or both sides through the context menu: normally to the Windows Recycle Bin, or permanently when Shift is held while activating the side command. This command has its own confirmation and does not use synchronization recovery copies.

## 0.1.2 — 2026-10-17 (development)

### Added

- Ascending/descending column-header sorting within directory groups, preserving group boundaries and selection.
- Configurable visibility of state, reason, error, size and date columns, while retaining names and the operation column.
- Mirrored file-name columns for easier orientation on both sides.

### Fixed

- Disappearing column headers while scrolling long grouped results.

## 0.1.1 — 2026-10-16 (development)

### Added

- Group comparison results by relative directory, including an explicit root group. File rows show their final name; copied paths remain complete. Filters remove groups with no visible rows, and group headers do not select or execute their contents.

## 0.1.0 — 2026-10-15–2026-10-16 (development)

### Added

- Native directory comparison and a reviewable, initially read-only proposal preview: recursive scanning, masks and exclusions, optional content comparison, timestamp handling, panel-selection scope, and asymmetric mode using the left tree as the source.
- Verified copying to new file targets with sibling staging, supported data-stream/metadata verification, inherited target permissions, progress and Stop.
- Optional, remembered **Keep originals** and a recovery directory outside both compared trees, with verified independent original copies and manifests for manual recovery.
- Existing-file overwrites preserving destination permissions, plus asymmetric deletion of right-only files and explicitly planned directories. Directory removal is nonrecursive and proceeds only when empty; retained, skipped or filtered children protect their parents.
- English/Czech interfaces, a small synchronization icon, host-controlled light/dark appearance, custom operation colors, compact native dialogs and directory pickers.
- Selected-row proposal editing and Unicode path copying, structured operation summaries with clipboard commands, and clearly separated visible-plan execution commands.
- Independent, combinable icon filters that wrap with available width, paired start/Stop controls, a native status bar and a compact results-focused layout.
- Host panel size/name formatting and regional number formatting, long/Unicode path support, clean-configuration registration/restart checks and local NTFS pilot packages.
- Automated and GUI validation of cancellation, forced unloading/shutdown, path changes after scanning, recovery, and disk-space failures, including an isolated NTFS VHD run.

### Fixed

- Keyboard focus during work, filter toggle/repainting behavior on resize, plugin registration reload, tooltip/layout issues, and new-copy behavior with ordinary Modify permissions.

### Changed

- Comparison-option changes apply to the next Compare and preserve current results; path/mask changes invalidate the current plan.
- Keep recovery/appearance preferences global. Completed operations remain after cancellation or an error; require a fresh Compare before another execution.

## Origins — 2026-10-14

- Document the comparison/synchronization design, SDK constraints, recovery approach and verification plan before implementation in the original fork. Plugin development later moved to this standalone repository; the [migration records and commit map](docs/migration/repository-migration.md) preserve that provenance.

[Unreleased]: https://github.com/majklvi/salamander-directory-sync/compare/v0.3.0...main
[0.3.0]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.3.0
[0.2.1]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.2.1
[0.2.0]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.2.0
[0.1.6]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.1.6
[0.1.5]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.1.5
[0.1.4]: https://github.com/majklvi/salamander-directory-sync/releases/tag/v0.1.4
