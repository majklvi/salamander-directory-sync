# Native speed-limit dialog alignment

2026-10-19, 0.3.0 development. Existing SDK; no SMB tests.

## Host source and reuse boundary

Read-only inspection of the archived Samandarin source at `eae895ca696fef639664a2cc373fb0654006e648`:

- `src/lang/lang.rc:2317`: `IDD_SETSPEEDLIM`, 234 × 88 dialog units, MS Shell Dlg 8. Two radio buttons at y=19/32, a 34-unit numeric input, a 35-unit units list, separator at y=61, OK/Cancel/Help at x=33/92/151, y=67.
- `src/dialogs.cpp:3260–3373`: `CSetSpeedLimDialog` initializes the two choices, enables inputs only for the limit choice, chooses B/s/KB/s/MB/s/GB/s using exact powers of 1024, validates on OK and retains the prior value on Cancel. Its unlimited/default value is displayed as 4 GB/s.
- `src/dialogs3.cpp:1388`: `GetSpeedLimit` accepts positive integral quantities and surrounding whitespace; the core stores a DWORD and handles 4 GiB/s as its maximum DWORD special case.
- `translations/czech/salamand.slt`, dialog 2830: native Czech captions and footer. The original translated label extends past the unchanged edit position; that overlap was also observed live.

The pinned SDK contains neither `CSetSpeedLimDialog` nor a public function/service to invoke it. `spl_gui.h` exposes native progress/split controls and `ArrangeHorizontalLines`, but no speed-choice dialog. The implementation belongs to the core and its language resources. Calling private addresses or hard-coding a host resource/module lookup would not provide a supported standalone plugin interface. No SDK, host checkout, ABI or vendor dependency was changed.

The plugin therefore adapts the GPL-licensed resource arrangement and behavior using ordinary native controls. The core dialog source credits **2023 Open Salamander Authors**, GPL-2.0-or-later; the Czech translation credits **Hynek Mikulič**. The adaptation is attributed in `src/transfer_speed.inc` and retains the repository's GPL license. It does not claim to invoke the actual core dialog. Help explains the plugin's own worker rather than opening a core help page with different transfer semantics.

## Changes

- Replace the one-row checkbox layout with the host's two radio choices and 234 × 88 layout, including the three-button footer and localized Help/F1.
- Use native B/s, KB/s, MB/s and GB/s units; load arbitrary existing byte rates exactly without rounding up to KB/s. A disabled limit shows the same 4 GB/s initial choice as the host.
- Keep the plugin's existing 64-bit limit ceiling of 1 TiB/s; it is not restricted to the core's DWORD representation. Zero still means unlimited. This differs from the core's numerical ceiling, not its UI arrangement.
- Retain the per-batch setting, pause state, cancellation and unthrottled verification. Showing this dialog does not unpause a previously paused transfer. Cancel leaves the current limit intact.
- Use the same English coordinates as the core. In Czech, place the edit at x=126 and units at x=166 after the longer label, within the same window width; align the heading separator through the existing SDK helper. Do not reproduce the native translation overlap.

## Verification

Computer Use compared both actual dialogs in the same private Czech/dark Samandarin 0.16 x64 host, preserving installed user instances. Fixtures were under `C:\Samandarin\builds\dirsync-speed-dialog-20261019`, outside Git.

1. Started an ordinary F5 copy of synthetic `a-large.bin` (1,879,048,192 bytes) into initially empty `native-right`, with an initial 1 MB/s core limit. Opened the core progress split menu and its speed-limit dialog. Observed the two radio buttons, all four units, native footer, dimensions and Czech label/input overlap. Cancelled the speed dialog, then cancelled that isolated transfer through the core's own confirmation. The target directory was empty afterward.
2. Loaded Release SPL SHA-256 `3c1af1c1aa4732c63fb0740bb6e155f2dc106dde0bbbb752a583e94b1582ec82` (713,728 bytes). Compared the existing three-file synthetic Unicode source against initially empty `plugin-right`. Confirmed exactly three new copies, no overwrites/deletions, recovery OFF.
3. Paused during Verify, opened the new speed dialog and compared its 353 × 174-pixel outer window with the actual native dialog at the current system DPI. Confirmed the unlimited radio selection, disabled inputs and 4 GB/s initial display. Opened Help and returned; enabled the limit and expanded the four-unit list. Czech captions were fully visible without overlapping the inputs. Selected 4 MB/s and accepted; pause remained set. This checks dialog behavior, not a new throughput benchmark.

Production dialog probes run both language resources and verify radio exclusivity, input enable/disable, all four units, default display, exact 17 B/s round trip, disabling via OK and unchanged values after Cancel. Parser checks cover unit scaling, surrounding whitespace, zero/negative/fractional/malformed inputs and overflow. Existing nested-dialog shutdown and transfer lifetime tests remain enabled. Resource/layout tests check the native grid, non-overlap and caption metrics at 96/120/144/192/240 DPI; these are not live monitor-DPI tests.

All five x64 suites passed in Debug and Release: engine 2,216; copy/recovery/export 4,460; lifecycle 2,369; English layout 5,147; Czech layout 5,277. The plugin matrix passed Debug Win32/x64/ARM64 and Release x64; ARM64 was compiled, not run. An initial parser regression still expected the old KB/s unit at index zero; adjusting its overflow input to the new B/s boundary made the unchanged parser range check pass. No production failure was hidden by that test update.

The resumed GUI batch finished 3/3 without errors or unstarted items. Target verification is recorded in `verification.json` under the fixture root. The completed large synthetic target was removed through filesystem tools after matching its source SHA-256, to retain disk space; the source and small targets remain. The private host was closed normally after testing.

The local development package `out/package-030-native-speed/dirsync-0.3.0-x64.zip` passed verification of all 14 extracted payload files; archive SHA-256 `29fd2f3a394b97c07dad291b6a5a10b45c9fd19b32af8e18399e17a5195b3074`. No release was published.

These are local verification results; GitHub CI is checked separately after publication. Live coverage here is Czech/dark at one DPI; English and other DPI layouts are covered by compiled-resource tests.
