# Interactive asymmetric deletion — 0.4.0

Implemented and verified on 2026-10-21, following approved [R1–R5](directory-sync-plugin-error-handling.md) and the [original-preservation/overwrite checkpoint](directory-sync-plugin-original-errors-040.md). Source baseline: `be3b5303a515084ff847b273f641aca4c42bd189`.

## Scope and decisions

The separate **Delete visible items…** command now offers the existing compact native **Retry / Skip / Skip all / Cancel** question for supported unpublished item failures. This applies to asymmetric right-only files and explicitly selected directories, with the existing remembered global **Stop at first error** preference and recovery choice. Immediate context-menu Shell deletion is unchanged. Combined **Execute visible plan…** still stops at its first error; continuation across combined phases remains the next implementation stage.

- Retry attempts only the current unchanged, unremoved item. A successful earlier deletion is never repeated. Source/counterpart absence, target identity/metadata, root/parent safety and any required saved original are checked again.
- Skip leaves the item incomplete and continues independent confirmed work. Skip all uses the existing operation + phase + source/target/data site + Windows-error key for this command only. Saving and deletion remain distinct operations.
- A skipped deletion protects **every selected ancestor directory**. A failed original save that is skipped blocks its exact deletion and all those ancestors too. The result records the retained path; similarly named siblings such as `tree-other` are not treated as descendants of `tree`.
- Directories are still attempted deepest first, after files, and removed only when actually empty. There is no recursive fallback, attribute clearing, unconfirmed child selection, Recycle Bin use or automatic rollback.
- Shared manifest/journal failures, invalidated plans, verification mismatches and errors after a successful removal remain fatal, without item decisions. A failed Windows removal is eligible for a decision only if the target can still be inspected with the same observed identity/metadata. Missing, replaced or uncertain targets stop conservatively.

## Recovery and plan validation

With recovery ON, preservation settles before any deletion. Each eligible removal requires its own verified, logged copy, mapped by confirmed row identity on the right. Skipped saves do not shift other mappings. A healthy preservation phase with skips is distinct from having saved every item; successfully saved originals remain available even when their dependent deletion or a parent is retained. Shared-protocol failure stops all destructive execution that has not yet occurred.

The verified identity of a saved file or directory marker is retained in memory and checked before its dependent deletion, including Retry. Creating a directory recovery marker is now recorded immediately as a published effect: a later metadata failure is fatal, rather than incorrectly offering another creation attempt. Recovery OFF creates no recovery copy or deletion journal and never uses earlier backups as prerequisites.

The read-only checkpoint accepts only the executor's supplied successful removal indices as known changes. It permits those directories to be absent from the previous parent set and permits timestamp/link-count changes of their surviving ancestors. Other pending files keep their original stamps; pending directories keep identity/type/attributes, with full stamp matching unless an actual completed descendant removal accounts for the change. No comparison proposals are rebuilt or added. Final `RemoveDirectoryW` still rejects a newly created unplanned child. These are ordinary host-level checks, not a transaction or content-history guarantee.

## Results and native dialogs

Deletion outcomes now distinguish deleted, execution-skipped, dependency-blocked, failed, cancelled and not started. They preserve successful removal effects and record `attempts`, `lastError`, `automaticSkip` and `blockedBy` in the deletion JSONL journal. CSV/text exports add `blocked_by` and the same attempt/error/skip accounting. Skips end a healthy journal as `incomplete`, not `complete`.

The saved-originals browser accepts the additive schema-1 deletion statuses. Its blocked-operation caption is generic enough for both overwrites and deletion. English/Czech summaries separately count execution skips and blocked items, explain a retained dependency, and count all successful original saves even when a skipped entry lies between them. Confirmation/help text now describes per-item preservation rather than claiming that every original must succeed for any independent deletion to proceed.

Stop, question Cancel/Close, transfer Close and preview cancellation use the existing cancellable question channel. The UI cancels the wait before joining the worker; no synchronous worker-to-UI request was added. Pause stays disabled while waiting, the host is not disabled by the question, and decision wait time remains excluded from rate/throttle accounting.

## Verification

Tests use disposable, unique local NTFS fixture trees created by the repository test suites. No user directory was selected and no live host deletion was driven through Computer Use in this checkpoint. GUI verification here means automated creation of the shipping English/Czech dialogs and invocation of their production handlers, combined with actual filesystem worker tests; it is not a claim of a separate live desktop/pilot campaign.

| Scenario | Verified result |
| --- | --- |
| Locked file, recovery ON/OFF, Retry | A real read handle denies delete sharing while allowing backup reads. Unlock/Retry deletes the current item once; an earlier successful deletion retains one attempt. Both the file and now-empty ancestors can finish. |
| Locked file, recovery ON/OFF, Skip | The file and its ADS remain. Every planned ancestor is dependency-blocked with zero removal attempts and the retained file path. Independent sibling files, independent directories and prefix siblings are deleted. |
| Skip all and skipped original | Matching delete failures receive automatic skips. An exclusive original-read lock produces a saving decision instead; Skip blocks only its deletion and its ancestors. Independent work continues. |
| Retry after completed directory removal | A nested empty directory and its parent are already gone before a later locked directory prompts. The checkpoint accepts these known removals and Retry finishes without repeating them. |
| New unplanned child during Retry | After an earlier child deletion, an external file appears while a directory question is open. Retry never selects it; the final nonempty-directory failure stops the batch and leaves the new file and later independent directory intact. |
| Stop/Close while awaiting a decision | Recovery ON/OFF deletion waits and recovery-ON saving waits wake and join without another UI message pump. Completed removals stay; the current and later items stay. Cancellation during preservation prevents every deletion. |
| Read-only target | Skip leaves its attributes and contents unchanged and permits an unrelated deletion. Stop-first callers retain the prior eager refusal. |
| Changed pending file, new left counterpart, changed saved original | Retry terminates as stale/invalid; current and later targets are not deleted. |
| Post-removal failure | The result keeps `removed=true`, stops without a question, and never repeats removal or attempts later work. |
| Shared preservation/deletion journal failures | Injected result/end append failures stop without Skip/Retry. Effects already completed are reported truthfully; later removals do not start. |
| Manual recovery, ON | Copying an independently saved original to a separate unused path restores the main data and named stream. No automatic restore or backup cleanup was added. |
| Records and summaries | New deletion statuses load in the catalog and appear in text/CSV with attempts/dependencies. Actual EN/CS result dialogs verify two successful originals around one skip as 2/3, two deleted files, zero errors and one blocked deletion. |

The native error-dialog regression now covers copy/save/overwrite/delete with Retry, Skip, Skip all, Cancel, question Close, transfer Close and Stop in both languages. It checks full long Unicode edit values, enabled owner, disabled Pause, worker termination and dialog cleanup. Compiled-resource layout checks cover 96, 120, 144, 192 and 240 DPI. Existing path, hidden/system, mask, Skip, ADS, reparse, ACL, cancellation, shutdown/unload, publication and deletion regression tests remain enabled. A read-attributes-only test handle initially failed to block deletion; it was corrected to a real read handle with delete sharing denied, rather than weakening the assertions.

All five x64 suites passed in Debug and Release: 2,216 engine assertions, 5,841 copy/recovery/overwrite/deletion assertions, 2,451 lifecycle checks, 5,511 English resource/layout checks and 5,652 Czech resource/layout checks. Debug Win32/x64/ARM64 and Release x64 plugin builds passed. ARM64 is compilation only. Test outputs are retained locally in `out/test-results-Debug.json` and `out/test-results-Release.json`.

`tools/package.ps1` verified all three modules as 0.4.0 x64, all 14 packaged-file hashes after extraction and bilingual guide links. The SPL is 749,056 bytes. Local archive `out/package-040-delete-errors/dirsync-0.4.0-x64.zip` has SHA-256 `55ee7818e79c636fdced1cd489c502dc2d4101850de59c3f5d0760667f51af45`; it records modified sources based on the baseline commit and is not a published release. Current-head CI and CodeQL must be checked after pushing. No SMB testing, SDK/vendor modification, release publication or combined-phase continuation is included.
