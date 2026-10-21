# Compact synchronization confirmation: research and proposal

Date: 2026-10-21. Status: reference GUI research and plugin source analysis completed; the proposed plugin UI and direction selection are **not implemented**. Plugin baseline: `30e726554e2f136d07d49d86a07737ed6d2b149a` (0.4.0 development).

Implementation follow-up: the user approved this design; see the [0.4.0 implementation and verification](directory-sync-plugin-sync-confirmation-040.md). The observations and proposal below retain their research-time status.

## Method and boundaries

Computer Use inspected the installed reference file manager, x64 version 11.58. Only Compare, display filters, comparison options and the synchronization confirmation controls were exercised. Every synchronization confirmation was cancelled; no reference copy, overwrite or deletion was started. No disassembly was necessary. The conclusions describe observable behavior in these cases, not proof of every internal branch or a claim about other releases.

The isolated NTFS fixture is outside Git at `C:\Samandarin\builds\dirsync-direction-dialog-20261021-e43ec16e`, with `left-ž` and `right-漢字`. Initially it contains:

- Twenty left-only files, 9 bytes each.
- One right-only file, 524,288 bytes.
- Two paired names, 14 bytes per side: one newer on the left and the other newer on the right, with a two-day difference. Their contents match; content comparison was disabled for the time-based cases.
- One equal pair, 14 bytes per side, with identical timestamps and contents.

The two roots contain 27 physical files. Matching-file masks provide additional one-sided and equal cases. For the genuinely empty-root cases, fixture preparation temporarily moved files to sibling holding directories outside both roots, then restored them. These preparation moves were not synchronization operations. The final check confirms all 27 original paths, SHA-256 hashes, lengths and last-write timestamps match `before.json`; see the local `after-verification.json`. An initial checker compared an automatically parsed JSON DateTime to its formatted string and reported false mismatches; comparing UTC ticks corrected that checker, without changing fixture data.

The initial command-line private-config launch did not leave a running process. The GUI campaign therefore used a newly launched installed instance, not a claimed private-configuration session. Its original mask, display toggles, Asymmetric and By content choices were restored. The main file panels were not navigated or operated on. The instance opened for this campaign was closed; existing host/plugin windows were left open.

## Observations

| Case | Copy left to right | Copy right to left | Observed confirmation state |
| --- | --- | --- | --- |
| All names, symmetric, date comparison | 21 files / 194 B | 2 files / 524,302 B | Both checked and enabled, despite the strongly unequal counts and bytes. |
| Uncheck one direction in confirmation | Same available counts | Same available counts | The two checkboxes are independent. Unchecking does not turn proposals around. |
| Uncheck both directions | Same available counts | Same available counts | Both can be unchecked. OK still appears enabled. It was not pressed. |
| Cancel and reopen | 21 / 194 B | 2 / 524,302 B | Both nonempty directions are checked again; cancelled choices are not retained in this case. |
| Hide all rightward proposals using the display toggle | 0 | 2 / 524,302 B | Rightward choice unchecked and disabled; leftward choice checked. Physical files have not changed. |
| Hide both directional categories | 0 | 0 | Main Synchronize button disabled. |
| Show rightward proposals only | 21 / 194 B | 0 | Only the rightward choice checked/enabled. |
| Compare mask `l-only-*` | 20 / 180 B | 0 | Only the rightward choice checked/enabled. |
| Compare mask `r-only*`, symmetric | 0 | 1 / 524,288 B | Only the leftward choice checked/enabled. |
| Same right-only mask, Asymmetric, then Compare | 0 | 0 | Both copy choices disabled; a separate right-side deletion choice is checked for 1 file. No deletion was executed. |
| Compare mask `equal*` | 0 | 0 | One equal pair found, hidden by the unchanged equal display toggle; Synchronize disabled. |
| Entire right root genuinely empty, symmetric | 23 / 222 B | 0 | Only the rightward choice checked/enabled. |
| Entire left root genuinely empty, symmetric | 0 | 4 / 524,330 B | Only the leftward choice checked/enabled. |
| Both roots genuinely empty | 0 | 0 | Zero results and Synchronize disabled. |

The initial mixed case had just one highlighted row, yet confirmation counted all eligible visible proposals. Highlighting was not the execution scope.

The compact reference dialog has two independent directional checkboxes with counts and bytes, destination-path fields, separate deletion controls, and options for overwrite questions, verification and NTFS permissions. A zero-count direction also disables its destination field. A manually unchecked nonempty direction leaves its path displayed. The deletion-of-empty-directories control was disabled in the inspected cases; this campaign does not establish its eligibility rules. Destination redirection and the effects of the advanced options were not tested.

**Supported inference:** the observed defaults and availability can be explained by eligible visible proposals in each operation category. There is no evidence here of choosing a dominant direction based on the total number or size of files. Existing file dates affect the proposals produced by Compare; the confirmation then offers their directions. Hidden proposals are excluded. This is a behavioral model, not recovered implementation code. Persistence after actually executing a restricted reference batch was not tested.

## Proposed plugin dialog

Use a compact host-style dialog, approximately 300 DLU wide initially, measuring English/Czech captions at the supported DPI values. Use the established font, spacing, native controls and host light/dark styling. No new SDK interface is needed.

1. Show left/right roots as compact path captions with middle ellipsis and full-path tooltips. They identify the compared roots and are not editable destinations in this confirmation.
2. Offer independent **Left to right** and **Right to left** checkboxes. Each direction shows the number of new files, empty directories and overwrites, plus formatted source-data bytes. Both may be selected. Default to all directions with eligible visible proposals; disable and clear a zero-count direction. No count/volume heuristic or remembered restriction from a previous batch.
3. If the plan includes asymmetric deletion, show a separate **Delete on the right** checkbox with file/directory counts. It defaults to the already planned deletion set, preserving current combined-command semantics. Disabling a copy direction does not silently enable, disable or reverse deletion. Deletion remains explicit and conspicuous.
4. Always show **Keep originals: ON/OFF**. When overwrites/deletions are selected with recovery OFF, keep a short visible warning that originals are not saved and the Recycle Bin is not used. When ON, expose the recovery location compactly, with the full path accessible. Do not hide this information behind Details or change the remembered global preference here.
5. Provide **Details…**, **OK** and **Cancel** (Czech **Podrobnosti… / OK / Zrušit**). OK is disabled when no eligible operation remains selected, unlike the observed reference all-unchecked case.
6. Details opens the existing table-style information dialog in read-only mode, populated for the currently selected subset. Closing Details returns to the same compact confirmation and never authorizes execution. Only the compact dialog's OK starts the operation; there is no second mandatory confirmation.

The direction choices **restrict existing proposals**. They do not force an older source over a newer target, generate missing proposals, include hidden or Skip rows, or change the comparison mode. Changing proposals remains a separate list/context-menu operation. Show a short scope caption or tooltip explaining that highlighted rows do not limit the command.

Recommended navigation: make the main Synchronize action open this compact combined confirmation directly. Retain the separate copy/overwrite/delete commands and export under the existing Visible plan context menu. Preserve the adjoining Stop part and current keyboard access. Result/error dialogs are outside this confirmation redesign; the proposal does not remove required error questions or operation results.

Do not copy destination redirection, optional data verification or permission-copy toggles from the reference dialog: the plugin must preserve its compared roots, mandatory verification and target-permission policy. The empty-directory deletion shortcut must not widen the plugin's explicit empty-only deletion plan.

## Implementation map and consistency requirements

- `src/dirsync.cpp`: `SynchronizeMenu` currently opens a command menu; `SyncAll` calls `BuildSyncPlan(p.result,p.visible)` and an `InformationBox` confirmation with all counts. Extract one summary model for compact captions and Details, rather than maintaining two calculations.
- `src/copy.cpp` / `src/copy.h`: `BuildSyncPlan` splits new copies, overwrites and validated asymmetric deletions. Add an explicit per-command selection or confirmed-plan value. Use that same selection for confirmation, execution and dependency checks. Keep the complete comparison snapshot and stable row identities; do not remove model rows or reverse actions to implement direction selection.
- `DeletionRows` currently treats nonselected descendants as retained and protects their ancestors. Preserve that rule for hidden, skipped, blocked and direction-excluded descendants, as well as actual remaining on-disk children. Never infer permission for recursive deletion.
- `src/report_ui.inc` and `src/report.cpp`: `BeginReport` / `PrepareReport` currently derive operations from the full visible set. They must use the confirmed subset too. Preserve a distinction between actual UI visibility and exclusion by the confirmation choices; do not describe a disabled direction as hidden, execution-skipped, failed or completed. Record the chosen directions/deletion selection explicitly in CSV/text batch metadata.
- `TransferBudget` derives counts/bytes from prepared report operations. Build it from the confirmed subset, including only recovery copies needed by selected destructive operations. Keep source-data estimates distinct from total I/O, ADS, staging and verification work.
- The details rows, worker input, recovery work, result denominator and export must describe the same immutable confirmed plan. Changing confirmation checkboxes must not edit filters, manual proposals, profiles or stored recovery preferences.
- Preserve current revalidation and R1–R5: external changes while either confirmation window is open can invalidate execution; Retry, Skip, Skip all, Stop/Close, fatal/shared-log errors and phase order remain unchanged. Details is owned by the compact confirmation, which is owned by the modeless plugin window; lifetime/unload handling must cover both.

Suggested implementation sequence: (1) shared selection/summary model and focused tests; (2) compact EN/CS confirmation and read-only Details; (3) connect execution, progress and report metadata to the identical subset; (4) focused GUI verification and distribution checks. There is no need to replace the copy/recovery/deletion adapters or expand the SDK.

## Focused acceptance after implementation

- Both directions, each direction alone, zero choices, zero-count directions, unequal counts/bytes, equal-only and empty roots.
- New files, empty directories and overwrites in both directions; hidden, highlighted, Skip and blocked rows retain their existing scope semantics.
- Asymmetric deletion alone and mixed with copies, recovery ON/OFF, protected ancestors and no implicit recursive deletion.
- Counts and bytes change correctly; Details agrees, closing Details does not start work, Cancel leaves files/proposals/filters/settings unchanged.
- Progress/result denominators, recovery records and CSV/text describe only authorized operations and distinguish confirmation exclusion from execution skips.
- Revalidation after a file changes while the dialog is open; Stop/Close, pending errors, host shutdown and forced unload with confirmation/Details open.
- English/Czech, light/dark, long Unicode roots, keyboard focus and supported DPI values. Use ordinary isolated NTFS fixtures; SMB and SDK changes remain excluded.

This report adds no executable code, performs no plugin synchronization and makes no claim that the proposed dialog is already shipped. No commit, push or release is part of this research step.
