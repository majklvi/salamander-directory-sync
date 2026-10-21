# Combined error decisions — live GUI verification, 0.4.0

Live Computer Use campaign on 2026-10-21, using source `c8e6fbd1e8113e871f083944c4d1c400d5efe4b0`. This supplements the [automated combined-error checkpoint](directory-sync-plugin-combined-errors-040.md); it does not replace its dated description of automated versus live coverage.

## Isolation and method

The private Samandarin 0.16 x64 host, its file-backed configuration, six independent fixture pairs and recovery directory are under `C:\Samandarin\builds\dirsync-combined-errors-gui-20261021`, outside Git. No installed host configuration, real project data, SMB paths or SDK files are involved. The host executable SHA-256 is `f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd`; the Release x64 plugin SHA-256 is `6495c1494d29f8a370d645ed934c493ae6f4ba90d9c78113d9745975af15e84f`.

Each comparison uses subdirectories, asymmetric mode and empty directories, all five visibility filters, and the `*.*` mask. Its 15 rows contain six new-copy proposals (five files and one physically empty directory), three overwrites, five right-side deletions (two files and three directories), and an equal `keep.txt` row omitted from execution. Files carry a named NTFS `test` stream. Unicode is present in both roots, nested names and the external recovery path.

Locks are ordinary read handles created after comparison. Exclusive source handles cause error 32 when reading data without changing the confirmed plan. The deletion child uses read/write sharing without delete sharing: preserving its original remains possible, but removal fails. GUI actions use the shipping dialogs through Computer Use. Filesystem checks are independent hash, ADS, timestamp, existence and journal inspections, not fabricated GUI messages or injected executor results.

The disposable portable configuration initially had a preparation-format error (a UTF-8 BOM added to `REGEDIT4`); the host rejected it unchanged and exited. Removing that test-only BOM allowed startup. The recovery path was then entered through Preferences. This was fixture preparation, not a plugin failure.

## Completed recovery-ON mixed batch

The user confirmed the exact synthetic destructive plan immediately before execution. `1-retry.txt` prompted, Retry with its lock still held prompted again, and Retry after release completed it. `2-skip.txt` was skipped once. Skip all at `3-skip-all.txt` automatically skipped `4-auto.txt`, without another question for that matching copy error.

The later original-save question for locked `6-original.txt` still appeared. Skip left its original untouched and blocked exactly its overwrite. The later source-read error for `5-overwrite.txt` also still prompted despite Copy Skip all; Skip retained its original. `7-overwrite-ok.txt` was replaced successfully. The independently saved original of the skipped `5-overwrite.txt` remained available.

Deletion subsequently prompted for `tree-ž\nested\locked-漢字.txt`. Retry while it was still locked prompted again; Skip retained this child and protected both `nested` and `tree-ž`, with zero removal attempts for the protected parents. Independent `9-delete.txt` and `empty-right` were removed.

The actual result dialog reported **copies 3/6, overwrites 1/3, deletions 2/5, execution-skipped 5, dependency-blocked 3, errors/cancelled 0, not started 0**. It explicitly described synchronization as incomplete. The failed save is represented by its blocked main overwrite, without double-counting that prerequisite as another skipped main operation.

All 71 independent disk checks passed: unchanged sources and retained targets, copied main data/ADS/times, absent skipped new targets, retained ancestors, actual independent deletions and verified recovery copies. Four successfully preserved files were manually copied into a separate `manual-restore` tree, including their ADS, and checked against the original baseline. Two recovery sessions contain truthful save/overwrite/delete outcomes, attempts and blocking paths; the retried deletion records two attempts, and the protected parents record zero. A recovery-tree hash inventory was captured before the OFF batch.

## Recovery-OFF mixed batch

The same comparison was executed in the independent `off` pair after disabling Keep originals in Preferences and confirming that the execution dialog said OFF. A fresh question at `1-retry.txt` demonstrated that the previous command's Skip all decision had been cleared. Releasing its lock and choosing Retry completed the copy. Skip at `2-skip.txt` and Skip all at `3-skip-all.txt` again left all three skipped new targets absent; `4-auto.txt` produced no further matching question.

The overwrite source error at `5-overwrite.txt` still prompted. Skip preserved that target, while `6-original.txt` and `7-overwrite-ok.txt` were replaced. The deletion child then prompted independently; Skip all retained it and both selected ancestors. Independent file and empty-directory deletions completed.

The result reported **copies 3/6, overwrites 2/3, deletions 2/5, execution-skipped 5, dependency-blocked 2, errors/cancelled 0, not started 0**, with the incomplete-synchronization note. All 56 disk checks passed. The entire earlier recovery-tree file inventory and hashes remained identical: no new recovery copy, manifest or operation journal was created. Subsequent OFF cancellation cases were checked against that same inventory before the later ON cases began.

## Stop and Close with an outstanding question

Each case had its own fresh pair, the same full confirmed operation plan, a separate action-time user confirmation and one deterministic lock. Stop was the small **× beside Synchronize** in the preview; Close was the preview's **Close** button. Neither action required answering or first dismissing the outstanding item question.

| Case | Question open when cancelled | Observed result |
| --- | --- | --- |
| `stop-off` | First overwrite, source read | All six new items completed. Stop closed the question/progress and returned a stopped summary: overwrite 0/3, deletion 0/5, one cancelled item, seven unstarted main operations. No recovery files were added. 61 disk checks passed. |
| `close-off` | Second new copy, source read | Close dismissed the question, progress and preview; the private host remained open. Only `0-done.txt` had been copied. All later new items, all original targets and all planned deletion targets remained unchanged. No recovery files were added. 53 disk checks passed. Reopening the plugin succeeded. |
| `stop-on` | Saving the second overwrite original | Six new items and the first original save had completed. Stop returned a stopped summary: overwrite 0/3, deletion 0/5, one cancelled preservation item, all eight destructive main operations unstarted. The manifest ends `cancelled`, with the first save `saved`, second `cancelled`, third `not_started`. No overwrite/deletion journal was started. 60 disk checks passed. |
| `close-on` | First overwrite, source read, after all three original saves | Close dismissed question/progress/preview and left the host open. Six new items and three verified saved originals remained; no original target was replaced and no deletion started. The save manifest ends `complete`; the overwrite journal ends `cancelled`, with the first replacement `cancelled` and the other two `not_started`. 60 disk checks passed. |

Across all four cases, source files and pre-existing right-side data/ADS/timestamps matched the original baseline. Final file inventories contained exactly the expected completed copies and original targets, without a partial target, owned staging file or unexpected later copy. Retained right-side directories remained present. Cancellation did not roll back earlier successful work. Close cases intentionally do not display the final summary because the preview itself closes; their evidence is the observed question, window teardown, filesystem and (when enabled) actual records.

## Outcome, regression checks and limits

**All six live cases passed; no plugin code fix was needed.** The campaign adds 361 independent per-case disk checks. All eight successfully saved files across the four recovery sessions were additionally checked against their baseline main-data hashes, named streams and write times. The two earlier sessions and their files remained unchanged during OFF execution. After testing, every lock worker exited normally and only the disposable host was closed. Fixtures, saved originals, records and evidence were retained outside Git.

All five existing x64 suites were rerun through `tools/test.ps1` in Debug and Release: 2,216 engine assertions, 6,730 copy/recovery/overwrite/delete assertions, 2,451 lifecycle checks, 5,513 English resource/layout checks and 5,654 Czech resource/layout checks, all passing in both configurations. No production source or SDK file changed in this campaign. The existing binaries were used; this is documentation of live verification, not a new release or rebuilt package. Current-commit cloud build/test/package and CodeQL results are checked after the documentation push.

Live cancellation here deliberately targets the decision wait in new-copy, original-save and overwrite stages, with both recovery modes. Active-I/O and Verify cancellation and additional deletion-phase decision cases remain covered by the separate automated regressions; they were not additional live cases in this campaign. This does not claim every error class, cancellation timing, host exit/unload path, DPI, language/theme combination, large real-data set, low-space disk or SMB scenario. The live host and plugin dialogs used English with the dark host appearance. No SDK extension, automatic restoration or cleanup feature was added.

Local evidence includes `baseline.json`, the `*-confirmation.txt`, `*-question.txt`, `*-summary.txt` and `*-after-stop.txt` accessibility captures, six `*-disk-checks.json` files, `close-*-windows.json`, original JSONL records and `recovery-before-off.json`. The small fixture/lock/check scripts remain beside the local fixture root. Evidence and temporary lock workers are deliberately outside the repository.
