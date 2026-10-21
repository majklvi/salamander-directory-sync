# Transfer-progress window and stored-originals menu — 0.2.2 development

Date: 2026-10-19. Local NTFS x64 verification; no SMB tests or SDK changes.

## Behavior

Confirmed new copies, recovery saves, overwrites, asymmetric deletions, immediate deletion/recycling and combined synchronization open a compact owned native Unicode window. Its 355-DU width, MS Shell Dlg 8 font, source/target rows, two progress bars and Stop button follow the host's copy-dialog arrangement. The existing host appearance helper supplies light/dark colors. This is a plugin window: the current SDK does not expose the full host transfer window with queue, background transfers, pause and speed limits.

The worker publishes source, final destination, operation kind and byte counters under a mutex. The UI reads a snapshot every 150 ms. The current-file bar switches from copying to content verification; verification includes all data streams. The aggregate verification-read count includes reads from both files. The total bar counts completed operations, including recovery saves, rather than estimating remaining time from bytes. A directory or zero-size transfer has no byte percentage and uses an indeterminate file bar. Tooltip callbacks retain complete long Unicode paths; static path controls receive mouse notifications.

The window opens after the operation confirmation, remains owned by the comparison window and closes before the existing result summary. Stop, Escape, Enter on Stop, and the title-bar close button request the existing cancellation path. Closing this progress window does not hide a running worker or discard completed changes. Stop becomes disabled while cancellation finishes. Completion, main-window Close, Preview destruction and forced plugin release destroy the owned window after the worker joins. Scanning, comparison and validation continue to use their existing main-window controls.

The lower Stored originals context menu now explicitly labels clipboard copying and Explorer opening in English and Czech. Ctrl+C/Ctrl+Shift+C remain unchanged. Opening is a request to Windows Explorer, not restoration or launching the original file; nonexistent/inaccessible directories and paths unsupported by Explorer may fail.

## Verification

Automated native-window tests instantiate the shipping English and Czech dialog resources and production callbacks. They check ownership/visibility, complete paths longer than MAX_PATH, path-tooltip text and mouse-notification styles, copy and verification positions, directory indeterminate progress, recovery-inclusive total counts, close-to-cancel behavior, Escape on Stop, disabled Stop after cancellation, removal after completion and joining a worker before destroying the window. Near-maximum 64-bit byte counters must not show 100% prematurely. Executor tests also check real copy/verification telemetry and the actual saved-original source/destination.

Before publication, all five x64 suites passed in Debug and Release: engine 2,216; copy/recovery/catalog/export 4,277; lifecycle 2,368; English resource/layout 4,873; Czech resource/layout 4,965. Resource layout and caption checks cover 96, 120, 144, 192 and 240 DPI. The plugin matrix passed Debug Win32/x64/ARM64 and Release x64. ARM64 was compiled, not run. The subsequent CI correction below adds a catalog regression assertion.

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

## CI correction: short recovery-root paths

The first published source commit, `9f546a8`, passed all four cloud builds, but Debug and Release copy tests failed at reading a genuine recovery manifest. Windows runner TEMP uses a short 8.3 path spelling while the recovery writer records a handle-normalized full run directory. The catalog enumerated the short spelling and incorrectly rejected its mismatch with the manifest. A new local regression calling the reader with `GetShortPathNameW` reproduced the failure before the fix.

The reader now resolves the existing recovery-root handle to the same normalized path representation as the writer before enumerating runs. Ordinary reparse-parent checks remain in place, and the handle itself rejects a reparse root. The correction does not change file executors or the progress-dialog code tested above. The added regression verifies the complete run and recorded overwrite result through an 8.3 alias; existing junction, unavailable/cancelled, damaged-record and long Unicode cases remain required. Updated local copy-suite count is 4,278 when 8.3 aliases are available.

After the correction, all five local x64 suites pass again in Debug and Release (2,216 / 4,278 / 2,368 / 4,873 / 4,965 checks), as does the full plugin build matrix. Cloud validation must use the corrective commit's head, rather than treating the initial failed workflow as successful.

## CI correction: localized value substitution

CodeQL reported four new non-constant-format findings in the new browser/progress text. These labels now insert named values into resource templates instead of treating translations as printf formats. Percent directives remain literal and inserted values are never parsed again. Native EN/CS dialog regression checks cover the counts, byte totals and elapsed label; all five suites and the complete build matrix passed again with unchanged check totals.

A final production Release smoke test used SPL SHA-256 `acb330e20c6a74a167b0a24d28115e02890a3e34dafb64413372596525686a2c` in the same private light/Czech host. The `compact-light-final` target again planned only three new copies. The live large-file display showed 28%, 534,773,773 copied bytes, 26 verification-read bytes and 1/3 completed steps, with correctly substituted Czech text. Stop was accepted during the large copy (last observed 59%, elapsed 0:00:04). The result was 1 completed, 1 cancelled, 1 unstarted; independent enumeration found only the 13-byte `0-done.txt`, without staging residue. No overwrite or deletion was planned. This additional smoke test checks the formatting correction; the four light/dark Stop/Close cases above remain the broader live coverage.

The earlier four CodeQL findings predate this feature, including one in the pinned SDK; they are separate from the four new findings corrected here. No SDK files were modified.

> Terminology update (2026-10-21): Cancel captions in this report are normalized to the plugin's current Czech **Zrušit**. This does not change the dated test results or assert a new live GUI test. The host's original Czech wording differs.

## Native host-dialog alignment (2026-10-19)

The later UI revision follows the actual single-stream copying dialog, rather than the initial 355 × 154 Windows-progress layout documented above. Read-only source inspection used Samandarin integration checkout `9d58fb8f2200fec5048e2efe6f521258c69caf0e`:

- `src/lang/lang.rc`, `IDD_PROGRESSDLG` (line 517): 355 × 110 dialog units, MS Shell Dlg 8. Source/target at (45,11)/(45,23), 297 × 12 bars at (45,40)/(45,55), one status line at (46,69), footer buttons at y=87.
- `src/dialogs.cpp`, `CProgressDialog::DialogProc` and `IDT_UPDATESTATUS`: the core attaches `CProgressBar`, path-aware static controls, updates percentages/title and displays transfer status. Its worker/script owns pause, scheduling, speed-limit and background/minimization behavior. Scan-ahead and multiple streams have additional layouts; these are not the plugin's executor model.
- `src/gui.cpp`, `CProgressBar::Paint`: host-configured progress colors, flat frame, percentage centered inside the bar with contrasting text on filled/unfilled regions; indeterminate mode uses a moving block.
- Pinned `spl_gui.h`, `AttachProgressBar` / `CGUIProgressBarAbstract`: the existing SDK exposes that same bar implementation and its lifetime lasts until the attached control's destruction. It does not expose `CProgressDialog`, `COperations` or a way to run the plugin's verified executor inside the full core copying dialog. No SDK extension, binary-resource scraping, private ABI or copied core worker is used.

Computer Use observed an actual F5 copy of the isolated synthetic `a-large.bin` (1,879,048,192 bytes) in the private 0.16 x64 host. The live window showed the two native blue bars with centered 3% text, path lines, compact footer (Minimize / Pause / Cancel) and percentage title. This copy completed before the subsequent Pause input could take effect; this session therefore does not claim live Pause behavior. Source and target SHA-256 matched (`9820ca93c478db6b8b4a322f6f601f0222523689af268a6b396bcc1ac163d122`). Only that verified synthetic destination was removed through the filesystem to recover test space. Local evidence: `C:\Samandarin\builds\dirsync-native-progress-20261019\native-copy.json`.

The plugin now uses the same 355 × 110 resource grid and attaches actual host progress bars through `gui->AttachProgressBar`, passing only numeric progress and no translated ANSI text. Long Unicode paths remain wide Win32 labels with full tooltips. Czech bar labels use the full existing left-label column to avoid clipping. Percentages move inside the bars; the title carries total progress. One status line replaces the separate phase/count/bytes/time rows; its tooltip preserves all the detailed values. Cancel/Zrušit is centered at the native footer position. Stop, Escape and title-bar Close retain the existing worker cancellation/join contract. A standard-progress fallback is used only if the host cannot supply a bar or in the standalone test harness. Borrowed SDK interfaces are cleared when the controls are destroyed, never deleted by the plugin.

The adaptation deliberately keeps truthful plugin semantics: Total counts completed steps (including recovery saves), not byte-weighted progress. File progress switches to verification reads during Verify. The plugin does not add decorative Pause/Minimize buttons, host queue, speed limit, or background transfer without the corresponding executor/API support; those previously deferred capabilities remain deferred. The full core dialog is not claimed to have been reused.

### Live revised-plugin checks

Production Release SPL SHA-256: `82971682af97cde350a1570ffaae8339ce1ca295f90624d1952541b215ad5eb9`. Computer Use loaded it in the same private host and confirmed three new copies, zero overwrites/deletions, recovery OFF, into separate empty NTFS targets under `dirsync-native-progress-20261019`.

| Case | Observed active transfer | Input | Result |
| --- | --- | --- | --- |
| Light `plugin-light` | Large file 28%, native blue bar, total 33% / 1 of 3 | Zrušit | 1 completed, 1 cancelled, 1 unstarted |
| Dark `plugin-dark` | Large file 25%, native blue bar on dark host colors, total 33% / 1 of 3 | Progress title-bar X | 1 completed, 1 cancelled, 1 unstarted |

Both screenshots match the native host's bar geometry and internal percentage rendering. Independent enumeration found only the correct 13-byte `0-done.txt` in each target, no large/following file or staging residue. The progress window disappeared before the result summary; dismissing the summary returned to idle preview. Host dark mode was selected through its Colors configuration, not through a plugin palette. The private preview and host were then closed normally; installed user instances were untouched.

All five x64 suites pass in Debug and Release: engine 2,216; copy/recovery/catalog/export 4,278; lifecycle 2,368; English resources/layout 4,789; Czech resources/layout 4,877. Resource geometry is checked at 96/120/144/192/240 DPI, including the host coordinates and borderless static placeholders needed by AttachProgressBar. Runtime probes cover native SDK progress value dispatch (including unknown and capped progress), fallback bars, long Unicode paths, status detail tooltips, cancellation and lifetime teardown. The first Czech metrics check found clipped Soubor/Celkem captions; widening those labels within the original left column resolved it. The plugin build matrix remains Debug Win32/x64/ARM64 and Release x64. Live high-DPI, shell deletion, SMB and core queue/Pause tests were not added to this revision.

## Native controls and transfer accounting — further revision (2026-10-19)

This revision supersedes the preceding revision's step-count total and deferred plugin Pause/Minimize/speed controls. It keeps the existing SDK and the plugin's own verified executor. Further read-only inspection of the same host commit established:

- `src/dialogs.cpp:1184` attaches the native `BTF_DROPDOWN` button; lines 1910–2050 build the Pause/Resume and Set Speed Limit menu. Queue waiting is conditional on actual queue membership. Lines 2329 and 2375 handle cooperative pause and minimization. The SDK already exposes `AttachButton` and `WM_USER_BUTTONDROPDOWN`, so the plugin now uses the real split-button implementation.
- `src/plugins3.cpp:81` derives the SDK progress control from the same `CProgressBar` used by the core. `src/gui.cpp:215–348` draws the border, host-configured colors and centered integer percentage. Both plugin bars continue using this renderer, rather than approximating it with a Windows progress theme.
- The core's status formatting uses `PrintDiskSize` mode 4, available through the SDK, independently of Panels → Show sizes. It supplies compact units, at least three significant digits and the regional decimal separator. `PrintTimeLeft` supplies the estimate's localized time units. The core updates status on a 500 ms interval and uses byte/work weighting with a 4 KiB small-item cost (`src/worker.h:17`).

The footer now follows the native Minimize / Pause-or-Resume / Cancel grid at x=92/152/212, y=87. The Czech Minimize button is four dialog units wider within the existing gap: the first caption-metrics check found clipping at 192 DPI with the English width. Pause's native arrow menu offers Pause/Resume and Set speed limit. The compact speed dialog follows the core's `IDD_SETSPEEDLIM` arrangement: enable checkbox, integer value, KB/s or MB/s, OK/Cancel. The supported range is 1 KB/s through 1 TB/s. Zero internally means unlimited. Cancel preserves the previous limit; each new batch starts unlimited.

Pause is cooperative, including copy callbacks, verification reads and safe pre-operation boundaries; it does not suspend a thread holding locks. Cancel releases a paused or rate-limited worker without requiring Resume. No new pause point is introduced between a filesystem change and its recorded result. On Resume, final source/target/staging checks are repeated before publication. Regression cases replace the staged file's name or add a child to a staged empty directory during Pause: neither is published, and unrelated data is preserved and reported. The speed limit accounts cumulatively for copied data, ADS and recovery copies, responds to live changes, and excludes verification reads. Bounded timer oversleep credit prevents small copy blocks from unnecessarily reducing higher limits.

The status line now shows copied/total data, transfer speed and remaining time when measurable, with a separate verification indication. It uses the host's size/time formatters; the tooltip retains phase, step counts, copied data, verification-read data and elapsed time. Total progress weights copying plus the two verification reads, with a 4 KiB minimum per step. Metadata preparation retains the copying-complete one-third position rather than jumping to completion before Verify. ADS are not enumerated in advance, so the estimate can change and total remains below 100% until all planned steps finish. Percentage and rate calculations handle 64-bit counters without integer multiplication overflow.

Minimize affects the progress window, which has its own taskbar entry and icon for restoration; it reduces the worker's priority until restoration. The host's private whole-application minimization helper is not exposed by the SDK. The preview remains locked: this is not host queue integration or background use of the main panels. Cancel/Escape/X retain the plugin's established direct safe-stop behavior, whereas the core can ask an additional cancellation question. No private core dialog/worker ABI is used. Completion explicitly ends an open speed-limit dialog before destroying its owner and rechecks batch state after nested menu/dialog dispatch.

### Live controls and data-format verification

Computer Use loaded production Release SPL SHA-256 `89364031725e153de02a25f09b892e351760cc989c5964fc13dd5fc5f51b14cc` into the private Czech/dark 0.16 x64 host. The isolated NTFS target `C:\Samandarin\builds\dirsync-native-controls-20261019\right` planned only three new files, zero overwrites/deletions, recovery OFF. The source was the previously verified three-file compact fixture, including `a-large.bin` (1,879,048,192 bytes).

- During copying, the file bar showed 14% and the work-weighted total 4%. The status read `Zkopírováno 258 MB z 1,75 GB, rychlost: 472 MB/s, zbývá: 11 sek`, confirming host units and a decimal comma.
- Pause was acknowledged during Verify: the file bar stayed at 29%, total at 53%, copied bytes at 1,879,048,205 and verification reads at 1,118,830,618 across subsequent observations. The caption and button changed to paused / Continue.
- The native arrow menu opened; enabling 10 MB/s in the speed dialog and accepting it retained the pause. This tested the GUI setting, not physical copy throughput: copying had already finished before the limit was entered. Automated worker tests below cover actual throttling.
- Minimize removed the progress window from the preview while keeping it available for activation; restoring it retained the paused counters. Tab moved focus from Minimize to Continue; Enter resumed verification and the bars advanced. The complete result was 3/3, zero errors and zero unstarted items.

Independent enumeration found exactly those three target files with matching SHA-256 values and no staging residue. The large file hash remained `9820ca93c478db6b8b4a322f6f601f0222523689af268a6b396bcc1ac163d122`. Evidence is outside Git in `C:\Samandarin\builds\dirsync-native-controls-20261019\verification.json`. The preview and this private host exited normally; installed user instances were preserved. This live binary preceded the final post-pause staging and nested-speed-dialog fixes; those changes are covered by the final automated tests. No empty directory, overwrite or deletion was planned in this live case. Earlier light/dark cancellation checks remain recorded above; live high-DPI and live throttled-copy throughput were not repeated here.

### Final regression coverage

The worker tests cover Pause/Resume and cancellation during Copy/Verify/Publishing, changes during a publication pause, ADS and multi-file rate accounting, rate changes/cancellation during a wait, per-batch reset, and recovery-journal completion before acknowledging a pause. Production dialog probes cover the three-button keyboard order, Enter/Escape, minimize/restore, the speed parser's invalid/overflow limits, monotonic preparation/verification weighting and batch completion while the modal speed dialog is open. Resource metrics cover EN/CS at 96/120/144/192/240 DPI. The first sandboxed Debug linker attempt failed with `LNK1101`/`MSPDB140.DLL`; the same repository build scripts completed outside the sandbox, without changing project flags or SDK files.

Final local results for this revision: all five x64 suites pass in both Debug and Release — engine 2,216; copy/recovery/catalog/export 4,460; lifecycle 2,368; English resources/layout 5,075; Czech resources/layout 5,195. The plugin matrix passes Debug Win32/x64/ARM64 and Release x64, with warnings treated as errors. ARM64 was compiled, not run. Final Release SPL SHA-256 is `edb7c4032f845bb3a2b8a79c335914586d41a5dc1ec70bcbe3ae152b85a5f112`. The final source diff does not change vendor/SDK files. These are local checks; this uncommitted revision has not run on GitHub CI. No SMB tests were performed.

## Modeless transfer follow-up — 2026-10-19

The [modeless transfer verification](directory-sync-plugin-modeless.md#live-modeless-transfer-verification) adds live Copy-phase Pause/Resume, actual 4/8 MB/s limiting, host-panel use during copying, Stop from a pause and preview Close during Copy. It also records the corrected limited-copy time estimate, final binary/package hashes and the precise remaining limits of coverage. Earlier dated reports above retain the state tested at the time.

## Speed-limit dialog alignment — 2026-10-19

The [speed-dialog report](directory-sync-plugin-speed-dialog.md) compares the actual core source and live dialog with the plugin, records the unavailable public SDK entry point, and documents the new native two-radio layout, four units and localized Help. It supersedes the earlier one-checkbox/two-unit description above without changing the transfer worker.
