# Progress frames and compact error paths — 0.4.0

Local follow-up, 2026-10-21. No SMB test, SDK change or executor change.

## Progress-bar source comparison

Reviewed host `src/lang/lang.rc` (`IDD_PROGRESSDLG`), `src/dialogs.cpp` (`CProgressDialog::DialogProc`), `src/gui.cpp` (`CProgressBar::Paint`) and `src/plugins3.cpp` (`CProgressBarForPlugin`, `CSalamanderGUI::AttachProgressBar`) at `e1efc2863e4bbee5ee78f27cacc8d8bcf7ed31d4`. The same frame painting is present in the `5.0-samandarin-0.15.1` tag.

The host has borderless Static placeholders at `(45,40,297,12)` and `(45,55,297,12)` DLU. Its progress control draws the fill, centered percentage and a single-pixel frame using `BtnShadowPen`. `AttachProgressBar` constructs a subclass of that same `CProgressBar`, without replacing its painting. The pinned SDK explicitly says that this control draws its own frame and should not receive `WS_BORDER` or `SS_WHITEFRAME`.

The plugin already uses those exact placeholder bounds and the host's `AttachProgressBar`. No separate frame is added on the normal shipping path; no replacement painter is needed. A concrete difference existed in the allocation-failure/standalone-harness fallback: the standard Windows progress control additionally received `WS_BORDER`. This flag is removed. The fallback remains a Windows common control, not a claim of identical SDK rendering, and its progress/indeterminate behavior is preserved. Native resource tests and runtime fallback tests guard against additional window/client/static-edge borders.

## Error-path presentation

The common File operation error dialog keeps complete wide source/target values in its read-only native controls. When a path is not focused, it now displays an ellipsis, measuring the available width using the actual control font. As with the host's `CStaticText` path-ellipsis policy, it preserves the final filename first. If the filename itself exceeds the field width, it preserves its tail and extension rather than silently clipping the end. UTF-16 surrogate pairs are not split. A fitting path is shown unchanged; literal ampersands remain literal.

Focusing the path restores ordinary native selection and horizontal scrolling. Selecting the full line with Home, Shift+End and copying with Ctrl+C retains the complete value, not the shortened display. Full paths are also supplied by tooltip callbacks. Only presentation changes: Retry and the worker still use the unchanged full paths. The progress window already has path-ellipsis static labels and complete-path tooltips; editable directory inputs are intentionally not shortened while the user is entering a path. Existing table/detail copying is unchanged.

## Verification

All five x64 suites pass in Debug and Release: engine 2,216; copy/recovery/overwrite/delete 6,730; lifecycle 2,451; English resources/layout 5,516; Czech resources/layout 5,656. The lifecycle probe now includes additional checks within its existing aggregate tests: measured short/long/UNC/Unicode/ampersand paths at several widths, extension retention, surrogate validity, complete tooltip values, full native selections and unchanged control text after offscreen painting. Runtime fallback bars have no added `WS_BORDER`, `WS_EX_CLIENTEDGE` or `WS_EX_STATICEDGE`. Debug Win32/x64/ARM64 and Release x64 plugin builds pass; ARM64 is compilation only.

Computer Use inspected the final Release SPL in the isolated custom Samandarin 0.16 x64 host at `C:\Samandarin\builds\dirsync-progress-paths-20261021\cs-dark\host`, first Czech/dark and then English/light, using only that instance's file-backed configuration. Both final presentations display the ellipsis and the `.txt` tail of a 267-code-unit Unicode path with a deliberately oversized filename. After focusing and selecting the line, Ctrl+C produces a string exactly equal to the fixture's complete source path. The native SDK progress bars were visually inspected in both modes by moving the outstanding error question aside. The normal SDK frame remains unchanged. This is not a pixel-diff comparison with a simultaneously running core transfer, nor a live test of SDK allocation failure.

An exploratory core-copy attempt on the long CJK source failed before transfer with system error 2 and was cancelled; it did not establish a live core progress-bar comparison. The border comparison above rests on the host resources and shared painting implementation. No host-code fix is included. The private missing-salmon startup warning was acknowledged.

The first plugin command copied only `0-done.txt`, then waited on the locked middle source; later commands only retried the two still-missing targets and were cancelled. No overwrite or deletion was planned. Independent checks confirm all source hashes/ADS/timestamps unchanged, exactly the completed `0-done.txt` in the destination with matching data/ADS/time, no pending target or staging residue, and an empty recovery directory. The intentional lock was released and private GUI instances closed. Local evidence includes `fixture.json`, `final-question.txt`, `final-question-light.txt`, `clipboard-check.json` and `disk-verification.json`. Final test SPL SHA-256: `cb61bb47482ada3660cd74d93332bda55d830746db6c6d74a3704ef0e45395a4`.

The live evidence above was collected before publication of these development changes. The complete local test suites and build matrix were also rerun successfully alongside the subsequent Czech Cancel terminology update, before this commit; no implementation changes followed those checks. Cloud CI results for the published commit are reported separately. No new release is published.
