# Modeless synchronization: host shutdown and forced unload

2026-10-19, 0.3.0 development. Existing pinned SDK; local NTFS only, no SMB tests. This follows the [modeless transfer tests](directory-sync-plugin-modeless.md) and the historical [copy/unload report](directory-sync-plugin-copy-unload.md).

Current outcome: native modeless Copy/Verify regressions and live host shutdown/Plugin Manager forced unload are complete. The live cases paused the worker during Verify; the production SPL/SLG were actually unmapped and reloaded in the surviving host. No production defect was found. The initial automation failure is retained below as dated attempt evidence, superseded by the completion section.

## Contract and host integration

The modeless preview returns from the plugin menu callback, so the host can enter its own close/unload path while synchronization continues. The plugin refuses ordinary `Release(FALSE)` while the preview is open. If the host subsequently requests `Release(TRUE)`, it requests cancellation, joins the worker, closes transfer UI and destroys the preview. `WM_DESTROY` also cancels/joins; `WM_NCDESTROY` clears the active pointer and removes the thread keyboard hook.

Read-only inspection of the host source confirms that `CPluginData::Unload` first calls ordinary Release and offers its own force-unload confirmation after refusal. It then calls forced Release before `FreeLibrary`. Main-window close uses `Plugins.UnloadAll`; declining a required unload cancels host close. This is not termination of the process by Task Manager. No host source, SDK interface or shutdown policy was changed.

If forced release reenters an existing plugin callback through a nested modal message loop, the existing module-pin safeguard is a separate exceptional path. The ordinary forced-unload cases below must not be treated as coverage of that reentrant path or as evidence that a pinned module is unmapped.

## Deterministic native regression

Extended the existing test-only copy lifecycle probe with a modeless mode. It retains the actual `CopyNewFiles` worker, production `Plugin::Release`, preview destruction handlers and `PreviewKeyboard` hook. The preview and progress containers are minimal native windows; this is not a full production-resource/host GUI test. Only destruction messages use the production preview callback, avoiding unrelated initialization of nonexistent fixture controls.

Four additional cases interrupt a real transfer callback after nonzero data movement:

| Boundary | Previously completed files | Result |
| --- | ---: | --- |
| Copy | 0 | Cancel current item; leave the next unstarted. |
| Verify | 0 | Same; no unpublished target survives. |
| Copy | 1, including ADS | Preserve completed main data and ADS; cancel current item and leave next unstarted. |
| Verify | 1, including ADS | Same preservation and cancellation result. |

Each case checks that ordinary Release refuses and leaves the worker active. Forced Release must synchronously join it, clear both window pointers and the active pointer, destroy preview/progress windows and remove the keyboard hook. A repeated forced Release is safe. The caller immediately runs `FreeLibrary` and checks that the test DLL is absent from the module map. Filesystem checks reject leftover staging/unpublished targets, independently read completed main data and ADS byte-for-byte, and exclusively open fixture files to detect surviving handles.

The existing callback fixture waits at most five seconds for the test thread to reach its boundary; cancellation releases it immediately. The existing three-second local Release bound passed in each case. Neither bound promises a maximum shutdown time for arbitrary devices or stalled I/O.

All five x64 suites passed in Debug and Release: engine 2,216; copy/recovery/export 4,460; lifecycle **2,491** (previously 2,369); English resources/layout 5,151; Czech resources/layout 5,279. Both complete test builds used the repository scripts and warnings as errors. No production code, language resource, binary version or SDK changed.

## Live host verification

An isolated portable Czech/dark Samandarin 0.16 x64 was prepared at `C:\Samandarin\builds\dirsync-modeless-unload-20261019\host`. Its production SPL is 713,728 bytes, SHA-256 `3c1af1c1aa4732c63fb0740bb6e155f2dc106dde0bbbb752a583e94b1582ec82`. The private fixture has `0-done.txt` with ADS `:note`, a hardlink to the existing synthetic 1,879,048,192-byte `a-large.bin`, and `z-next.txt`, with initially empty, separate shutdown/unload targets. All fixtures and configuration are outside Git; installed user instances are preserved.

Live outcome is recorded below separately from the native regression; preparation alone is not a passed host shutdown/unload test.

### Initial attempt

At the first commit (`e361821`), live verification was **not completed**. Computer Use repeatedly returned `foreground window did not report a process id` for the startup notice and main host window. The user manually acknowledged the notice and confirmed that the panels were visible. After resetting the Computer Use JavaScript session, input instead returned `GetCursorPos failed: Access is denied (0x80070005)` with a black screenshot. No transfer was started, and no shutdown or force-unload result is claimed from that attempt. The request to make the desktop available was then pending. The private host (observed PID 5144) was responsive and was left open for continuation; ordinary installed windows were not closed. These automation errors were not attributed to a plugin defect.

The four deterministic native cases above were complete and reproducible through `tools/build.ps1 -Mode Tests` and `tools/test.ps1` in either configuration. They alone did not establish the real-host close prompt, plugin-manager force-unload prompt, host process exit or production-module reload behavior. Those live checks were completed in the continuation below.

### Completion after the desktop became available

The user confirmed that the desktop was ready. Resetting the Computer Use JavaScript session again restored screenshots and input. The remaining UI steps were performed through Computer Use, without a helper DLL, private host commands, process kill or production-code changes. A later `coordinate input geometry is unavailable` on Compare was resolved by activating the preview, observing a fresh screenshot and clicking the actual button once; this was not counted as a successful comparison until the new rows appeared.

Each synchronization confirmation contained exactly three new file copies to its separate initially empty target, zero overwrites/deletions/directories and Keep originals OFF: `0-done.txt` (8 bytes plus 12-byte `:note` ADS), `a-large.bin` (1,879,048,192 bytes), and `z-next.txt` (8 bytes). No existing target data was replaced or removed. The large source was an existing synthetic fixture hardlink, not user data.

| Case | Actual GUI actions and result | Independent evidence |
| --- | --- | --- |
| Host close, PID 5144, `shutdown-right-ž` | Started **Execute visible plan**, then Pause. By the post-Pause observation the worker was in Verify, with one item completed, 71% overall and 57% of the current file's verification. Switched to the host main window and used Alt+F4. The host displayed its own force-unload question. **No** cancelled host close: the main window was enabled again and the preview/progress remained paused with unchanged counters. A second host Alt+F4 followed by **Yes** cancelled/joined work and closed all private-host windows. | An observer attached to that exact process before close recorded normal exit with **code 0**. Only `0-done.txt` remained, with matching main data and ADS; no large target, later target or staging entry remained. |
| Plugin Manager unload, PID 13480, `unload-right-ž` | Restarted only the private host, started the same three-copy plan into the other empty target and paused it. The post-Pause observation showed Verify at 5% of the current file, 36% overall, one completed item. Opened the host's Plugin Manager while the preview remained open. Chose **Unload**, accepted saving the private plugin configuration, then confirmed **Yes** to the force-unload question. Both plugin windows disappeared; the manager showed **Loaded: No**, loaded count zero and disabled Unload. The host remained responsive. | Process-module snapshots before/after showed `dirsync.spl` and its `czech.slg` first present, then both absent in the same surviving PID. Only the completed first file and correct ADS remained; no staging/large/next target remained. |
| Reload in the same PID 13480 | Closed Plugin Manager and invoked the synchronization command through the Plugins submenu. The preview opened with a fresh idle state; process-module inspection showed SPL/SLG mapped again. Set the fixture roots and ran a new comparison only. It completed with three rows: the retained `0-done.txt` was equal, and large/next files were left-only. Closed the idle preview, then closed the private host normally. | Same PID throughout unload and reload. Module snapshots and the observed new comparison distinguish a fresh load from merely raising a surviving old preview. No second synchronization was started. |

Both targets independently contained exactly the first completed file, including hidden-entry enumeration. Its SHA-256 is `5274abf70efa917f42b77949b35234415740f600b941315d4a55f83500972061`; ADS content is exactly `DONE-ADS-030`. Main and ADS bytes matched their source and could be opened exclusively after cancellation. All three original main-stream SHA-256 values were unchanged; neither source path nor its synthetic hardlink content was modified. Final window enumeration confirmed no remaining private-host window. Installed user instances and unrelated test hosts were preserved.

Outside-Git evidence under the fixture root: `before.json`, `native-tests-Debug.json`, `native-tests-Release.json`, `host-exit.json`, `modules-before-shutdown.json`, `modules-before-unload.json`, `modules-after-unload.json`, `modules-after-reload.json` and `gui-files-verification.json`. The files and small completed targets remain available for inspection.

### Scope and remaining limits

These checks close the ordinary main-window close and Plugin Manager force-unload gaps for the current modeless version, with a real partially completed batch and an actually paused worker. The live Pause inputs were issued after Copy screenshots, but reached Verify before delivery; they are **not** claimed as live force-unload-during-Copy evidence. The new deterministic cases cover precise Copy/Verify boundaries with zero/one prior completion. No speed limit was needed once the live workers were paused.

The GUI scope is Czech/dark, one system DPI, new-copy batches and recovery OFF. Mixed overwrite/deletion/recovery unload remains covered by the prior native suites, not newly repeated live. Critical Windows shutdown/logoff, hard kill, stalled device I/O, SMB and forced unload reentrant inside plugin modal callbacks remain outside this verification scope. The existing module-pin safeguard for that exceptional reentrant case is unchanged. No automatic rollback or recovery guarantee after process termination is implied. Final release packaging/clean installation remains a separate verification step.
