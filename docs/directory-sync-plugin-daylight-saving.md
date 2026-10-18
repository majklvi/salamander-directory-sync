# Host comparison: daylight-saving time shifts

Investigation on 2026-10-18. No SDK or comparison implementation changes.

## Where the host setting lives

**Options → Configuration → General**:

- English: **Compare Directories: ignore time differences of exactly one or two hours**.
- Czech: **Porovnání adresářů: ignorovat časové rozdíly přesně jedné nebo dvou hodin**.

This is a global setting used by Compare Directories (Ctrl+Shift+F10), separate from its command dialog. The adjacent setting controls ordinary timestamp tolerance in seconds. Live inspection of the installed Samandarin 0.15.1 x64 Czech GUI confirmed the location: hourly-shift suppression was OFF, ordinary tolerance ON at 2 seconds. Configuration was closed with Cancel; neither setting was changed.

## Core behavior

The core configuration field is `IgnoreDSTShifts`; its default is false. It covers absolute differences of **one or two hours**, in either direction, rather than every difference up to one hour.

- Without time-resolution tolerance, `MyCompareFileTime` recognizes an exact 3,600- or 7,200-second difference.
- With time-resolution tolerance enabled, `CompareFilesByTime` compares whole-second values and also accepts a residual within the configured tolerance around either hourly shift. Thus a 2-second tolerance can accept 3,601 seconds, but does not turn a 30-minute difference into equality.
- Ignoring the timestamp shift does not discard the files or their other differences. Files can still be selected as different under other enabled criteria such as size and content.
- The core counts relevant hourly-shift pairs and can show an informational warning about ignored shifts, or a hint that shifts could be ignored. These warnings have separate confirmation preferences.
- Filesystem/date conversion rules are also involved in the core comparison. A future plugin implementation must not claim identical FAT/FTP behavior based solely on a raw NTFS timestamp test.

Local primary-source references in the host checkout: `src/mainwnd5.cpp` (`MyCompareFileTime`, `CompareFilesByTime`, `CompareDirectories`), `src/dialogs4.cpp` (defaults and General page), `src/cfgdlg.h`, `src/lang/lang.rc`, and `translations/czech/salamand.slt`. The reference `integration-0.16` checkout also retains the setting without exposing it through the SDK configuration IDs.

## Initial plugin state and SDK limitation

At the start of this investigation, the plugin had its approved inclusive **3-second** timestamp tolerance (`Options::tolerance`) and did **not** inherit the host's ordinary tolerance or hourly-shift setting. The ordinary tolerance is now inherited as described below. Enabling Ignore date is a different, broader operation and is not a substitute.

The pinned SDK exposes `SALCFG_COMPDIRSUSETIMERES` (60) and `SALCFG_COMPDIRTIMERES` (61) through `GetConfigParameter`, but has no ID for `IgnoreDSTShifts`. Inspection of the host's `CSalamanderGeneral::GetConfigParameter` implementation in `src/zip.cpp` confirms that the hourly-shift preference is not exported. The existing configuration-change event does not supply the missing value.

Reading a hard-coded registry value named `Ignore DST Shifts` is not a reliable replacement: it can be stale relative to unsaved live settings, and file-backed/portable configurations and multiple host configurations need not share that location. No registry workaround or dependency on host memory internals was added.

## Proposed integration

For true automatic inheritance, a future host/SDK change should expose the live BOOL through the existing `GetConfigParameter` mechanism. This requires no new virtual method, but it is still a host/SDK change and is outside the current approved scope.

When that value becomes available:

1. Read an immutable snapshot of the host time-comparison settings on the UI thread when Compare starts. Keep the existing result valid under that snapshot until a new comparison.
2. Normalize only the timestamp comparison for one-/two-hour shifts when enabled. Preserve actual timestamps for display, relative time differences, recovery and verification.
3. Preserve differences in size/content and blocked/conflicting entries. A pair differing only in an accepted shift should be Equal / no proposed operation, still available under the Equal filter.
4. Explain accepted shifts through the existing Reason/details/status UI rather than hiding the rows or introducing a mandatory modal dialog.
5. Verify enabled/disabled cases, both directions, one and two hours, tolerance boundaries, 30/90/180-minute differences, size/content differences, asymmetric mode, Ignore date, and changed host settings before a new Compare.

If SDK changes must remain excluded but this behavior is needed sooner, a separate remembered plugin preference is possible. That would be an explicitly independent setting, not automatic inheritance, and needs agreement before implementation. No such preference was added in this investigation.

## Follow-up: inherit ordinary time resolution

Following the user's additional request on 2026-10-18, the plugin now reads the existing `SALCFG_COMPDIRSUSETIMERES` and `SALCFG_COMPDIRTIMERES` parameters on the UI thread at every new Compare. This supersedes the ordinary-tolerance limitation above; the hourly DST limitation remains unchanged.

When enabled, the configured 0–3,600 seconds are inclusive. As in the core's enabled time-resolution path, timestamps are truncated to whole seconds for comparison. For example, with 2 seconds enabled, a difference of 2 whole seconds is accepted and 3 whole seconds is different. Disabled time resolution requires exact timestamp equality. Ignore date continues to override timestamp comparison entirely. Actual file timestamps, relative display, sorting, recovery and snapshot validation are not normalized or rewritten.

The value belongs to the comparison snapshot; host configuration notifications do not silently alter existing results or an active worker. A new Compare reads the latest live values. No duplicate plugin preference is stored. If the host fails to return valid parameters, the previous inclusive 3-second comparison remains as a compatibility fallback, with no whole-second truncation. A disabled host switch needs no numeric value and results in zero tolerance.

Regression checks cover enabled/disabled settings, 0/1/2/5/3,600 seconds, invalid or unavailable values, refresh/reset and snapshot independence, whole-second boundaries in both directions, exact subsecond differences when disabled, size/content differences, and Ignore date. This follow-up does not implement hourly-shift suppression and does not modify the SDK. The packaged English/Czech guides describe the new inherited behavior; published release assets remain unchanged until explicitly updated.

Local validation passed: Debug Win32/x64/ARM64 and Release x64 plugin builds; all five x64 suites in Debug and Release (engine 561, file operations 3,729, lifecycle 2,367, English resources 2,477, Czech resources 2,514 checks in each configuration). ARM64 was compiled, not run. The configuration reader was exercised with an injected SDK-query provider; the rebuilt plugin has not yet been loaded into the live host for this follow-up. No SMB tests were run.

## Live GUI validation of inherited seconds

Completed later on 2026-10-18 in the installed Samandarin 0.15.1 x64 Czech/light-mode host. The previous plugin was unloaded before deployment. The new Release SPL and both SLGs matched the build outputs by SHA-256; the SPL hash was `475284a5eba2cba735ac074ef02d3895a0fea8c7ad6cdfd82e800ef087ca4e16`.

Fixture outside Git: `C:\Samandarin\builds\dirsync-time-tolerance-20261018`, eight file pairs (16 files), base timestamp 2026-10-01 12:00:00 UTC. All pairs have identical four-byte contents except the final size-difference pair. Compare used `*.*`, subfolders ON, content/asymmetry/Ignore date OFF and all result filters ON.

| Pair | Difference | Tolerance ON, 2 s | Tolerance OFF |
| --- | --- | --- | --- |
| `00-equal.txt` | Identical times | Equal, no proposal | Equal, no proposal |
| `01-left-1s.txt` | Left newer by 1 s | Equal, no proposal | Copy right |
| `02-right-2s.txt` | Right newer by 2 s | Equal, no proposal | Copy left |
| `03-left-3s.txt` | Left newer by 3 s | Copy right | Copy right |
| `04-right-3s.txt` | Right newer by 3 s | Copy left | Copy left |
| `05-subsecond.txt` | Left newer by 0.5 s | Equal, no proposal | Copy right |
| `06-left-2_9s.txt` | Left newer by 2.9 s | Equal at whole-second precision | Copy right |
| `07-size-within.txt` | Left 5 B / right 4 B; left newer by 1 s | Different, choose direction | Copy right under timestamp precedence |

Observed counts: ON = five equal pairs / three differences; OFF = one equal pair / seven differences. All eight rows remained visible, with no blocked rows. Relative times continued to display actual differences even when a pair was Equal within tolerance.

The host's General-page switch was changed in the GUI and applied with OK. The host and plugin stayed loaded between comparisons; only the plugin preview was closed/reopened to access host configuration. There was no separate Save Configuration or restart, confirming that the SDK supplied live values. After OFF testing, the original ON/2-second value was restored and another Compare returned the original five-equal/three-different result. Hourly DST suppression remained OFF throughout.

No new defect was found. All 16 files retained their paths, hashes, sizes and write times; no files appeared in either tree. No synchronization or immediate deletion command was invoked. The final ON/2-second preview remains open. No SMB tests or SDK changes.

The conditional integration plan is recorded in [the design](directory-sync-plugin-design.md): if upstream accepts the hourly-shift API (for example for 0.16), implement automatic inheritance only after the corresponding host/SDK version is actually released. The upstream request itself remains a local draft outside tracked files and has not been submitted or uploaded to GitHub.
