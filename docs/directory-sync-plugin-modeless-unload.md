# Modeless synchronization: host shutdown and forced unload

2026-10-19, 0.3.0 development. Existing pinned SDK; local NTFS only, no SMB tests. This follows the [modeless transfer tests](directory-sync-plugin-modeless.md) and the historical [copy/unload report](directory-sync-plugin-copy-unload.md).

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

The first live attempt is **not completed**. Computer Use repeatedly returned `foreground window did not report a process id` for the startup notice and main host window. The user manually acknowledged the notice and confirmed that the panels were visible. After resetting the Computer Use JavaScript session, input instead returned `GetCursorPos failed: Access is denied (0x80070005)` with a black screenshot. No transfer was started, and no shutdown or force-unload result is claimed from this attempt. A request to make the desktop available remains pending. The private host (observed PID 5144) is responsive and was left open for continuation; ordinary installed windows were not closed. These automation errors have not been attributed to a plugin defect.

The four deterministic native cases above are complete and reproducible through `tools/build.ps1 -Mode Tests` and `tools/test.ps1` in either configuration. They do not establish the still-pending real-host close prompt, plugin-manager force-unload prompt, host process exit or production-module reload behavior. Critical Windows shutdown/logoff, hard kill and forced unload reentrant inside plugin modal callbacks remain outside this verification scope.
