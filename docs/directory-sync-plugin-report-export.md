# Comparison and operation-result export — 0.2.2

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
