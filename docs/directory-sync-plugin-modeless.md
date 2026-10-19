# Modeless synchronization window — 0.3.0 development

Date: 2026-10-19. Local NTFS only, existing pinned SDK, no SMB tests.

Follow-up evidence for [host shutdown and forced unload](directory-sync-plugin-modeless-unload.md) distinguishes the new real-transfer native regressions from live host checks still pending after a Computer Use input failure. Historical validation counts and unpublished-worktree statements below describe their original development stage.

## Behavior and implementation

The preview now uses `CreateDialogParamW` and returns immediately from the menu callback. Its heap-owned state outlives that callback. It is an independent top-level window with its own taskbar entry, rather than an owned modal window that disables or stays in front of Samandarin. One preview is supported; invoking the command again activates the existing window. Its captured paths and selected entries remain independent of subsequent panel navigation.

The window and all SDK/UI calls stay on the host UI thread. A thread-local `WH_GETMESSAGE` hook routes keyboard messages for the preview and its progress window through `IsDialogMessageW`; it does not process unrelated host windows or plugin modal children. The hook is removed when the preview is destroyed. The file worker remains separate. This avoids introducing cross-thread SDK/configuration access or requiring a new host API. The core's own modeless plugin examples use separate message loops, but their private queue/thread infrastructure is not imported into this standalone plugin.

Settings, confirmations and result dialogs are owned by the plugin preview. They can remain locally modal without disabling the host main window. The plugin's own controls remain locked during an operation. Host panels can be used independently; host changes to compared files still undergo the existing revalidation before execution/publication. Configuration-change notifications defer rebuilding group headers while a comparison worker may be replacing its result.

Close during work requests cancellation and waits for completion before closing. The modified-profile Save / Don't save / Cancel question remains. Actual window destruction also cancels and joins the worker and destroys the progress window before clearing the preview pointer. Closed preview state is retained until the next launch or unload to let nested callbacks unwind. Ordinary unload refuses an open preview; forced unload synchronously stops/joins work and destroys it. The SDK explicitly ignores a FALSE return during forced release: if forced release reenters an existing plugin callback through a nested modal message loop, the module is pinned in memory until process exit so its frames cannot return through unmapped code. This exceptional path is different from ordinary forced unload and was not exercised live. No background worker is intentionally left running.

The initial size/position is capped to the host monitor's working area, including the minimum tracking size. The existing resize layout gives the remaining height to the result list, keeping the title and status bar accessible on a smaller display. Version resources, plugin-manager version and packaging now use 0.3.0; it remains an unpublished ALPHA development build.

## Automated verification

`ProbeModelessPreview` uses real production keyboard, Close, destroy and Release handlers with a minimal native dialog on the same UI thread. It verifies that the synthetic host stays enabled, receives its own queued message, and is not the preview's owner; Tab moves focus between controls; a deliberately oversized window fits the monitor working area; Close cancels a waiting worker; destruction clears state and the hook; and forced release joins work and destroys the modeless window. Existing suites retain scan/content/copy/Verify/recovery/overwrite/deletion Stop/Close and immediate-FreeLibrary coverage.

All five x64 suites pass in Debug and Release: engine 2,216; copy/recovery/catalog/export 4,460; lifecycle 2,369; English resources/layout 5,075; Czech resources/layout 5,195. Resource metrics cover 96/120/144/192/240 DPI. The plugin build matrix is Debug Win32/x64/ARM64 and Release x64; ARM64 is a compile check, not a runtime test. Local results are distinct from GitHub CI, which has not run for these uncommitted changes.

Final Release SPL SHA-256: `3586b181733bd781ef1f9d87e0b11cf216c770d3b98b5339711a24c784e8346c`. The local package script verified all 14 payload files and extracted hashes in `out/package-030-modeless/dirsync-0.3.0-x64.zip` (archive SHA-256 `1827742742ec4fb103da7eb1fd7473e146de874977527ff9af2c21a61dd55010`). The package identifies its source as a modified development worktree. It was not published; public README downloads still point to release 0.2.1.

## Live GUI verification

Computer Use loaded Release SPL SHA-256 `9af9daa3b42cc97f74c90d7ba6572e17ef1e192535b54d344a5d4b589b4caa8b` in the private Czech/dark 0.16 x64 host under `C:\Samandarin\builds\dirsync-progress-gui-20261019\host-compact`. This binary preceded only the final monitor-position correction. Installed user host instances were preserved.

1. Opened the preview from Plugins. It appeared as a separate top-level window. Switched to the host and opened `C:\Program Files` in the right panel while the preview remained open.
2. Returned to the same preview: its captured right path was still `C:\`, despite host navigation. Set only the preview's right path to `C:\Samandarin\builds\dirsync-native-controls-20261019\right` and enabled By content. The left path remained the isolated `compact-left-漢字` fixture.
3. Started comparison without synchronization. The live preview showed content comparison, an enabled comparison Stop and disabled operation controls. Switched to the host main window; navigated its right panel back to `C:\`.
4. Returned to the preview. Comparison completed with exactly three binary-content matches, including the 1,879,048,192-byte `a-large.bin`, and the original comparison paths were preserved. No synchronization, overwrite or deletion was invoked.
5. Closed the idle preview with its Close button and verified the host remained available. The private host was then closed normally.

These checks demonstrate live host use with an open preview and switching away from an active content comparison. They do not repeat live background synchronization, forced unload inside a child modal dialog, or host shutdown during a transfer; the worker/close/unload paths have automated coverage. The final monitor-position correction is covered by the oversized-window regression rather than this live binary. See the [transfer-progress report](directory-sync-plugin-transfer-progress.md) for the separate native progress-control, pause and transfer tests.

## Live modeless transfer verification

2026-10-19 follow-up, Czech/dark private Samandarin 0.16 x64, Computer Use. This adds actual synchronization coverage to the earlier comparison-only session. Installed user instances were left open. The initial optimized Release SPL was 713,216 bytes, SHA-256 `f8ebb31d399675f1743821d82437055aa9b4df849e86718ca76f1b9dcee6f39c`.

The source was `C:\Samandarin\builds\dirsync-progress-gui-20261019\compact-left-漢字`: `0-done.txt` (13 bytes), `a-large.bin` (1,879,048,192 bytes), `z-next.txt` (13 bytes). Separate initially empty targets were under `C:\Samandarin\builds\dirsync-modeless-sync-20261019`. Each confirmation contained exactly three new copies to the right, zero overwrites/deletions/directories and recovery OFF. These cases exercise the real **Execute visible plan** worker without modifying existing data.

| Case | Live action and result | Independent filesystem check |
| --- | --- | --- |
| Complete (`success-right`) | Pause during Copy at 80%; counts then remained stable across several UI actions. Set 4 MB/s while paused and Resume. Navigate the host's right panel into `C:\Program Files` during actual copying; return to the same fixed plugin paths. Progress showed 3.55 MB/s with the 4 MB/s limit. Pause again at 94%, change to 8 MB/s and Resume; a later short sample showed 8.88 MB/s with the 8 MB/s limit. Verification followed without throttling; final summary 3/3, zero errors/unstarted. | SHA-256 of all three targets matched the sources. |
| Stop (`stop-right`) | The next batch started unlimited, confirming the limit is per batch. Pause during Copy at 75%, then use the preview's synchronization Stop button. The progress window closed; result summary showed 1/3 completed, one cancelled, one unstarted. The preview remained usable. | Only `0-done.txt` remained, with matching SHA-256. No large target, next file or staging entry, including hidden entries. |
| Close (`close-right`) | Click the preview's Close button during active Copy (observed at 27% before input). After cancellation/join, both preview and progress disappeared. The host remained open and responsive. | Same exact one-file result and hash as Stop; no unpublished staging. |

The displayed speed is a short sampled measurement, not a strict instantaneous ceiling; cooperative callback pacing can produce samples above/below the configured average. This session demonstrates actual limited copying and a limit change, rather than merely opening the speed dialog. It does not benchmark sustained throughput or shared-disk fairness.

Large targets were checked before cleanup. To retain sufficient disk space, only the exact verified synthetic `a-large.bin` destinations from the earlier native-controls run and the successful run were removed through filesystem tools; sources, small completed files and verification evidence were retained. Local evidence is `success-verification.json`, `stop-verification.json` and `close-verification.json` under the fixture root. The large source hash is `9820ca93c478db6b8b4a322f6f601f0222523689af268a6b396bcc1ac163d122`.

### Time-estimate correction

The limited Copy phase exposed a misleading estimate: weighted work includes two verification reads, but they are not subject to the copy limit. Extrapolating a 4 MB/s copy sample over those reads produced an approximately 18-minute estimate shortly before actual copying and verification completed. The UI now suppresses this estimate while a limit is active and planned copying remains; bytes, rate, bars and phase remain visible. Once planned copying is complete, remaining work may be estimated again. Ordinary unlimited transfers retain their estimate. No transfer, recovery or cancellation rules changed.

The final Release SPL remains 713,216 bytes, SHA-256 `0292ee25599b0899dbf72ebeb787e36db505c9e48b8b96f1bc3e6b181d899417`. Production dialog probes cover this policy in both languages, including restoring the estimate after planned copying. All five x64 suites passed in Debug and Release (2,216 engine; 4,460 copy/recovery/export; 2,369 lifecycle; 5,075 English and 5,195 Czech resource/layout checks). Debug Win32/x64/ARM64 and Release x64 builds passed; ARM64 remains compile-only. Sandboxed linker attempts failed with PDB-service errors/timeouts; identical scripts passed outside the sandbox without project/SDK changes.

A final live smoke run loaded that corrected SPL after restarting only the private host. In the new `eta-right` target, paused Copy at 83%, enabled the default 10 MB/s limit, resumed and observed subsequent verification. The final summary again showed 3/3 with zero errors/unstarted; all target SHA-256 values matched, recorded in `eta-verification.json`. The verified large destination was then removed for space. The exact estimate-policy assertions run against the production callbacks in both languages; this smoke run did not collect a sustained rate sample before verification.

`tools/package.ps1` verified all 14 extracted payload hashes in `out/package-030-modeless-transfer/dirsync-0.3.0-x64.zip`, SHA-256 `f455927671b20319583b7e5217a672c555cc8df43695345cb509347d95ddfb31`. This is a local development package, not a published release.

The live scope remains local NTFS, a three-file new-copy plan, Czech/dark mode and one system DPI. This follow-up does not repeat mixed overwrite/deletion recovery, forced unload inside nested modal callbacks, host shutdown during transfer, multi-monitor DPI, simultaneous core transfer queues or SMB. Their prior automated/historical coverage must not be read as new live evidence here.
