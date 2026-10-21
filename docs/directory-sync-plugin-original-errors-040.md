# Interactive original preservation and overwrites — 0.4.0

**Historical checkpoint.** A subsequent [deletion checkpoint](directory-sync-plugin-delete-errors-040.md) implements interactive asymmetric deletion and retained-parent protection; the scope below describes this earlier commit.

Implemented and verified on 2026-10-21 against approved [R1–R5](directory-sync-plugin-error-handling.md), following the [stages 1–2 checkpoint](directory-sync-plugin-errors-040.md). Source baseline: `048e5a19711e4b585402870dbccf5f17f5959ab0`.

## Delivered behavior

The separate **Save visible originals…** and **Overwrite visible files…** commands now use the same cancellable Retry / Skip / Skip all / Cancel policy as new-item copying. The remembered global **Stop at first error** also applies to these commands. Recovery remains a global preference, outside profiles, captured when the command starts.

- Skip all matches operation, phase, source/target/data site and Windows error code for this command only. A saving error cannot silently suppress an overwrite error with the same code.
- Saving uses staging and verification as before. A recoverable unpublished failure can be retried from fresh staging, or skipped after cleanup. Pending confirmed rows and their parents are revalidated before Retry or independent continuation. Completed items are not repeated.
- Every overwrite with recovery ON requires its own verified, logged saved original. Originals are matched by confirmed row identity and side, not a shortened vector position. Skipping a save produces `dependency_blocked` for exactly that replacement. Other eligible replacements can continue after a healthy preservation phase. The saved copy's observed identity/metadata are checked again before its dependent replacement and on Retry.
- Recovery OFF creates no recovery copy or recovery journal. Successful replacements retain target permissions and replace data/ADS only after staging and verification. Skipping a failure leaves that target unchanged; other independent items can continue.
- Shared recovery setup, manifest and execution-journal creation/appends remain fatal. A shared logging failure never offers Skip or Retry, including after a target has already changed. There is no fallback to an unprotected overwrite.
- Invalid/stale plans, verification failures, published or uncertain effects, and unaccounted staging residue remain fatal. Read-only target flags are not cleared. No automatic rollback or stronger concurrent-write guarantee was added.
- Stop/Close cancel an outstanding question before joining the worker. Pause is disabled during the question; decision time is excluded from transfer-rate/throttle accounting. Completed saves and replacements remain.

**Interactive deletion and combined-plan continuation remain later stages.** The combined command explicitly enforces its existing stop-first contract, including for an internal caller that supplies a prompt. The deletion preservation path does not offer interactive decisions. No SMB tests, SDK/vendor changes or release publication were performed.

## Records and presentation

The schema-1 JSONL records gain additive `attempts`, `lastError` and `automaticSkip` fields, plus `skipped` and `dependency_blocked` statuses. Only the final result of an item is appended, not an apparent completion for every failed attempt. A phase containing skips ends as `incomplete`. A healthy completed preservation phase can contain skips; this differs from having saved every original successfully.

CSV/text exports include saving/overwrite attempts and skip causes. The saved-original browser recognizes skipped saves and blocked replacements without presenting either as completed. Earlier schema-1 sessions still load. Older plugin versions may warn about new statuses; keep the current plugin when inspecting these sessions.

A live test exposed a summary bug: the saved-original counter stopped at the first skipped backup, incorrectly showing 1/3 when two originals around that skip had been saved. Counting now examines the full result set. A production information-dialog regression verifies 2/3 saved, two replacements and one blocked replacement in both languages. Legacy confirmation/result text was also corrected from an all-or-nothing description to the per-item dependency rule.

## Automated verification

All five x64 suites passed in Debug and Release:

| Suite | Checks |
| --- | ---: |
| Comparison engine | 2,216 |
| Copy/recovery/overwrite/deletion | 5,386 |
| Lifecycle | 2,475 |
| English resources/layout | 5,501 |
| Czech resources/layout | 5,642 |

Coverage includes actual sharing violations, unlock/Retry in both directions with recovery ON/OFF, prior successes not repeated, data/ADS/DACL preservation, noncontiguous reordered selections, exact skipped-save dependencies, operation-specific Skip all, independent continuation, partial disk-shortage cleanup and Retry, changed source/target/saved copy, cancellation, fatal verification corruption and failures at shared protocol boundaries before and after publication. Disk-shortage and protocol failures in this checkpoint are injected faults, not a new VHD campaign.

Engine workers waiting on questions exit on cancellation or channel closure without a UI message pump. Production dialog handlers cover Retry, Skip, Skip all, Cancel, question Close, transfer Close and Stop for copy/save/overwrite in English and Czech. Existing lifecycle coverage also exercises Stop/Close during transfer and Verify with recovery ON/OFF. Compiled-resource geometry covers 96–240 DPI; this is not a live desktop test at every DPI.

Plugin builds passed Debug Win32/x64/ARM64 and Release x64. ARM64 is compilation only. SDK/vendor changes are absent. `tools/package.ps1` verified all three x64 modules as 0.4.0, all 14 packaged-file hashes after extraction and bilingual guide links. `dirsync.spl` is 736,768 bytes. The local archive in `out/package-040-originals` has SHA-256 `96bdadbe8670e4e5f12871c8dc1e259ea9a6d2397d9f7fa980120ca3eccc5b7f`; it records a modified working tree based on the baseline commit and is not a published release. Current-head GitHub build/test/package and CodeQL results must be checked after pushing.

## Live GUI verification

Computer Use exercised a private Samandarin 0.16 x64 host, English UI in dark mode, on isolated local NTFS fixtures under `C:\Samandarin\builds\dirsync-original-errors-040-20261021`. Each fixture contained three existing-file replacements, Unicode paths and old/new named streams. No deletion command was used and the regular host configuration was not changed.

| Case | Action and observed result |
| --- | --- |
| Recovery ON | Locked the second original. The **Save original — read original** question offered all four decisions. Skip all skipped that save, preserving the target and blocking its exact replacement. A separately locked third source still prompted **Overwrite — read source** despite the same error 32. Unlock/Retry finished it. First/third targets changed, second remained original. Two originals were saved and verified; journal results were `replaced`, `dependency_blocked`, `replaced`, with attempt counts 1, 0, 2. |
| Recovery OFF | After the first successful replacement, a locked second source prompted. Skip preserved it and allowed the third replacement. The summary showed two replaced, one skipped, no blocked replacement. The entire existing recovery-file inventory and hashes were unchanged. |
| Stop, recovery ON | While the second original was locked and its question was open, clicked the main Stop control. The question/progress windows closed and the worker returned. All three targets remained original. The first saved original remained; the manifest recorded `saved`, `cancelled`, `not_started`, then `cancelled`. No overwrite journal was created. |
| Close, recovery OFF | After the first replacement, clicked the preview Close while the second source's question was open. The preview, question and progress windows closed; the host remained available. Only the first target changed; second/third targets remained original. Recovery inventory and hashes stayed unchanged. |
| Manual recovery | Copied the first saved ON original to a separate unused path outside both trees. Main data and the old ADS matched the saved copy. No automatic restore operation was added. |

Post-test checks verified main-data SHA-256, expected old/new ADS, absence of owned staging residue, the ON/Stop JSONL records and OFF recovery inventories. The count defect described above was observed during the first ON GUI case; the corrected count was subsequently verified by the actual information-dialog regression in both languages, rather than claiming a repeated live ON test.

Evidence remains outside Git in that isolated test root: `on-save-question.txt`, `on-overwrite-question.txt`, `on-summary-before-count-fix.txt`, `off-question.txt`, `off-summary.txt`, `stop-question.txt`, `stop-summary.txt`, `close-question.txt`, `close-windows-after.json`, and `verification.json`. Test lock handles were released and only the private host was closed. The historical checkpoint remains dated; current behavior is summarized in the [English](directory-sync-plugin.en.md) and [Czech](directory-sync-plugin.md) user guides.
