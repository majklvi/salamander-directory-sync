# Directory Synchronization for Samandarin

> [!CAUTION]
> **ALPHA SOFTWARE — RISK OF DATA LOSS. NOT FOR PRODUCTION USE.**
> Use entirely at your own risk. The authors and contributors provide **NO WARRANTY** and accept **NO RESPONSIBILITY OR LIABILITY FOR ANY LOSS OR CORRUPTION OF DATA**. Test only on disposable copies and keep a separate, verified backup. Keep originals does not provide automatic rollback or eliminate the risk of data loss.

**[Download the ready-to-use 0.2.0 ALPHA x64 plugin](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.2.0/dirsync-0.2.0-x64.zip)** · [SHA-256 checksums](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.2.0/package-sha256.txt)

The ZIP contains the compiled plugin and both language modules. **No compilation, Visual Studio, or SDK is needed to install it.**

A native directory comparison and synchronization plugin for **Samandarin**, with English and Czech interfaces. Compare two directory trees side by side, review the proposed actions, then copy, overwrite, or remove right-only items in asymmetric mode. Optional recovery copies preserve originals before synchronization overwrites and deletions.

**Version 0.2.0 ALPHA · local NTFS pilot · x64** · [Česky](README.cs.md)

**Since version 0.2.0, the plugin supports named profiles.** Save directory pairs, masks, comparison options and result filters, then switch tasks from the star button beside the directory inputs. The last selected profile opens next time; loading a profile never starts comparison or synchronization. Recovery and appearance remain shared across profiles. [Using profiles](docs/directory-sync-plugin.en.md#profiles-020).

**Development version: 0.2.1 ALPHA.** Adds copying of empty directories, profile import/export and Save / Don't save / Cancel when closing a modified profile. A combined visible-plan command runs new copies, overwrites and deletions after one confirmation. The download above remains the published 0.2.0 release.

<img width="917" height="796" alt="image" src="https://github.com/user-attachments/assets/a994cc19-ca1a-40bc-99d3-6edb76edfd51" />

## Highlights

- Named synchronization profiles with remembered last selection.
- Optional relative file sizes and timestamps.
- Custom colors that follow the host's light or dark scheme.
- Symmetric columns, directory groups, sorting, and combined filters.
- Editable action proposals and a read-only comparison preview.
- Optional, remembered recovery of originals before overwrites and deletions.
- Native UI, Unicode and long paths, and host display preferences.

## Relative sizes and timestamps

Enable **Show sizes relatively** and **Show times relatively** independently in **Settings → Columns**; both are off by default. For paired files at the same relative path, relative display replaces differing values with their difference from the opposite side. For example, an 8 KiB file paired with a 4 KiB file shows **4,096 B larger** on one side and **4,096 B smaller** on the other. Timestamps can show **2 d 3 h newer** and **2 d 3 h older**.

Size differences are exact byte counts, with digit grouping taken from the host. Time differences are elapsed UTC durations with 24-hour days; differences below one second have a dedicated label. Equal values, unpaired entries, and directories retain their absolute display.

These options only change presentation: they do not change comparison results, timestamp tolerance, proposed actions, or sorting by actual values. The plugin reads the host's time tolerance for each comparison, with a three-second fallback if it is unavailable. Relative display applies to existing results without another scan and persists across restarts.

## Appearance and preferences

Open **Settings…** in the preview or **Plugins Manager → Configure…**. The color button beside each item opens a native color picker. **Restore default colors** restores only the current light/dark palette. The host chooses the active scheme; the plugin has no separate theme switch.

Preferences also control action labels, optional columns, side labels in column headings, relative values, Keep originals, and the recovery directory. **OK** saves changes; **Cancel** discards them. Appearance changes preserve current results and manual proposals.

<img width="461" height="498" alt="image" src="https://github.com/user-attachments/assets/21ac9ad5-4465-47fa-aeb5-3c42a28f0929" />

## Tested host versions

The lowest tested Samandarin versions are **0.15.1** in user testing and **0.16** in AI-automated testing using a custom developer build. Version 0.16 had not yet been released at the time of testing.

## Install the alpha build

1. Download the [ready-to-use x64 ZIP from release 0.2.0](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.2.0/dirsync-0.2.0-x64.zip). It includes the compiled `dirsync.spl` and English/Czech language modules; no build step is required. The package is unsigned and includes English `README.md`/`PILOT.md` and Czech `README.cs.md`/`PILOT.cs.md`. Detailed development records stay online.
2. Close any host instance using an older copy. Put the entire `dirsync` directory inside the host's `plugins` directory, beside `salamand.exe`. Keep `dirsync.spl`, `lang/english.slg`, and `lang/czech.slg` in their original layout.
3. In **Plugins → Plugins Manager → Add…**, select `plugins/dirsync/dirsync.spl` unless already registered. Launch **Plugins → Directory Synchronization → Directory synchronization…**. The host selects the interface language.

Start with copies of local NTFS data. Before overwrites or deletions with Keep originals enabled, select a recovery directory outside both compared trees that is not an ancestor of either. Allow room for recovery copies and staging. Old recovery runs are not cleaned up automatically.

## Typical workflow

1. Check the left and right paths or use their **…** browse buttons. Set the mask (`*.*` includes all names), Subfolders, and other comparison options. **Compare** builds a preview without writing files.
2. Review groups, symbols, and reasons. Combine filters to show relevant results. Right-click highlighted rows, or press **Shift+F10**, to mark them for a direction or **Skip**. Marking a proposal does not start copying.
3. **Synchronize…** offers separately confirmed batches of new copies, overwrites, and asymmetric deletions. **These commands act on all visible eligible proposals, not only highlighted rows.** Hidden and skipped rows are excluded. Check counts, direction, and recovery mode in the confirmation.
4. Read the result summary, then compare again before another batch. A new Compare discards manual proposals. Comparison checkboxes affect the next Compare; changing a path or mask invalidates the current plan.
5. The small **×** beside Compare or Synchronize stops the corresponding work. Closing waits for the worker. Completed operations remain in place; Stop is not undo.

**Asymmetric mode uses the left tree as its source.** Right-only entries become deletion candidates. Directory entries require Empty folders. Directories are removed after their included children and only when actually empty; skipped, filtered-out, or retained children protect their parent.

### Selection and direct removal

Row commands change proposals for highlighted visible rows. **Ctrl+A** selects visible rows; **Ctrl+C** copies relative paths. Copy paths also offers full left/right paths. The separate **Visible plan** submenu uses the visible eligible plan regardless of selection.

The separate **Remove selected…** command immediately removes highlighted items on a chosen side: normally to the Windows Recycle Bin, or permanently when **Shift is held while activating the side command**. It does **not** use Keep originals or create a synchronization recovery copy. Check its confirmation carefully. Directories must be empty; selecting a parent does not select descendants. Restore recycled items through the Windows Recycle Bin.

## Recovery

With **Keep originals ON**, synchronization saves and verifies originals before overwrites/deletions and records the run in the recovery directory. With it **OFF**, these commands make no new recovery copy or persistent execution journal and do not use the Recycle Bin. Existing backups remain untouched.

For manual recovery, find the run and `manifest.jsonl` named in the summary. Match a successfully saved item's `original` and `saved` paths. First copy saved data into a separate checking directory and inspect it before restoring to the original location. Recreate directory structure from directory records as needed; do not copy the entire run directory into the original tree. Original ACLs and every NTFS property are not automatically restored. See the [detailed user guide](docs/directory-sync-plugin.en.md) and [recovery records (Czech)](docs/directory-sync-plugin-recovery.md).

## Limits and bug reports

- Runtime testing covers **local NTFS x64**. SMB is untested. Win32/ARM64 builds are compile checks, not runtime validation.
- Hidden/System items are included when matching the mask and scope, even when the host panels hide them. Their attributes appear in the row tooltip. By content compares the main data stream, not alternate data streams (ADS).
- No standalone copying of empty directories, synchronization history, automatic mixed-operation batch, automatic restore, or backup cleanup. Reparse points, EFS, and unsupported cloud/offline items are blocked. Read-only destinations are not forcibly overwritten or removed.
- The first error stops the remaining batch. Completed changes are not rolled back. This is not transactional backup or a complete NTFS clone. Avoid modifying either tree through another program during a run.
- Transfers use the plugin's own worker, without the host's background transfer window, speed limit, or shared queue. Live cross-monitor DPI remains unverified. Inspecting thousands of rows through UI Automation has caused delays in earlier pilot sessions.

Report bugs in [Issues](https://github.com/majklvi/salamander-directory-sync/issues). Include package version/hash, host and Windows versions, language/theme/DPI, masks/options/filters, recovery mode, exact steps, and expected versus actual results. Copy a result dialog with **Ctrl+Shift+C**. Include errors and relevant journal excerpts with private paths removed; a small synthetic reproducer is best. After unexpected data changes, stop further writes and preserve the summary and recovery run.

## Build from source and test (developers)

Requires Windows, Visual Studio 2026 C++ desktop tools (v145 and target architecture components), Windows SDK 10, and PowerShell 7.3+. No host checkout/import library, Python, vcpkg, signing certificate, or dependency download is required. MSBuild is found using the system's vswhere.

```powershell
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Platform x64
pwsh -NoProfile -File tools/build.ps1 -Configuration Debug -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Debug
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Release
pwsh -NoProfile -File tools/package.ps1 -OutputDirectory out/package-01
```

Outputs go into `out/`, with separate intermediates per project/configuration/platform. Build arguments: `-Configuration Debug/Release`, `-Platform Win32/x64/ARM64`; standard runtime tests are x64, and packaging is Release x64. `-Project` builds one component. Open `build-support/dirsync.vcxproj` for Visual Studio; both language modules are dependencies. The historical `protocol_probe` is a manual probe, not an automatically executed test.

Own CI builds Debug Win32/x64/ARM64 and Release x64, runs all five x64 suites in Debug and Release, verifies the ZIP, and performs actual CodeQL C/C++ analysis. Actions are pinned to commit SHAs. See [Actions](https://github.com/majklvi/salamander-directory-sync/actions) for a specific revision and the [migration validation and reviewed CodeQL findings](docs/migration/repository-migration.md) for the distinction between workflow success and an absence of alerts.

## Documentation and license

[Český přehled](README.cs.md) · [Detailed user guide](docs/directory-sync-plugin.en.md) · [Pilot checklist](docs/directory-sync-plugin-pilot-guide.en.md)

This repository retains the plugin's own development history and documentation from the original fork. The [commit map](docs/migration/commit-map.tsv) connects original and exported SHAs. Dated reports in `docs/` describe their historical stages, not necessarily current limitations.

Licensed under [GPL-2.0-or-later](LICENSE). See [dependency credits](docs/third_party.md), [export attributions](EXPORT-ATTRIBUTIONS.md), and the [pinned support snapshot](EXPORT-SUPPORT.json) for third-party licenses and provenance.
