# 0.4.0 readiness and plain explanatory captions

Updated 2026-10-21, following the [live combined-command verification](directory-sync-plugin-combined-errors-gui-040.md). Version 0.4.0 remains in development; the public release is still 0.3.0.

## Presentation follow-up

The shared confirmation/result dialog now displays its explanatory text as a native wrapping Static caption, without a Notes/Poznámky label, edit border, scrollbar or additional Tab stop. This includes the new-item copy result and the save/overwrite/delete/combined result dialogs. Its compact initial layout measures the complete wrapped explanation rather than retaining the old scrolling edit's height cap. Resizing remains available. Empty explanations are hidden.

The common recoverable-operation error dialog likewise displays the system error description as a plain caption. Its compact 350 × 104 DLU layout now follows the host's basic file-error dialog, with a centered standard button grid and borderless selectable, horizontally scrolling source/target paths. The synchronous narrow SDK dialog is not used: the plugin preserves two complete Unicode paths and its existing cancellable modeless question handle. Operation/phase context and Retry/Skip/Skip all/Cancel behavior are unchanged. Literal ampersands are displayed normally. Table value copying and full-summary copying remain available; the latter still includes the explanatory text without requiring an editable control.

Both English and Czech shipping resources were updated. Automated native-dialog checks verify that long confirmation/result explanations fit at the initial size and after enlarging and restoring the dialog, with full Unicode/ampersand text preserved. The real operation questions retain their decision/cancellation tests and check the plain description's fit. Resource tests verify native Static classes, absence of edit chrome/Tab stops, and compiled dialog bounds at 96, 120, 144, 192 and 240 DPI. Those resource checks are not live monitor-DPI tests or a Computer Use visual inspection of this follow-up.

All five x64 suites passed in Debug and Release: 2,216 engine assertions, 6,730 copy/recovery/overwrite/delete assertions, 2,451 lifecycle checks, 5,516 English and 5,656 Czech resource/layout checks. Debug Win32/x64/ARM64 and Release x64 plugin builds passed. ARM64 is compilation only. No SDK or executor behavior changed.

**Focused live GUI acceptance is complete.** Four private EN/CS light/dark configurations verify confirmation/result captions, the host-derived error layout, representative long Unicode paths, keyboard navigation and pending-question lifetime. Both Plugin Manager forced-unload cases actually unmap SPL/SLG and permit a reload/Compare in the same host PID. Both host-close cases finish without a hang; the Czech/dark launch observer records exit code 0. Completed data/ADS/timestamps remain correct, pending targets/staging are absent and sources are unchanged. See the [source comparison and live report](directory-sync-plugin-native-errors-gui-040.md) for scope and observation limits.

## Completed 0.4.0 behavior

- Approved R1–R5 apply to new copies, original preservation, overwrites, asymmetric file/empty-directory deletion and combined synchronization.
- Interactive Retry/Skip/Skip all/Cancel is the default, with a remembered global Stop at first error. Skip all matches operation/phase/site/error within one command and resets before the next command.
- Skips allow independent confirmed operations and later phases. A skipped original blocks its destructive dependant, and retained deletion children protect every selected ancestor.
- Retry revalidates the pending confirmed plan, cleans eligible unpublished staging and repeats only the current item. Verification mismatch, stale plans, uncertain/published effects, staging residue and shared-protocol failures remain fatal. Completed changes are not rolled back.
- Results, exports and recovery records distinguish completed, execution-skipped, dependency-blocked, failed/cancelled and unstarted work, with attempts/last errors/automatic skips.
- Six live local NTFS GUI cases passed for mixed decisions and outstanding-question Stop/Close with recovery ON/OFF. Independent disk checks, saved-original records and manual restoration agreed with the results. Broader failure/timing coverage is described separately in the automated checkpoints.

## Remaining release checks

The GUI-consumer consistency check is complete: mixed and stopped local NTFS batches agree across result dialogs, actual GUI-created CSV/text exports, Stored originals and real recovery journals. The independent verifier passed 201 checks, including manual Skip, hidden rows, automatic skips, blocked dependencies, confirmation exclusions and unstarted work. No production fix was needed. See the [live outcome-consistency report](directory-sync-plugin-outcomes-gui-040.md) for exact coverage and limits.

1. **Real low-space check for the current copy adapter.** Repeat the relevant isolated NTFS VHD case with the buffered main/ADS copy routine added for low-rate pacing. The previous successful real-VHD campaign used the earlier copy primitive; existing injected-error tests do not replace this check. Follow the [VHD runbook](directory-sync-plugin-vhd-runbook.md); no SMB test is needed.
2. **Final distribution check.** Build a fresh x64 package, check clean installation and update from published 0.3.0, English/Czech registration after restart, profiles and global settings (especially recovery and Stop at first error). Verify current-head CI, changelog, release notes and download links. Publish only after a separate release request, retaining the alpha/data-loss warning.

The next recommended step is item 1, the focused real-volume low-space check, followed by final distribution verification. No additional feature is needed to finish this release scope. SMB testing and SDK changes remain excluded; restoration UI and large-list optimization are separate roadmap work.
