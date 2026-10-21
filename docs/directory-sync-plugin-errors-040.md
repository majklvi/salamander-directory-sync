# Interactive operation errors — 0.4.0 stages 1–2

**Historical stages 1–2 checkpoint.** A later [0.4.0 follow-up](directory-sync-plugin-original-errors-040.md) adds interactive original preservation and overwrites; the scope statements below describe this earlier checkpoint.

Implemented and verified 2026-10-21, based on approved [R1–R5](directory-sync-plugin-error-handling.md). Baseline: released 0.3.0, `5c9022c3ad104c5ab0f693222d24830c4d6ac24b`.

## Delivered scope

The common decision model and the separate **Copy visible new items…** adapter are implemented. This includes files and physically empty directories in either direction. Settings contain a remembered global **Stop at first error**, off by default; neither profiles nor the SDK format changed. Existing installations with no value use interactive mode for this command.

**Recovery saves, overwrites, deletions and Execute visible plan still stop on the first error.** Their interactive adapters, stable recovery/dependency mapping and continuing later phases are subsequent stages, not features of this checkpoint. The shared `DependencyBlocked` result code is reserved for those stages.

No SMB test, SDK/vendor change, release publication, queue integration, automatic rollback or advanced filesystem backend was introduced.

## Decision and safety boundaries

- Retry, Skip, Skip all and Cancel use copied operation/phase/side/Windows-error context. Skip all matches all four fields exactly for this batch only. It cannot suppress fatal errors and is reset before every command.
- Supported unpublished errors include access denial, sharing/lock violations, insufficient space and selected ordinary I/O/device errors. The target must not have been published and owned staging cleanup must be accounted for. Verification-phase failures, data mismatch, invalid/stale plans, published side effects and staging residue remain fatal. Unsupported error codes stop conservatively.
- Retry starts fresh staging for the current item and checks the remaining confirmed rows plus root/parent identities. Prior successful items are not repeated; no new proposals are adopted. Skip permits independent remaining new copies, after the same pending-plan check. Newly required empty parents can remain, as before.
- Existing identity, size, timestamps and metadata checks remain. They are not a content history: a same-size change with unchanged observed metadata is not guaranteed to be detected. No stronger concurrent-write contract is claimed.
- The worker posts an asynchronous request to the UI thread and waits on a cancellable channel. UI teardown cancels the channel before joining. No synchronous worker-to-UI send is used. The compact native EN/CS question is owned by the progress window; the host and preview Stop/Close remain available. Cancel or closing the question stops the whole remaining batch.
- Pause is disabled during a question. Decision-wait time is excluded from transfer-rate/throttle accounting; rate samples reset around the wait. Actual elapsed wall time remains available.

## Results and exports

The copy summary distinguishes copied/verified, execution-skipped, failed/cancelled and unstarted items. A skipped batch explicitly says it did not complete all planned operations. The CSV/text schema receives additive `attempts`, `last_error` and `automatic_skip` operation fields. A successful retry has final error zero while preserving its last encountered error and attempt count. A skipped operation is not completed and is distinct from proposal Skip. Attempt tracking currently covers the new-copy adapter; zero for another adapter means not tracked there.

## Automated verification

- All five x64 suites pass in Debug and Release: engine, copy/recovery/overwrite/delete, lifecycle, English resources and Czech resources.
- Engine: 2,216 assertions. Copy/recovery/overwrite/delete: 4,624 assertions, including real source locks, unlock/Retry without repeating a prior copy, Skip all with a different target error still prompting, independent continuation, Cancel, retry after simulated disk shortage, source/target mutation, verification corruption and post-publication failure.
- Empty-directory Retry after a target ACL denies subdirectory creation passes in both directions. Corrected access permits the same confirmed item to retry. A read-attributes-only directory handle was found not to simulate an actual copy failure and was replaced by this real ACL fixture; no unsupported sharing assumption is retained.
- Policy tests cover operation/phase/side/code matching, fatal exclusions and next-batch reset. A worker waiting for a decision exits on atomic cancellation or channel closure without any UI message pumping.
- Production error-dialog handlers are exercised with both language modules: Retry, Skip, Skip all, Cancel, error Close, transfer Close and Stop, complete long Unicode edit values, enabled host owner, disabled Pause and joined worker.
- Settings round trips verify missing/default, ON/OFF, and invalid stored policy values. EN/CS resource/layout checks cover 96, 120, 144, 192 and 240 DPI. These are compiled-dialog/handler tests, not claims of live desktop tests at every DPI.
- Plugin matrix: Debug Win32/x64/ARM64 and Release x64 passes. ARM64 is compilation only. SDK/vendor diff is empty.
- Development x64 package verification passes: all three binary versions are 0.4.0, 14 packaged-file hashes match after extraction, and bilingual user/pilot guides resolve their documentation links. `dirsync.spl` is 726,016 bytes. This local dirty-source package is verification evidence, not a published release artifact.

An initial mutation regression used an immediate same-length write and was timing dependent; changing the fixture length makes its intended stale-plan assertion deterministic. Initial lifecycle expected-count bookkeeping was updated for the four new settings assertions. The first sandboxed MSBuild attempt timed out; subsequent builds through the normal escalated build process passed. No product safety check was removed to make tests pass.

## Live GUI verification

All desktop input used the Computer Use skill. Shell/file APIs only prepared isolated fixtures, held/released test locks and inspected resulting files. Local evidence is outside Git at `C:\Samandarin\builds\dirsync-errors-040-20261021`.

Host: isolated Samandarin 0.16 x64 developer build, SHA-256 `f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd`, with its own file-backed configuration. The tested plugin/language binaries are Release x64 0.4.0. No user data was overwritten/deleted by the new scenarios. Ordinary installed host instances were not used for the tests.

| Scenario | Observed result |
| --- | --- |
| English/dark, source locked after Compare | One new file completes, then Copy error shows the affected Unicode source/target and Windows error 32. Progress says it is waiting; Pause is disabled. |
| Host use during question | Selecting another source in the host panel succeeds while the worker remains waiting; no main-host modality. |
| Unlock + Retry | All four new files and one empty Unicode directory complete. All four source/target SHA-256 pairs match. Earlier completed file is not repeated by the adapter, also checked automatically. |
| Two source locks + Skip all | One question; two matching sources remain absent, independent file and empty directory copy. Summary: 3 completed, 2 execution-skipped, 0 errors, 0 unstarted, with an incomplete-work note. |
| Next batch after Skip all | The same locked source prompts again; no cross-batch suppression. |
| Preview Stop with question open | Error/progress windows close, summary reports 1 complete, 1 cancelled, 3 unstarted. Only `00-completed.txt` exists in the new target. |
| Preview Close with question open | Error/progress/preview close after the worker exits. Host remains usable. Only the earlier `00-completed.txt` exists; later work and staging are absent. |
| Settings: Stop at first error ON | The same lock ends the separate four-file command without a decision question: 1 completed, 1 failed, 2 unstarted, 0 skipped. Only the first new target exists. Normal host shutdown persists `StopOnFirstErrorV1=1` in its isolated configuration; automated reload tests cover both values. |

The desktop launcher first delegated to the installed application; the private developer executable was then launched directly. The private host's missing crash-reporter startup warning was acknowledged. This is test-environment setup, not evidence of a plugin failure. Live testing used English/dark; Czech and other DPI/handler paths are covered automatically, not claimed as live appearance checks.

## Remaining stages

Implement interactive recovery/overwrite/delete adapters with stable prerequisite mapping and retained-child protection, then connect combined synchronization and later phase continuation. Verify those destructive paths separately with the agreed recovery modes and protocol-failure boundaries. They are deliberately not enabled through this common model alone.

Cloud checks are associated with the implementation commit in this repository’s GitHub Actions; local results above do not imply cloud success. The public download remains release 0.3.0 until a separately authorized release.
