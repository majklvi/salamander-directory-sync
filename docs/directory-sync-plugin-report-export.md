# Comparison and operation-result export — 0.2.2

Current verification: the [0.3.0 live GUI follow-up](#live-gui-follow-up--2026-10-19-030-development) below completes the export-menu/Save-dialog checks that were pending in the original implementation report. The initial 0.2.2 build, package and unpublished-worktree statements are historical evidence, not the current release state.

Implemented on 2026-10-19 after the read-only stored-originals browser, at the user's request. No SMB tests or SDK changes. This is an export of the current comparison and its last confirmed operation, not a recovery/restore feature or a persistent activity log.

## Interface and scope

**Synchronize… → Export comparison and results…**, also at the top level of the results context menu. The native file-save dialog offers CSV/text and confirms overwriting an existing output. Menu help explains that all loaded rows, including hidden/Skip rows, are included; highlighted rows do not restrict export. The command is disabled during work and before the first completed comparison. It remains enabled after cancellation, failure or execution makes the plan stale.

Comparison rows keep their actual scan metadata and proposed actions. Export reads memory without rescanning/revalidating. Record the scan options (including actual tolerance and selected panel items), current filters/profile, pending option changes, state/reason/error, original/current action and all present side metadata. File sizes are exact main-stream bytes, times are UTC plus exact FILETIME ticks. Relative GUI values, sorting and hidden columns do not alter exported values. Items not loaded because of scan exclusions cannot be enumerated by the report.

Before each confirmed operation, freeze its kind, planned rows/targets/sources, visible membership, filters and recovery preference/root. On worker completion, capture results before UI flags/snapshot are cleared. Supports new files/empty directories, save-originals-only, overwrites, asymmetric file/directory deletion, combined execution and immediate recycling/permanent deletion. Main operations and recovery saves have separate records tied to the 1-based comparison row ID. Comparison `visible` is current; `execution_visible` is frozen. A subsequent comparison/path-mask invalidation clears the last batch; closing loses the in-memory report. Existing recovery journals are independent.

`completed=true` requires the executor's success state, publication/removal and no per-step error; a recovery save also requires a logged result. `filesystem_changed` reflects reported publication/removal even if a later error makes completion false. Missing outcomes remain `not_reported` and leave filesystem-change status blank: no inference of rollback or no change. A worker exception makes the batch incomplete. Unstarted later combined phases never become fictitious successes. Record order follows the model/planned rows; it is not a reconstruction of chronological execution.

## Format and output

- Schema 1; stable English field names/codes in both UI languages. One comma-separated CSV table with a `record` column (`metadata`, `comparison`, `batch`, `operation`). Metadata/batch settings use `key` and `value`; row/operation records use the named columns. Empty fields are not zero values. Rows use a 1-based ID for references.
- UTF-8 BOM and CRLF. CSV quotes every field and doubles quotes, including multiline fields. An apostrophe guards text with initial spreadsheet formula prefixes (also after spaces/tabs/newlines). This intentionally changes those textual cell values; it does not execute them. Exact large numerical values should be imported as text in a spreadsheet.
- Text uses sections and `key: value` lines. Backslash/control characters are escaped, keeping a path or profile name from introducing a counterfeit record. UTF-8 preserves Czech and other Unicode characters.
- Only existing output parents, outside both compared roots and the current/batch recovery roots; no reparse-parent traversal. Wide paths, including extended-length paths. No directory creation, source-file writes or plan changes.
- Create an owned unique staging file next to the output, write all data, flush and close it, then rename it into place. A refused/failed replacement preserves the previous report. Stage cleanup is attempted on failure. This is ordinary core-level staging, not a new transactional filesystem protocol.
- First implementation builds/writes the report synchronously with a wait cursor; no export progress/Stop control. Cancel in the Save dialog leaves everything unchanged. No automatic export, upload, restore or backup cleanup.

## Verification

New tests use an independent CSV reader to check field counts/quoting and exact round trips, including quotes, commas, multiline/formula-like profile names and Unicode. Verify hidden/Skip rows, blank absent-side fields, actual byte sizes, frozen execution visibility after changing filters, and comparison-only output with no invented operation records.

Genuine combined executor fixtures test recovery ON/OFF, new files and empty directories, overwrite, file/empty-directory deletion, and separate saved-original paths/results. Existing five deterministic failure/Stop/external-change cases now also validate the report: later deletion phases remain unreported. A synthetic publication-followed-by-error verifies `filesystem_changed=true` with `completed=false`; a missing later outcome exports blank filesystem-change evidence.

NTFS output tests cover UTF-8 BOM, Unicode and paths longer than 260 UTF-16 units, new files, explicitly permitted replacement, refusal/preservation when replacement is not allowed, exclusion of compared/recovery trees, invalid UTF-16, missing parents, relative output, reparse parents and staging cleanup. Invalid scan-input roots do not block exporting diagnostics to a valid outside destination; valid normalized comparison/recovery roots are still excluded. Lifecycle/menu checks cover export before comparison, while running, and after stale/cancelled/failed results. English/Czech resource tests check all new labels/help strings.

Live GUI verification remains incomplete. Computer Use could retrieve the old private host's settings tree, but keyboard targeting kept focus on its disabled owner; attempting the exact modal Close button returned `coordinate input geometry is unavailable`. No claim is made here of a successful native export-menu/Save-dialog end-to-end test. No new destructive GUI operation was performed. The ordinary automated native tests and filesystem tests are separate evidence.

Final local results: all five x64 suites pass in Debug and Release (engine 2,216 assertions; copy/recovery/overwrite/delete/catalog/export 4,275; lifecycle 2,367; English resources/layout 4,496; Czech resources/layout 4,588). The lifecycle proposal test also passes its 100 internal checks including export availability. Plugin builds pass Debug Win32/x64/ARM64 and Release x64. ARM64 was compiled, not executed. Cloud CI was not triggered; the changes remain local and uncommitted.

The current user guides document output scope, privacy-sensitive paths, lifecycle and limitations. Public README download links stay on the published 0.2.1 release; 0.2.2 remains an uncommitted local development build until separately requested.

Verified local package including both 0.2.2 features: `out/package-0.2.2-dev-20261019-export-final/dirsync-0.2.2-x64.zip`, SHA-256 `eae409a4f490971946e3689758afea0f5e2519eba619e9dbbe84a87ca26de03a`. Packaging checked x64 PE/version metadata, 14 payload hashes after extraction, the inventory and local guide links. NOTICE identifies the modified worktree/base commit. Both language modules and both languages of the full/pilot guides are included; development reports are not bundled.

## Live GUI follow-up — 2026-10-19, 0.3.0 development

Completed through Computer Use in a separate portable Samandarin 0.16 x64 host, Czech UI/dark mode at the current system DPI. All data and the host configuration were isolated under `C:\Samandarin\builds\dirsync-export-gui-20261019`, outside Git. Installed user instances and the earlier portable hosts were left untouched. No SMB tests, SDK changes or release publication.

The tested production SPL is 713,728 bytes, SHA-256 `3c1af1c1aa4732c63fb0740bb6e155f2dc106dde0bbbb752a583e94b1582ec82`, from source `ae4f1d5`. Both language modules were copied from the current Release output. No production defect was found and no executable code/resource change was needed.

### Comparison, dialog and output checks

- Compared `left-漢字` and `right-ž`: four rows, comprising one equal pair, two left-only files and one right-only file. Manually marked `skip.txt` as Skip; the equal filter then hid `equal.txt`. The preview showed four loaded, three visible and one hidden row, with only the skipped row highlighted.
- Opened **Synchronize → Export comparison and results**, saved `reports-漢字\porovnání-ž.csv` using the native Save dialog, and independently parsed the actual file. All four rows were present; the equal row had `visible=false`, Skip retained its default direction alongside `action=none`/`skipped=true`, absent sides were blank, and byte sizes/UTC timestamps were exact. There were no batch or operation records before execution. Highlighting did not restrict export.
- Opened export from the results context menu. Selected **Text report** in Save as type; the default filename changed from `.csv` to `.txt`. Saved `porovnání-ž.txt` and checked its Unicode paths, four comparison records, metadata and absence of invented operations. Schema field names remain English in either UI language, as documented.
- Cancelled a subsequently opened Save dialog. No new file appeared; both existing report SHA-256 hashes were unchanged and the comparison remained available.
- Entered the existing CSV name and observed the native replacement confirmation. **No** returned to Save without changing either existing report hash. A second Save followed by **Yes** replaced only that synthetic CSV with a fresh, valid report and newer `generated_utc`; the text report was unchanged. No staging file remained.
- All original comparison fixture files retained their SHA-256 hashes through these export-only checks. The report destination was outside both trees and the configured recovery root.

### Actual operation results

1. Executed the visible combined plan on the small fixture: exactly two new files, one in each direction, no overwrites/deletions and Keep originals OFF. The result dialog reported 2/2, no errors/unstarted items and one omitted visible proposal. Both copied targets matched their source data; `skip.txt` was not copied.
2. Changed the left-copy display filter after execution, hiding the previously executed right-only row. Exported `dokončeno.csv` without a new comparison. The report preserved all four comparison rows and exactly two `new_copy` operation records with `status=copied`, `completed=true` and `filesystem_changed=true`. The now-hidden row had current `visible=false` while its operation retained `execution_visible=true`; execution filters were frozen independently of the current filters.
3. Compared a second fixture, `stop-left-漢字` against empty `stop-right-ž`: `0-done.txt` (5 bytes), `a-large.bin` (1,879,048,192 bytes) and `z-next.txt` (4 bytes). The large synthetic source reused an existing test file through a hardlink; its content was not modified. Confirmed only these three new copies, no overwrite/deletion, Keep originals OFF.
4. Clicked **Cancel** in the progress window while the large item was active. The last pre-action capture showed copying that item, one file completed and 11% overall. The result reported 1/3 completed, one cancelled item, one unstarted item and Windows error 1223. Only `0-done.txt` remained in the target; the large target, later file and staging files were absent. The precise worker phase at click delivery is not inferred from the earlier screenshot.
5. Exported the retained result to both `zastaveno.csv` and `zastaveno.txt`. Both reported `copied / cancelled / not_started`, completion `true / false / false`, and a cancelled, incomplete batch. No fictitious completion or rollback was reported.
6. Attempted to export to `stop-right-ž\must-not-exist.csv`. The plugin showed the localized export-error summary, error 5, with instructions to choose an existing parent outside the compared/recovery trees. No report or staging file was created there; the existing completed target remained intact. Closed the error, preview and private host normally.

### Evidence and limits

The outside-Git fixture retains the five GUI-created reports, the original file-hash snapshot, pre-replacement report hashes, `verify_reports.py` and `verification.json`. The independent verifier passed **36 report/file checks**, including CSV/text records, UTF-8 BOM/CRLF, exact values, frozen visibility, original fixture hashes, copied contents and the absence of extra output/staging files. Save cancellation and replacement rejection were additionally checked by file hashes at action time.

| Final report | Bytes | SHA-256 |
| --- | ---: | --- |
| `porovnání-ž.csv` | 5,945 | `0827d094749a0c4e19c341884cd66db1c9a31f1692017b0aa5373f92d3ecbb7f` |
| `porovnání-ž.txt` | 3,705 | `874de777f47abfbf1db0205e491b42c365e380b697b50f628b95c31d5453b43b` |
| `dokončeno.csv` | 8,876 | `eff96e4e36422a8599047fa27aac2b633a34019f9985297f6697f8a0da90c80c` |
| `zastaveno.csv` | 8,834 | `d4ce88438613a2557ff07ba998e2e83a2e07d4025a3571db54bb20a019b3e2ac` |
| `zastaveno.txt` | 5,110 | `d53d89d310f59504da05562faf0311654a55c41004d8c608dcb25c08fede2c5e` |

Preparation initially used an incompatible encoding for the copied portable registry file. Discarded that attempted configuration and copied the known-good file byte-for-byte, then entered fixture roots through the GUI. This was fixture setup, not an export failure. The private host also displayed its existing missing crash-reporter startup notice. No normal installation/configuration was changed.

Computer Use's accessibility snapshot included the owned Save dialog, but its child element indexes were not actionable and its reported focus could remain on Search while the filename caret/selection was visibly active. Fresh screenshots, coordinate clicks and keyboard entry successfully completed the real Save dialogs and confirmations. A transient null accessibility snapshot immediately after Stop was resolved by a new observation; the result was verified afterward. No automation-only production focus workaround was added. Runtime test windows can interrupt menu targeting, so the final excluded-root check was repeated after those suites finished.

All five existing x64 suites were rerun successfully in **Debug and Release**: engine 2,216; copy/recovery/export 4,460; lifecycle 2,369; English UI 5,151; Czech UI 5,279 checks. No binary rebuild was needed for this documentation-only follow-up; current-head build/test/package and CodeQL workflows are checked after pushing it.

This closes the previously pending live export-menu/Save-dialog coverage for comparison, successful new-copy execution and Stop. Live overwrite/deletion/recovery-result export, injected post-publication errors, long-path Save dialogs, English/light mode and other monitor DPI were not repeated here; their existing automated evidence above remains separate. The export is still synchronous, includes all loaded rows and retains only the last in-memory batch. Final release packaging/clean installation of 0.3.0 is a separate remaining verification step.
