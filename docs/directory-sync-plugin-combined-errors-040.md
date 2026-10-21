# Combined synchronization after Skip — 0.4.0

Follow-up: [live Computer Use verification of mixed decisions and Stop/Close with recovery ON/OFF](directory-sync-plugin-combined-errors-gui-040.md). The checkpoint below retains its original automated-test scope.

Implemented and verified on 2026-10-21 against approved [R1–R5](directory-sync-plugin-error-handling.md). Source baseline: `794ee35859b9a0115ec370270aa0258f96e10149`, following the [asymmetric-deletion checkpoint](directory-sync-plugin-delete-errors-040.md).

## Delivered behavior

**Execute visible plan…** now uses the same frozen error policy as its separate copy, preservation, overwrite and asymmetric-deletion commands. The default is to ask **Retry / Skip / Skip all / Cancel** for supported recoverable item errors; the remembered global **Stop at first error** remains available. The single confirmation describes independent continuation and the unchanged operation order:

1. New files and physically empty directories.
2. Existing-file overwrites, with original preservation when enabled.
3. Right-only asymmetric deletions, files before actually empty directories, deepest first, with original preservation when enabled.

A phase whose eligible items ended as completed, explicitly skipped or dependency-blocked can continue to independent confirmed work in the next phase. This is **not** successful completion of every planned operation: the final `complete` flag stays false if anything was skipped or blocked. Skipped Windows errors remain on their individual outcomes; they are not promoted to a fatal batch error merely because the item was skipped.

Failed, cancelled, unstarted or missing expected adapter outcomes prevent later phases. Invalidated plans, verification mismatch, uncertain or already published/removed effects, unaccounted staging and shared manifest/journal failures remain fatal. Cancel, Stop and Close stop the remaining batch; successful earlier work remains without rollback. No earlier completed operation or whole batch is retried.

## Boundaries between phases

The plan, row identities, visible eligibility and recovery choice remain the ones confirmed by the user. Before entering a later phase, its pending rows must still match their original identities and stamps and roots/parents must remain safe. Only the read-only validation baseline advances to account for earlier successful work; proposals are never rebuilt or added. An earlier adapter's private snapshot does not authorize adopting external changes to future overwrite/deletion targets.

Saved originals retain the exact row/side mapping and independently verified/logged prerequisites implemented in the preceding checkpoints. A skipped save blocks its dependent overwrite or deletion, not an unrelated later operation. Retained deletion children protect all planned ancestors. Final directory removal remains nonrecursive and requires actual emptiness.

**Skip all belongs to this entire command**, not to an individual adapter. Matching operation + phase + source/target/data site + Windows error decisions survive between its phases. A Copy decision cannot suppress an Overwrite or Delete question. A Save decision can match saving another original in the later deletion phase; its skipped destructive prerequisite and ancestor protection still apply. The next combined command clears the earlier decisions, without changing the configured policy, pause/rate controls or profile settings. The existing UI start-of-command reset also remains in place.

Recovery OFF creates no plugin recovery copy or execution journal. It does not reuse earlier recovery files. Recovery ON preserves successful originals and keeps shared manifest/journal failures fatal, even if an earlier phase had an explicit Skip. No SMB tests, SDK/vendor changes, release publication, automatic recovery/cleanup or host transfer-queue integration are included.

## Results, exports and native dialogs

The combined summary separately counts completed/planned copies, replacements and deletions, execution-skipped items, dependency-blocked items, errors and unstarted main operations. Skips and blocked dependencies are not counted again as errors or unstarted work. A fatal preservation/protocol error can coexist with unstarted dependent main operations. The result explicitly states that synchronization is incomplete.

Error details prefer the terminal failure over an earlier skipped error. This fixes a misleading result in which a handled sharing violation could otherwise hide the later fatal write/protocol error. Recovery manifests and operation journals remain available in the summary, including after cancellation.

Text/CSV conversion preserves actual completed operations, saving/overwrite/delete attempts, last errors, automatic skips, original paths and blocked-child causes. After a normal returned terminal report, confirmed operations whose later adapter never started are marked `not_started`. If the worker supplied no terminal report, they remain `not_reported`: absence of evidence must not be turned into a claim that no write happened. The terminal result moves its vectors rather than copying them at the return boundary.

The existing compact English/Czech error question is now available while the preview is in combined mode alone. It remains owned by the transfer window, with the host enabled and Pause disabled during the question. The cancellable channel is resolved before worker join; no synchronous worker/UI send was introduced. Decision waits retain the existing transfer-rate accounting rules. Completed-work progress remains distinct from skipped work; skips do not falsely become copied/verified data.

## Verification

Verification uses disposable local NTFS fixtures and the repository test scripts. No live host deletion or separate Computer Use pilot was performed in this checkpoint. Native GUI verification means automated creation of the shipping dialogs and calls to their production handlers, paired with actual filesystem workers; this is distinct from a live desktop campaign.

| Scenario | Observed result |
| --- | --- |
| Recoverable skips in all main phases, recovery ON/OFF | Real source-read locks produce Copy and Overwrite questions; delete-sharing locks allow backup reads but prevent removal. Each Skip all suppresses only its matching further item. Independent new copies/empty directories, overwrites and later deletions finish. Six skipped items, three automatic skips and both retained ancestors are reported without a fatal error or false completion. |
| Save Skip all across phases | An unreadable overwrite original prompts once. The matching locked deletion original is automatically skipped later. Exact destructive dependants and their ancestors remain; independent replacements and deletions continue. Both preservation phases settle as incomplete but healthy. |
| Recovery ON/OFF and records | ON keeps verified originals and two readable recovery sessions when overwrite/deletion execution is skipped. OFF creates no recovery journals. Final text/CSV includes skip/dependency/attempt information, with no false pending items in a fully settled skipped batch. |
| External future target change during an earlier question | A copy Skip cannot authorize a changed later overwrite target. An overwrite Skip cannot authorize a changed later deletion target. The phase boundary stops as stale before entering the affected adapter. |
| Stop/Close while awaiting decisions | Actual combined workers are cancelled through the atomic flag or closed question channel during new-copy, overwrite-original saving, overwrite, deletion-original saving and deletion questions, with both applicable recovery modes. Workers join without another UI message pump. Exactly the earlier completed phases remain; the current retained child and later work remain unexecuted. |
| Stop during active transfer or Verify | Cancellation at copy/save/overwrite transfer and verification boundaries, and before actual deletion, prevents later destructive phases. Cancellation does not open another error question. |
| Shared protocol failure | Injected preservation-result, overwrite-result and deletion-result append failures stop without item decisions, preserving truthful earlier copy/replacement/removal effects. |
| Fatal failure after an earlier Skip | Verification corruption, post-publication failure and execution-journal failure still prevent later deletion. The earlier completed new copy and explicit Copy Skip remain in the result. Skip all cannot suppress these failures. |
| Stop-first policy and next command | Enabling stop-first prevents the question and later deletion. Interactive Skip permits that independent deletion. A new command prompts again rather than inheriting the prior Skip all set; the caller's configured policy remains unchanged. |
| Final export knowledge | Returned terminal reports classify unentered operations as `not_started`. A missing worker report leaves `not_reported` unchanged. |
| Native question and summary | English/Czech dialog regressions cover every operation kind in both standalone and combined modes, all four decisions, question/transfer Close and Stop, full long Unicode edit values, enabled owner, disabled Pause, joined worker and dialog cleanup. The actual combined result dialog verifies mixed success/skip/blocked/failure/unstarted counts and prioritizes the fatal write error over an earlier skipped sharing error. |

Existing comparison, path/metadata/ADS/ACL/reparse, original preservation, immediate Shell deletion, directory protection, modeless lifetime, shutdown/unload, profiles, export and compiled-resource DPI tests remain enabled. Compiled layouts cover 96–240 DPI; this is not live testing at each DPI or of every host-exit/unload timing with a combined question open. No broader live pilot or SMB verification is claimed.

All five x64 suites passed in Debug and Release: 2,216 engine assertions, 6,730 copy/recovery/overwrite/delete assertions, 2,451 lifecycle checks, 5,513 English resource/layout checks and 5,654 Czech resource/layout checks. Debug Win32/x64/ARM64 and Release x64 plugin builds passed; final tooltip strings were also rebuilt and checked in both resource-test configurations. Test outputs remain under `out/test-results-*.json`; ARM64 is compilation only. Packaging and current-head GitHub build/test/package/CodeQL results are checked separately before task completion.

`tools/package.ps1` verified all three modules as 0.4.0 x64, all 14 packaged-file hashes after extraction and bilingual user/pilot-guide links. The SPL is 750,080 bytes. The local archive in `out/package-040-combined-errors` has SHA-256 `daddf2b15cd6f1fba2fb2646aacaa81f56022e668142bbeeb5545476f2d04b88`. It records modified sources based on the baseline commit; it is verification evidence, not a published release. README download links continue to point to released 0.3.0.
