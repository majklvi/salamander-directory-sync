# Profiles — version 0.2.0

Plan and explicit user decisions recorded on 2026-10-18; implementation follows in the standalone plugin, without SDK changes or SMB testing.

Latest verification: [multiple profiles, dark GUI and full host restarts](#multiple-profiles-dark-gui-and-full-restarts--2026-10-18). The dated records below retain the scope and limitations of their individual stages.

## Scope and decisions

- A named profile stores the absolute left/right directory paths, masks, Subfolders, By content, Ignore date, Asymmetric, Empty directories and the five result filters.
- Keep originals and the recovery directory remain **global**, as explicitly requested by the user. Appearance, columns, sorting and relative display also remain global. Host timestamp tolerance continues to come from the host on each Compare.
- The plugin opens with the last used saved profile. If no valid profile is selected, it uses the current host panels as before. Merely opening or selecting a profile never starts Compare or a file operation.
- Panel selection and highlighted rows are transient and are never stored. Neither comparison results nor manually edited file proposals belong to a profile.
- A single native icon button at the right of the directory inputs opens saved profiles, current panel directories and a compact management dialog. Management changes are committed by OK and discarded by Cancel. The active name and unsaved marker appear in the main window title and tooltip. English/Czech resources and host appearance are retained.
- Manual form edits mark the active profile with an asterisk. Save updates it; Save as creates a separate profile. Selecting a different profile or returning to panel directories asks OK/Cancel before discarding modified form values or existing comparison results. Loading changed inputs clears the old comparison so actions cannot use mismatched roots.
- Profiles are managed only while idle. Existing Stop/Close and forced-unload behavior remain in force.

## Persistence

Use the existing SDK plugin-configuration callbacks, with Unicode-safe data stored under a separate versioned value. Profile names are distinct without case sensitivity; stable IDs preserve last selection across rename/delete. Missing profile data upgrades cleanly from 0.1.x. Invalid serialized data must not overwrite unrelated appearance/recovery preferences. Missing/offline directories do not make a profile disappear: normal Compare reports the access error.

## First implementation and validation

1. Implement the profile model, validation and versioned serialization; preserve all existing settings.
2. Add compact native controls and name input, switching/dirty-state behavior and persistence. Set code/resources/package metadata to 0.2.0 together.
3. Verify save/load/rename/delete, duplicate names, Unicode/long paths, startup with last profile, empty/invalid storage, cancellation, result invalidation and global recovery/appearance preservation. Run the five existing Debug/Release x64 suites and relevant builds; inspect native GUI and document limits.
4. Update English/Czech guides. Export/import, groups, scheduled/batch execution and automatic synchronization remain outside 0.2.0.

## Implementation and verification — 2026-10-18

Implemented in the standalone working tree based on `11e25d4c58cf4d2920a9a2b44160aa4493fb6261`. This record describes local changes, not a published release or a cloud CI run.

- `src/profiles.h` owns named profiles, case-insensitive name uniqueness, stable IDs, removal and bounded UTF-16 serialization. `ProfilesV1` is a separate binary value in the existing SDK configuration. Limits: 1,000 profiles, 128 UTF-16 code units per name and 16 MiB total serialized data. Truncated, malformed, duplicate or unknown-version data is rejected without replacing other preferences. A missing selected ID falls back to panel startup.
- Both shipping language resources have one compact profile row. The dialog grows by 20 dialog units; the file list retains its original height. Management offers Save as, Rename, Current panel directories and Delete. Save updates an existing profile; names are entered in a compact standard OK/Cancel dialog. Tooltip text explains scope and the asterisk.
- Profile selection and management are disabled while busy. Switching requires confirmation for dirty saved profiles or existing results, including failed/stale results without a live snapshot. Cancel restores the previous selection. A successful switch clears results, groups and manual proposals before replacing inputs. Returning to panel directories is explicitly available in the menu even after manually editing an unnamed task.
- Recovery and appearance remain global. No profile operation calls a file executor. Deleting the active profile keeps current inputs/results but makes the next startup use panel directories; the current inputs can be saved under another name.
- Version metadata, both SLGs, package checks and CI artifact name now use 0.2.0. English/Czech guides explain profiles. Root README files distinguish the development version from the still-published 0.1.6 download.

### Automated local validation

All five x64 suites pass in **Debug and Release**:

| Suite | Checks per configuration |
|---|---:|
| Engine and profile storage | 2,211 |
| Copy/recovery/overwrite/delete | 3,729 |
| Lifecycle and native controls | 2,367 |
| English compiled resources/layout | 2,737 |
| Czech compiled resources/layout | 2,781 |

Profile tests cover Unicode names and long paths, all option/filter bits, truncation at every byte boundary, unknown versions/bits, trailing data, duplicate names/IDs, removal of active and inactive profiles, last selection, malformed UTF-16 and preservation of existing output on decoding failure. Fake SDK callbacks verify profile persistence separately from shared recovery/color settings and upgrades from missing/invalid profile values. Native-control tests exercise capture/apply, dirty/Save states, clearing old results, exclusion of panel selection, no worker on load and busy locking.

Build matrix passed: Debug Win32/x64/ARM64 and Release x64, including both language modules. Resource geometry and text metrics were checked at 96, 120, 144, 192 and 240 DPI. These resource checks are not live monitor-DPI or ARM64 runtime tests. `git diff --check` passed.

### Live GUI validation

Computer Use tested the installed Samandarin **0.15.1 x64, Czech, light appearance** using only the local NTFS time-tolerance fixture. Save as accepted `Pilot – žluťoučký 漢字 😀`; changing By content marked the profile dirty and enabled Save. Cancel during profile switching retained the profile and modified checkbox. Save removed the asterisk. Explicit Compare produced eight fixture rows. Switching to panel directories required confirmation and cleared all eight rows; keyboard End loaded the saved profile with no comparison. Rename to `Pilot – uložené porovnání 漢字 😀` preserved the task.

The plugin was then closed, fully unloaded with configuration saved, replaced with the final local build and loaded again. Opening it restored the renamed last profile, its test roots and By content; results remained empty and no operation started. Shared row colors, relative-time display and column preferences remained visible. A temporary metadata text-encoding regression found during inspection was corrected; the final About/manager text again displays Michal Várady correctly. No synchronization or file-deletion command was invoked in this GUI session.

### Package and remaining pilot checks

Local packaging verified 14 payload files and extracted hashes. The internal test ZIP is `out/packages/profiles-0.2.0-local-20261018/dirsync-0.2.0-x64.zip`, SHA-256 `e3eb2291580cb775655da793226c848b45f5ad81897502422c997e2eb1d61389`. It was built from the uncommitted working tree; the package's existing source-revision field identifies the base commit. Rebuild after committing before any distribution.

Before release, exercise multiple profiles and duplicate-name correction in the GUI, dark appearance and resizing, profile deletion confirmation/Cancel, and a full host restart in an isolated configuration. Profile removal itself is covered by automated model/storage tests; live deletion of the test profile was not performed. The unload/reload check above is not a full host restart. English localization was checked through compiled resources, not a second live English host. No SMB tests, SDK modifications, public release, push or current-change cloud CI verification were performed.

## Compact profile UI revision — 2026-10-18

At the user's request, the separate profile row described in the initial implementation above has been replaced by one icon-only native button at the far right of the directory row. Both directory edits remain equally wide. The recovered 20 dialog units go to the file list (404 instead of 384 units); the list remains flush with the status bar. On resize, the icon stays anchored to the right edge. Its neutral slider glyph is drawn at the dialog DPI and follows the host's light/dark button text color. The localized accessible name and tooltip remain available.

The quick menu contains panel directories, checked saved profiles and Manage profiles, separated by a horizontal rule. Names preserve Unicode and escape literal ampersands; long menus use native scrolling. The active profile and dirty marker also appear in the main window title and tooltip. The compact 280 × 166 dialog-unit manager shows all profile names, selected paths and Save current/Save as/Rename/Delete. It edits a draft: OK commits it, Cancel discards it. Saving or renaming current settings does not invalidate an existing comparison; loading different settings still requires the existing confirmation and invalidates results.

Debug and Release passed all five x64 suites: engine/profile storage 2,211; copy/recovery 3,729; lifecycle/native controls 2,367; English resources/layout 3,043; Czech resources/layout 3,092. The native-control probe additionally verifies that updating a manager draft leaves global profiles and the main form untouched. Compiled dialog metrics passed at 96–240 DPI. Debug Win32/ARM64 and x64 builds and Release x64 passed. The layout test caught an incorrect static status-bar anchor during development; it was corrected before these final runs.

Computer Use on the installed Czech/light Samandarin 0.15.1 x64 verified the icon placement, active-profile title, checked quick-menu entry and compact manager with selected Unicode paths. Renaming the test profile in the draft and then choosing Cancel left the original name in both the title and reopened menu. No synchronization was started. Live dark-mode, full-host restart and profile-deletion checks remain pending as listed above; resource DPI tests do not claim live monitor scaling.

The updated local package verified 14 payload files and extracted hashes: `out/packages/profiles-compact-0.2.0-local-20261018/dirsync-0.2.0-x64.zip`, SHA-256 `b8b5165e060d6f0ddfe24555f6fa9591a4abd902c4a0418f818af16197539ef5`. This supersedes the earlier local package for current UI testing. It remains an uncommitted-working-tree validation artifact, not a published release.

## Complete selected-profile overview — 2026-10-18

The manager now shows every user-facing stored profile field together: name in the profile list, left/right directories, masks, Subfolders, By content, Ignore date, Asymmetric, Empty directories, and the Equal/Different/Single-sided/Left to right/Right to left result filters. All ten switches show explicit localized On/Off values, including disabled choices. This is the saved profile preview, not a reflection of unsaved main-window values. Shared recovery, appearance and host time-tolerance preferences are not profile fields and are therefore not included.

The width stays at 280 dialog units. Below the shorter profile list, three read-only single-line edits show the two paths and masks, followed by comparison/filter details in two compact columns. Text in the edits can be selected, horizontally scrolled and copied without modifying the profile. All details fit without tabs or a scrolling details pane. Missing selection clears all values to avoid displaying a previous profile's settings.

Debug/Release validation passed all five x64 suites: engine/profile storage 2,211; copy/recovery 3,729; lifecycle 2,367; English resources/layout 3,684; Czech resources/layout 3,770. Native-control tests exercise all 32 patterns of comparison bits and complementary filter patterns, a long Unicode mask, exact displayed paths and clearing after deselection. The test loads the compiled English UI resources as data, without running that executable or using user configuration. Both compiled language layouts and the actual On/Off translations were measured at 96, 120, 144, 192 and 240 DPI. Debug Win32/ARM64 and Release/Debug x64 builds passed.

Computer Use inspected the complete manager on Czech/light Samandarin 0.15.1 x64. It initially exposed clipping from reusing uppercase report strings; dedicated title-case profile translations corrected it, with regression checks on their actual rendered text width. The final build was reloaded and visually checked: all ten Czech state values fit completely, and the manager was left open for review. No synchronization or deletion command was invoked in the GUI; no SDK modification or SMB testing was performed. English/different-DPI checks are compiled-resource tests, not live host or monitor-DPI tests.

The current local test package is `out/packages/profiles-details-final-0.2.0-local-20261018/dirsync-0.2.0-x64.zip`, SHA-256 `eeb741fed3428281b555b3f13e6e3d5fa360054b2f1b93d81deab6ffe4fd87fd`; 14 payload files and extracted hashes verified. It supersedes the earlier local profile packages. Changes remain in the uncommitted working tree; this is not a published release or cloud CI result. English/Czech full and pilot guides were updated.

## Selected profile icon — 2026-10-18

The user selected proposal 3: a saved-profile card with a star. It replaces the slider glyph described above. The 16 × 16 monochrome pixel mask scales with button DPI and uses the host's light/dark button text color; the accessible name, tooltip and menu behavior stay the same. The earlier local package predates this icon change. Light/dark artwork previews were inspected; this change does not claim a live-host reload.

### Final icon build and reload — 2026-10-18

The star-icon build subsequently passed all five x64 suites in Debug and Release (2,211 engine/storage assertions; 3,729 file-operation assertions; 2,367 lifecycle checks; 3,684 English and 3,770 Czech resource/layout checks). Debug Win32/x64/ARM64 and Release x64 builds passed. Initial sandboxed build attempts encountered a timeout and an MSPDB linker error; rerunning outside the sandbox completed successfully.

Computer Use then unloaded the old plugin with configuration saved in the installed Czech/light Samandarin 0.15.1 x64. The registered local `dirsync.spl` was replaced by the Release x64 binary, and matching SHA-256 hashes were verified. Reopening restored the last profile and showed the card/star icon at the right of the directory row. The window was left open with no comparison or synchronization started. This verifies the light-mode live rendering; dark artwork previews are not a live dark-host test. The remaining pilot checks above still apply. No SMB testing or SDK changes were made.

## Multiple profiles, dark GUI and full restarts — 2026-10-18

Verified commit `da8d00e7701f01c469c1465b6b0111d18a938036` using Computer Use on a separate **Samandarin 0.16 x64 developer build**, Czech interface, actual dark appearance. The host, plugin, portable configuration and NTFS fixture are under `C:\Samandarin\builds\dirsync-profiles-gui-20261018`. This is an isolated test configuration seeded from an earlier portable-host test, not a fresh first-install test. The installed user's instance and its profiles were left untouched.

Host EXE SHA-256: `f0b2a56885b363dc0f75926fedd3036bb1954b775753348cbca0fc4d48574dd`. Plugin SHA-256: `6ade50f6089b1780ba7c6bc08198a70b6616be714c4659608f2c3c547569676b` (matches the Release x64 build).

| GUI scenario | Observed result |
|---|---|
| Save profile A with Czech, CJK and supplementary Unicode characters | `Pilot A – žluťoučký 漢字 😀` saved with Unicode left/right paths, `*.*`, Subfolders ON and By content OFF. All manager fields were readable in dark mode. |
| Change masks and By content, then Save as B | A acquired the dirty asterisk. B saved `*.txt;*.md` and By content ON; A retained its original values. |
| Rename B | `Pilot B – přejmenováno & 漢字` appeared in the manager, window title and quick menu. The literal ampersand remained visible. |
| Three profiles together | A, B and temporary C were listed separately; selection changed the saved-details preview. OK committed the manager draft and selected B without starting a comparison. |
| Cancel deletion of C | The confirmation closed and C remained in the manager. No profile was removed. |
| First full host restart | Closed the plugin, exited the test host normally and verified its window disappeared. Relaunched the exact test EXE. Plugin registration remained available; opening restored renamed B, Unicode roots, masks and By content. Status stayed Ready with zero rows. A, B and C remained in the menu. |
| Confirm deletion of C | After explicit user approval, removed only `Pilot C – dočasný`. A/B remained, details cleared when the deleted selection disappeared, and Rename/Delete became disabled until another profile was selected. |
| Load A through the manager, then B through the quick menu | A restored `*.*` and By content OFF; B restored `*.txt;*.md` and By content ON. Neither selection started Compare or a file operation. C disappeared from the quick menu. |
| Second full host restart, after committing deletion | B reopened as the active profile with the correct settings. The menu contained A/B and no C. Dark appearance and the profile star icon remained readable. |

After normal shutdown, an independent read-only decoder inspected `ProfilesV1` in the portable `config.reg`: exactly two profiles, IDs 1/2, active ID 2, flags 1/3 and filters 31 for both, exact Unicode names/paths and masks. Local evidence is `storage-verification.json` with the decoder `verify-storage.py`, outside Git beside the test host. The fixture `alpha.txt` retained SHA-256 `5a3b501918917aa4d0a1ae00106c6bd8f9bce3f3232ff60b219371ed8bf01411`; right and recovery directories stayed empty. No comparison, synchronization or filesystem-deletion command was invoked during this pass. The test host was closed; the original installed instance remained open.

No plugin defect was found, so this pass changes documentation only. Initial setup issues were corrected locally: the launcher resolved a bare EXE path to the installed application, while the exact `process:` identifier launched the portable host; its RegFile parser required the original UTF-8 format rather than UTF-16. The extra launcher-created instance was closed. Nested modal input worked when targeted through the host's root window rather than the preview window. The minimal test host also displayed its known missing `salmon.exe` warning at startup. These are test-environment limitations, not fixes to plugin or SDK code.

This completes the previously pending multiple-profile, rename, deletion/Cancel, live dark-mode and full-restart checks. Live duplicate-name correction, a second English host, monitor-DPI changes and a successful live resize sweep remain outside this pass; storage/resource tests already cover duplicate names and DPI metrics. The attempted edge drag did not resize the preview and is not counted as a resize test. No SMB testing or SDK modifications were performed. Existing five-suite Debug/Release and build-matrix results apply to the unchanged tested code; remote checks are verified separately for the documentation commit.

## Version 0.2.1: unsaved profile on close — 2026-10-18

Normal closing now checks whether the active saved profile differs from the form. A compact native dialog offers Save changes (selected by default) or Discard changes, confirmed with OK; Cancel, Escape and the prompt's close button keep the main window open. This applies to Close, Escape, Alt+F4 and the main window's close button. An unchanged profile or the default unsaved task closes without this prompt. Saving validates both roots and the mask and persists the current profile without changing its ID/name or shared recovery/appearance preferences. Invalid input or persistence failure leaves the window open; a failed persistence callback restores the old in-memory profile. The SDK persistence callback is not a transactional storage guarantee.

If an operation is running, Close first requests Stop and waits for the worker to finish, then asks about the profile. Cancelling that question leaves an idle usable window; it cannot resume the stopped operation. Forced plugin unloading and host teardown retain the existing noninteractive shutdown path.

All five x64 suites passed in Debug and Release: engine/storage 2,211; copy/recovery 3,729; lifecycle 2,367; English resources/layout 3,938; Czech resources/layout 4,021. Added checks cover successful profile saving, rejected masks, persistence failure/rollback, preservation of shared preferences, and the actual dialog procedure's default choice/Cancel. Both compiled language resources were measured at 96, 120, 144, 192 and 240 DPI. Debug Win32/x64/ARM64 and Release x64 builds passed. Resource measurements are not live monitor-DPI tests.

Computer Use verified the Release x64 build on the isolated Czech/dark Samandarin 0.16 developer host described above:

| GUI scenario | Observed result |
|---|---|
| Change B's By content setting and click Close | Dirty asterisk and compact localized prompt; Save selected by default. |
| Escape in the prompt | Main window stayed open with the unsaved form unchanged. |
| Alt+F4, then default Save + OK | Plugin closed. Reopening restored By content OFF with no dirty asterisk. |
| Change By content back ON, Escape from the main window, select Discard + OK | Plugin closed. Reopening retained the previously saved OFF setting, with no dirty asterisk. |
| Close the unchanged profile | Closed immediately without a profile question. |

The test host then exited normally and disappeared from the window inventory. Independent decoding of its portable configuration found profiles A/B, selected ID 2, both flags 1 and filters 31, with the original Unicode names/roots and masks. The fixture retained SHA-256 `5a3b501918917aa4d0a1ae00106c6bd8f9bce3f3232ff60b219371ed8bf01411`; right/recovery directories remained empty. The user's installed instance was left open. This pass did not run comparison or synchronization. Live persistence-failure injection and closing a dirty profile during a worker operation were not exercised in the host GUI; the helper regression tests and existing worker lifecycle tests provide narrower automated coverage.

Local working-tree package: `out/packages/profile-close-0.2.1-local-20261018/dirsync-0.2.1-x64.zip`, SHA-256 `b634c9d7d248c0a5a33545c9e0fae4200dc43f55509f320d6d795b53315e2999`. Packaging verified 14 manifest payload entries, extracted hashes, x64/version metadata and local guide links. This is an unpublished local build based on the working tree after `70b9581`; its NOTICE records that base commit, not a committed revision of these changes. No remote CI result is claimed. Public README download links continue to point to released 0.2.0. No SMB testing or SDK changes were made.

### Conventional close buttons — 2026-10-18

The user requested the standard Save / Don't save / Cancel interaction. The radio choices and separate OK were replaced with three direct buttons (Czech: Uložit / Neukládat / Zrušit). Save is the default; Escape and the close button cancel. The dialog is now 230 × 58 DLU instead of 230 × 88. Earlier GUI observations above describe the superseded radio layout. Storage, validation and worker shutdown behavior are unchanged. The previous local package predates this correction.

All five x64 Debug/Release suites passed again: 2,211 engine/storage, 3,729 file-operation, 2,367 lifecycle, 3,907 English and 3,991 Czech resource/layout checks. The native-control test verifies the three button IDs and Save as the default; compiled resource checks cover 96–240 DPI. Debug Win32/ARM64 and Debug/Release x64 plugin builds passed. The new layout has not yet been inspected live in the host GUI. Updated local package: `out/packages/profile-close-buttons-0.2.1-local-20261018/dirsync-0.2.1-x64.zip`, SHA-256 `c32a4eb18cf8c197e9891eae5d05f9ba3c0a1f53bb389481faed34455106793e`; all 14 manifest entries and extracted hashes verified. No publication, SMB test or SDK change.

### Three-button live GUI verification — 2026-10-18

Computer Use loaded the corrected Release x64 plugin into the same isolated Czech/dark Samandarin 0.16 developer host. Installed and build binary SHA-256 matched: `f2604965b602ff88a52136ab4cbc9bb0b77ae9970a71e40f2d32cf23c5e272e2`. The compact dialog displayed all three complete captions, aligned in one row: Uložit / Neukládat / Zrušit. No defect was found.

| Scenario | Observed result |
|---|---|
| Set profile B's By content ON, Close, click Zrušit | Main window stayed open, By content remained ON and the dirty asterisk remained. |
| Alt+F4, click Uložit | Plugin closed. Reopening restored By content ON with no dirty asterisk. |
| Set By content OFF, Escape from main window, click Neukládat | Plugin closed. Reopening retained the saved ON value with no dirty asterisk. |
| Close unchanged profile | Immediate close without the profile question. |

After normal host shutdown, decoding its portable configuration confirmed selected profile 2, profile A flags 1 and profile B flags 3, both filters 31, unchanged names/roots/masks. Fixture hash remained `5a3b501918917aa4d0a1ae00106c6bd8f9bce3f3232ff60b219371ed8bf01411`; right and recovery directories remained empty. No comparison or synchronization ran. The installed user's instance was left open. The first Ctrl+I after startup had no last plugin command to repeat; launching through Plugins initialized that command and later Ctrl+I reopened it normally. The known missing crash-helper warning was dismissed. Live verification here covers Czech/dark at the current desktop DPI; English and other DPI values retain the compiled-resource coverage above. No SMB tests or SDK changes.
