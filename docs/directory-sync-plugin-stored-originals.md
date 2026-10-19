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

## Live GUI follow-up — 2026-10-19, 0.3.0 development

This follow-up extends, rather than replaces, the dated 0.2.2 observations above. It uses the private Czech Samandarin 0.16 x64 host at `C:\Samandarin\builds\dirsync-progress-gui-20261019\host-compact`. The installed user instances were left open and untouched. The tested Release x64 plugin is from commit `a65f0682df81cc5e94e4ca9b343d681d8d65a43b`, 713,728 bytes, SHA-256 `3c1af1c1aa4732c63fb0740bb6e155f2dc106dde0bbbb752a583e94b1582ec82`. No production-code change was needed for the outcomes below.

### Isolated fixture and checks

The local NTFS fixture is `C:\Samandarin\builds\dirsync-stored-gui-20261019`. Genuine overwrite/deletion manifests, journals and saved data were copied from the earlier combined-operation test and their absolute paths rebased to this new fixture. The two original roots exist but contain no files. Thus directory-opening checks cannot accidentally launch an original document. The recovery root is `recovery-漢字`; its overwrite run contains one 18-byte file and its deletion run contains an 11-byte file and two directories. Preferences reads this unsaved draft recovery path; Keep originals was OFF for the normal browsing checks.

| Check | Observed result |
| --- | --- |
| Host-controlled light appearance | Both lists, headers, selected rows, native menus and buttons are readable. Two runs load with the expected complete-manifest, saved-copy and replaced/deleted states. The deletion run shows all three entries, with blank file-size cells for directories. |
| Item context menu | Four explicit commands distinguish copying original/saved paths to the clipboard from opening original/saved directories in Explorer, with a separator between the groups. Both path-copy commands were selected from the menu using keyboard navigation; pasting into a new blank Notepad document preserves the full Unicode paths. |
| Directory-open commands | The corresponding bottom buttons open exactly the existing original parent `on\right-漢字` and the saved file's `right\1\data` directory in separate Explorer windows. These buttons and the item menu dispatch the same `StoredCommand` handlers. The test Explorer windows were then closed. This verifies the handlers, not a separate live activation of both menu entries. |
| Missing original location | Opening the deletion file's absent original parent displays the localized unavailable/reparse-location message. Dismissing it returns to a usable browser; no directory is created. |
| Refresh after an external change | Rename only the synthetic saved `overwrite.txt` to `overwrite.txt.held` outside the GUI. **Refresh** changes copy availability to **Missing / unavailable**, while the recorded overwrite result remains unchanged. Rename it back and Refresh restores **Saved according to record**. The selected run is retained. |
| Newly added run | Add a header-only `run-refresh` manifest externally. Refresh changes the run count from two to three, labels the new run **Incomplete**, and retains the previous overwrite run even though its row index changes. |
| Resizing | Maximize the browser from 752 × 507 to the 2286 × 1099 work area and restore it. Lists expand, summary/buttons stay at the bottom, Close stays on the right, and the selected run and three deletion items remain intact. This is maximize/restore coverage, not continuous-drag or multi-DPI coverage. |
| Close during loading | A separate `loading-synthetic` catalog has 128 run directories with hardlinked, approximately 7 MiB newline-padded manifests. The GUI visibly shows **Loading recovery records…**, empty lists and disabled Refresh/path buttons. Click **Close** while this state is visible: the next capture shows responsive Preferences, with no browser or error dialog. The action plus capture took 545 ms; this is not a precise worker-latency measurement. Repoint the unsaved draft at the normal catalog and reopen it: both genuine runs and their entries load normally. |

The padded catalog deliberately prolongs parsing without a source-code delay or a test hook. It is a cancellation fixture, not evidence that all its synthetic records are valid recovery runs. No synchronization, restoration or backup removal was started.

### Automation boundary and remaining narrow checks

Computer Use exposes the owned browser under its disabled Preferences owner rather than as a separately targetable window. Some subsequent keyboard/menu actions activate that owner and dismiss the popup or leave the list without keyboard focus. The two successful menu-copy results above were verified by their pasted contents; a previous unrelated clipboard value was not accepted as evidence. Upper-list menu labels and separation were inspected, but execution of its three commands, separate activation of the two item-menu directory-open entries, and F5 with actual list focus remain **unconfirmed in the live host**. Refresh through its button and both directory-open handlers did pass. No plugin change was made solely to accommodate automation targeting. These remaining input checks must not be described as passed on the strength of shared handlers or source inspection.

### Read-only evidence and cleanup

`before.json` and `after.json` in the isolated fixture confirm unchanged lengths and SHA-256 hashes for all six baseline manifest/journal/data files after the temporary rename was reversed. The intentionally added `run-refresh` is separate from that baseline. The original roots still contain zero files. `clipboard-paths.txt`, saved from the test Notepad, contains the two full paths used to validate menu copying. These local fixtures and evidence are outside Git.

Preferences changes were cancelled, the browser/preview/private host were closed normally, and the private host's original file-backed configuration was restored byte-for-byte after confirming its process had exited. The temporary light theme therefore does not replace its earlier test configuration. No installed user host or existing editor document was changed. No SMB test or SDK change was performed.

Both existing x64 test configurations were rerun with `tools/test.ps1`: all five suites passed in Debug and Release (engine 2,216; copy/recovery 4,460; lifecycle 2,369; English resource/layout 5,147; Czech resource/layout 5,277). This follow-up changes documentation only; it does not rebuild or replace the tested binary. Cloud build-matrix, test, package and CodeQL results must be checked for the published documentation commit separately.
