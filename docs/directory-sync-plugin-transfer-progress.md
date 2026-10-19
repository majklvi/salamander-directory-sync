# Transfer-progress window and stored-originals menu — 0.2.2 development

Date: 2026-10-19. Local NTFS x64 verification; no SMB tests or SDK changes.

## Behavior

Confirmed new copies, recovery saves, overwrites, asymmetric deletions, immediate deletion/recycling and combined synchronization open a compact owned native Unicode window. Its 355-DU width, MS Shell Dlg 8 font, source/target rows, two progress bars and Stop button follow the host's copy-dialog arrangement. The existing host appearance helper supplies light/dark colors. This is a plugin window: the current SDK does not expose the full host transfer window with queue, background transfers, pause and speed limits.

The worker publishes source, final destination, operation kind and byte counters under a mutex. The UI reads a snapshot every 150 ms. The current-file bar switches from copying to content verification; verification includes all data streams. The aggregate verification-read count includes reads from both files. The total bar counts completed operations, including recovery saves, rather than estimating remaining time from bytes. A directory or zero-size transfer has no byte percentage and uses an indeterminate file bar. Tooltip callbacks retain complete long Unicode paths; static path controls receive mouse notifications.

The window opens after the operation confirmation, remains owned by the comparison window and closes before the existing result summary. Stop, Escape, Enter on Stop, and the title-bar close button request the existing cancellation path. Closing this progress window does not hide a running worker or discard completed changes. Stop becomes disabled while cancellation finishes. Completion, main-window Close, Preview destruction and forced plugin release destroy the owned window after the worker joins. Scanning, comparison and validation continue to use their existing main-window controls.

The lower Stored originals context menu now explicitly labels clipboard copying and Explorer opening in English and Czech. Ctrl+C/Ctrl+Shift+C remain unchanged. Opening is a request to Windows Explorer, not restoration or launching the original file; nonexistent/inaccessible directories and paths unsupported by Explorer may fail.

## Verification

Automated native-window tests instantiate the shipping English and Czech dialog resources and production callbacks. They check ownership/visibility, complete paths longer than MAX_PATH, path-tooltip text and mouse-notification styles, copy and verification positions, directory indeterminate progress, recovery-inclusive total counts, close-to-cancel behavior, Escape on Stop, disabled Stop after cancellation, removal after completion and joining a worker before destroying the window. Near-maximum 64-bit byte counters must not show 100% prematurely. Executor tests also check real copy/verification telemetry and the actual saved-original source/destination.

All five x64 suites pass in Debug and Release: engine 2,216; copy/recovery/catalog/export 4,277; lifecycle 2,368; English resource/layout 4,873; Czech resource/layout 4,965. Resource layout and caption checks cover 96, 120, 144, 192 and 240 DPI. The plugin matrix passes Debug Win32/x64/ARM64 and Release x64. ARM64 was compiled, not run.

The initial checks above were automated native-window and executor checks. The subsequent live-host session below adds interactive light/dark and Stop/Close coverage. Cloud CI is checked after publication of the source changes; a development package is not a published release.

Local development package: `out/package-0.2.2-dev-20261019-progress/dirsync-0.2.2-x64.zip`, SHA-256 `f96db484a05b9fb7acf46172b8c2276a70105deceefb9fb55b117edced941850`. Packaging verified x64/version metadata, all 14 extracted payload hashes and the guide links. It includes the earlier stored-originals browser and report export as well as this progress window. The package is not a published release.

An initial test exposed a null host-interface access in the isolated dialog harness; theme/centering calls are now guarded when no host interface is installed. A sandboxed build also failed to load the Visual Studio PDB tool correctly; the same repository build scripts passed with normal compiler access. Neither issue was ignored as a passing test.

## Live GUI verification

Computer Use loaded the matching production Release SPL in a private Samandarin 0.16 x64 developer host under `C:\Samandarin\builds\dirsync-progress-gui-20261019\host-compact`. SPL SHA-256: `f0d944eb8de19b5fa2c5241fa16944b88fb2602b32ba43a2a67e4f402eb67486`. No artificial worker delays or test-hook plugin were used. The host's Colors configuration switched from Windows dark mode to its standard Open Salamander scheme; the plugin followed the host without a separate palette switch. The installed user instance was outside the test scope.

Each final case compared three source files against its own empty NTFS target: `0-done.txt` (13 bytes), `a-large.bin` (1,879,048,192 bytes), and `z-next.txt` (13 bytes). The large source is a hardlink to an existing synthetic fixture; destination copies use the ordinary copying adapter. All result filters were enabled. Combined execution confirmed exactly three new copies, zero overwrites and zero deletions. Recovery was OFF and therefore no original needed saving.

| Case | Progress observed before input | Action | Result |
| --- | --- | --- | --- |
| Dark Stop | Large file 6%, 124,780,557 copied bytes; total 1/3 | Click Stop in progress window | 1 completed, 1 cancelled, 1 unstarted |
| Dark Close | Large file 20%, 394,264,589 copied bytes; total 1/3 | Click progress title-bar X | 1 completed, 1 cancelled, 1 unstarted |
| Light Stop (repeat) | Large file 4%, 76,546,061 copied bytes; total 1/3 | Click Stop in progress window | 1 completed, 1 cancelled, 1 unstarted |
| Light Close | Large file 11%, 208,666,637 copied bytes; total 1/3 | Click progress title-bar X | 1 completed, 1 cancelled, 1 unstarted |

The current source/target and phase were readable, percentages and bars updated, and verification reads showed 26 bytes from the completed first file. The window used dark controls in dark mode and standard light controls/green bars in light mode. It was centered on the host and appeared above the comparison window. Each cancellation removed the progress window before showing the existing combined result. Dismissing the result returned to an idle preview requiring a new comparison. Closing the progress window cancels the batch; it does not close the main preview. No functional defect was demonstrated in these four cases and no additional production fix was needed.

Independent filesystem verification found exactly the correct `0-done.txt` in each final target, with no large target, following target or staging residue. All 1,792 one-MiB blocks of the large source retained the preparation SHA-256 `db6fc560c464a84cc492563c0d0095349895fc753360db571aa1bc677a1fc5fb`. An exclusive source open succeeded after the tests. Local evidence is in `verification.json`; these fixtures are outside Git. Live cancellation inside Verify, during recovery/overwrite/deletion, different DPI settings and the main preview's Close during a transfer were not repeated in this session; existing automated tests cover their relevant callbacks/lifecycle paths.

### Preliminary cases and environment limits

An initial English/light host compared and copied 1,200 new 512-KiB files. The eventual result showed 1,200/1,200 completed, zero errors and zero unstarted items; every destination hash matched the synthetic source block. Large-list Computer Use operations suffered delayed click handling, null accessibility and `computer-use request timed out: activate_window`; recovery succeeded but did not provide reliable active-progress coverage. This matches the already documented large-list UI Automation limitation. The small-row cases above provide the actual cancellation evidence. The bulk fixture was removed only after completion and independent verification to release disk space; its counts/hash remain in `large-list-completed.json`.

The compact test host exited normally after the four cases. The bulk host again delayed ordinary close inputs after its result summary; only that verified-idle private process was terminated after checking its exact executable path. This cleanup is not evidence of graceful close during a transfer.

The first small light attempt encountered Windows error 112 because free space was just below the large source's size. It stopped after the first file and left no large target, following file or staging residue. It is an actual disk-space error observation, not a successful Stop test. After freeing only the verified synthetic bulk fixture, Light Stop was repeated in `compact-light-stop-r2` with adequate free space.

Launching an unqualified executable path through the app launcher selected the installed host instead; using the exact `process:` path launched the intended private host. A rewritten disposable REGEDIT4 configuration with a UTF-8 BOM was rejected (parser error 2); the final host uses the unmodified known-good configuration seed. The private developer host's known missing `salmon.exe` warning was dismissed. These setup events are separate from plugin behavior.
