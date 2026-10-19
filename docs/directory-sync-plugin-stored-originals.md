# Stored originals — 0.2.2 development

Scope agreed on 2026-10-19: a compact read-only browser for existing recovery records. No automatic restore, backup removal, SMB testing or SDK changes. The subsequent user request also added [comparison/operation report export](directory-sync-plugin-report-export.md) to 0.2.2; its implementation and tests are documented separately.

## Implementation

- Settings contains **Stored originals…**, enabled independently of Keep originals. It reads the current recovery-directory field, including an unsaved draft value.
- Upper list: creation date, run ID, complete/incomplete/damaged/unsupported manifest state. Lower list: original and saved paths, recorded main-stream size, current basic copy availability, independently recorded operation result. Double-click opens the existing compact details dialog.
- English and Czech resources, host theme, native resizable lists, explicit path-copy and directory-open buttons, context menus, Ctrl+C / Ctrl+Shift+C, and Refresh/F5. File contents are never launched.
- A cancellable worker reads records; closing joins it before the dialog's data is destroyed. Refresh retains the selected run when present.
- Strict UTF-8 and flat schema-1 JSONL reader. Retain valid records preceding a partial final line. Do not guess unsupported versions, duplicate IDs or unmatched results. Absolute manifest paths must match the run and original roots; saved paths outside the run are marked unsafe. Reparse traversal is refused for reading/opening recovery locations.
- Only read existing files with shared read/write/delete access. No log repair, directory creation, content verification or writes. A recorded verified save is separate from present-day availability and from the overwrite/deletion journal result. Unknown results are not evidence that an operation did not occur.
- Bound display work to 1,000 runs, 20,000 total entries, 8 MiB per file and 1 MiB per JSONL line. Reaching a limit is visible. Do not attempt further filesystem-hardening features beyond the existing core-level policy.

## Local verification

Automated x64 Debug and Release suites passed. New catalog cases use genuine executor-generated manifests and journals for an overwrite and deletion of a file plus an empty directory. They check recorded results, unchanged manifest/journal/copy contents, missing and differently sized copies, absent operation journals, partial final lines, malformed JSON, duplicate IDs, invalid UTF-8, unsupported schema, missing roots, cancellation, outside-run locations, junction traversal and Unicode recovery paths longer than 260 UTF-16 units.

Compiled English and Czech dialog bounds and caption metrics pass at 96, 120, 144, 192 and 240 font DPI. These are resource/font tests, not live monitor-DPI tests. Plugin build matrix: Debug Win32/x64/ARM64 and Release x64; ARM64 was compiled, not executed.

Live GUI check uses the private Samandarin 0.16 x64 host under `C:\Samandarin\builds\dirsync-combined-gui-20261019\host` and the pre-existing isolated `recovery-漢字` fixture. The user's installed host is untouched. With Keep originals OFF, the Czech dark interface reads two runs: one overwrite and one deletion run containing a file and two directories. The manifest-complete, saved-copy, replaced/deleted states and recorded sizes agree with their records. Double-click opens item details. Ctrl+C copied the full original Unicode path, verified by pasting into a new blank Notepad document.

Ctrl+Shift+C also copied the full saved Unicode path, verified by pasting after the original path in the same blank test document. Returning from Notepad required raising the modal dialog through Computer Use; sending a key to the disabled host owner alone did not establish focus. This is an automation-targeting distinction, not a failed path-copy result.

No new synchronization, restoration or backup deletion is part of this GUI check. The private host's missing crash-reporter warning predates this feature and was dismissed at launch. The context-menu check hit `point ... is outside window bounds` after a tooltip became screenshot 0, followed by `computer-use request timed out: get_window_state`. Fresh window selection succeeded but the state-capture retry also timed out; further UI input stopped. Thus **context-menu commands, directory opening, Refresh, live resizing, light mode and closing during a long read are not yet fully verified in the live host**. The private test browser and new unsaved Notepad document were left open; the user's original editor/host windows were untouched. Do not infer this GUI coverage from automated resource or catalog tests.

Before the subsequent export implementation, local test counts in both Debug and Release were: engine 2,216; copy/recovery/overwrite/delete/catalog 4,057; lifecycle 2,367; English resource/layout 4,482; Czech resource/layout 4,574. All five suites passed. The plugin build matrix passed in Debug Win32/x64/ARM64 and Release x64. Cloud CI was not triggered because this step has not been committed/pushed. Current counts including export are in its separate verification report.

Initial stored-originals-only development package (before export): `out/package-0.2.2-dev-20261019-final/dirsync-0.2.2-x64.zip`, SHA-256 `e5b2c456113c6738d8204dba87cf4d4fd51c32b39949cf44dbb43209255d93b6`. The packaging script verified x64 PE/version metadata, all 14 payload hashes after extraction, the expected file inventory and local guide links. NOTICE explicitly identifies the modified worktree and base commit; this is not a published release. English/Czech user guides and pilot instructions are included; development reports are not bundled. The export report records the replacement development package including both features.

## Limits and next step

This is a snapshot. Refresh after external changes; a missing journal result remains unconfirmed. A complete manifest does not certify that the entire synchronization completed. Basic type/main-stream-size inspection does not detect same-size corruption, ADS changes or metadata changes. Moving a recovery tree leaves absolute records stale. Windows' file browser may reject paths it cannot represent; the full path remains copyable.

No automatic restore or recovery cleanup is implemented. Comparison/results export was subsequently implemented for the same 0.2.2 development version; see the separate [export report](directory-sync-plugin-report-export.md). Restore/cleanup still require a separate design.
