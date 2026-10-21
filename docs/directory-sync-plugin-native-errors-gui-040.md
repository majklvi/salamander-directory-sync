# Native operation-error dialogs and pending-question lifetime — 0.4.0

Verified 2026-10-21. Version 0.4.0 is in development; this is not a release or an SMB test.

## Host source and adaptation

Reviewed the host checkout at `e1efc2863e4bbee5ee78f27cacc8d8bcf7ed31d4`: `src/lang/lang.rc`, `IDD_CANNOTOPEN`, and `src/dialogs.cpp`, `CFileErrorDlg`. The basic host dialog uses MS Shell Dlg 8, a 350 × 81 DLU layout, right-aligned short field labels, plain error text and centered Retry / Skip / Skip all / Cancel buttons. The source is GPL-2.0-or-later; the adapted resources retain a provenance comment and the existing copyright/license headers.

The pinned SDK exposes synchronous `DialogError(HWND, DWORD, const char*, const char*, const char*)` with one narrow filename and no externally managed question handle. Direct reuse would change the plugin's two complete Unicode path fields and its cancellable modeless question lifetime. Therefore the plugin retains its existing `CreateDialog`/worker decision channel and adapts the host's resource layout rather than invoking the blocking SDK dialog. No SDK or executor change is required.

The plugin layout is 350 × 104 DLU: the extra space accommodates operation/phase context and separate source/target lines. Paths use borderless, read-only, horizontally scrolling native controls, preserving selection and complete Unicode values. The error description is an ordinary wrapping Static caption. The English button grid follows the host's 50 × 14 DLU buttons; Czech uses 56 × 14 DLU so **Přeskočit vše** fits, with **Storno** matching the host's Czech convention. Colors follow the host. Retry/Skip/Skip all/Cancel semantics are unchanged.

Shared confirmation/result explanations are also ordinary wrapping captions, without a Notes/Poznámky label, edit border, scrollbar or Tab stop. Initial height measures the complete explanation. The existing resizable table, Ctrl+C value copying and Ctrl+Shift+C full-summary copying remain available; the full summary still includes the explanation.

## Isolated live setup

Computer Use operated four private configurations of the custom Samandarin 0.16 x64 host, each with its own file-backed configuration. The host executable SHA-256 was `f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd`.

Fixture and local evidence: `C:\Samandarin\builds\dirsync-native-errors-gui-20261021-r2`. Each pair had three new files in `left-žluťoučký`, an empty `right-漢字` and a separate empty `recovery-漢字`. Each source had an 18-byte `note` ADS. The middle source used Czech and 175 CJK characters, producing a complete path of 273–274 UTF-16 code units. An exclusive read lock caused system error 32 before its copy could be published.

Only **Copy visible new items…** was run. There were no overwrites or deletions. Recovery ON/OFF below identifies the frozen command setting, not a new test of original preservation, because these plans contain no existing targets.

| Language / host appearance | Keep originals | Pending-question lifetime test | Result |
| --- | --- | --- | --- |
| English / light | OFF | Host close; decline forced unload, then close again and accept | Declining preserves host/question; accepting closes all private host/plugin windows and the process |
| English / dark | ON | Plugin Manager → Unload → save configuration → force unload | Host survives; SPL and English SLG are unmapped; plugin reloads into the same PID and compares successfully |
| Czech / light | OFF | Správce pluginů → Uvolnit → save configuration → force unload | Host survives; SPL and Czech SLG are unmapped; plugin reloads into the same PID and compares successfully |
| Czech / dark | ON | Host close and accept forced unload | All private host/plugin windows close; retained process observer records exit code 0 |

For every case, the first command copied `0-done.txt`, then waited on the locked middle item. Its confirmation, error question and cancelled new-item result were visually inspected. Text, button captions, layout and host-controlled contrast were readable in all four combinations; no Notes/Poznámky edit field remained. Czech's longer button caption fits. The Windows error description remains in the operating system's available language (English on this machine), while plugin context/labels/buttons use the selected language.

Keyboard checks included English Alt+R and Czech Alt+O retrying the still-locked item, Tab moving from Retry/Opakovat to Skip/Přeskočit, Escape cancelling the command and Enter dismissing its result. The result reports one completed item, one cancelled item and one unstarted item. Before the lifetime test, a fresh Compare showed the completed file equal and only the two remaining files eligible for new copying. The next command waited on the same locked source; its question was left unanswered during host shutdown/unload.

The host remained usable while the question was pending: its panels could be activated and a completed target selected; its menu and Plugin Manager remained accessible. Direct native unload declined while the plugin window was open, as designed. Accepted forced unload stopped the waiting worker and removed its question/progress/preview windows. Module inspection, not just the Manager's loaded-state text, confirmed actual SPL/SLG removal in both Manager cases. Reopening the command loaded both modules again in the same surviving host process, without automatically starting a file operation.

The English/light process disappeared after the native close sequence, but the attached observer did not retain a numeric exit code; it is not reported as exit code 0. The Czech/dark observer retained its launch handle and did record 0. No process was killed for these tests. The private host's missing `salmon.exe` startup warning was acknowledged; no normal user configuration was changed.

## Independent disk and build checks

After closing all four private instances and releasing the deliberate source locks, a separate filesystem check verified:

- All twelve source data hashes, ADS contents and modification timestamps match the prepared baseline.
- Each destination contains exactly `0-done.txt`, with matching data hash, ADS and modification time. Neither pending file nor a staging item remains.
- All four recovery directories remain empty, as expected for new copies.
- Every formerly locked source can be opened for update without writing. No test host or plugin window remains.

Local `fixture.json`, `*-confirmation.txt`, `*-error.txt`, `*-result.txt`, `*-unload-pending.txt`/`*-shutdown-pending.txt`, module snapshots, exit observations and `disk-verification.json` retain the evidence outside Git. The test SPL SHA-256 is `bd8987ccaa097c126c5562a4977c189d36b3a8c155f4c12d87fce6c64a46e269`; English SLG `e32121f84ef25e0385b17b99760a913e0479c1698f2293e04cb7e10c69afc2e7`; Czech SLG `f0789d625dc089cec78ab8b010868b8b17918b247bdf40600912117be66e74e6`.

All five x64 suites passed in Debug and Release: 2,216 engine assertions, 6,730 copy/recovery/overwrite/delete assertions, 2,451 lifecycle checks, 5,516 English and 5,656 Czech resource/layout checks. Native probes check long plain explanations at the initial size, after enlargement and after restoration, including Unicode and literal ampersands; real error questions check the caption class and measured fit. Resource checks cover the host button grid, borderless selectable paths and compiled bounds/caption metrics at 96, 120, 144, 192 and 240 DPI. Debug Win32/x64/ARM64 and Release x64 plugin builds passed. ARM64 is compilation only; live inspection used the current desktop scale, not a new monitor-DPI campaign.

The narrower Czech button grid needed a small localization width adjustment during preparation; final live checks found no further defect. No lifetime/executor fix was necessary. Mixed Retry/Skip/Skip all and Stop/Close during recovery, overwrite and deletion retain the separate [combined GUI evidence](directory-sync-plugin-combined-errors-gui-040.md) and automated coverage; they were not rerun as destructive operations here. SMB, SDK changes, Windows logoff/hard termination and a new distribution package are outside this campaign.

## Remaining release work

Next inspect CSV/text exports and Stored originals against a settled mixed-result fixture, then perform the final clean-install/update package checks listed in the [release checklist](directory-sync-plugin-release-readiness-040.md). No additional feature or release publication is included here.
