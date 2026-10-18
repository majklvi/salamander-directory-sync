# Directory Synchronization — pilot testing

> [!CAUTION]
> **ALPHA SOFTWARE — RISK OF DATA LOSS. NOT FOR PRODUCTION USE.**
> Use entirely at your own risk. The authors and contributors provide **NO WARRANTY** and accept **NO RESPONSIBILITY OR LIABILITY FOR ANY LOSS OR CORRUPTION OF DATA**. Test only on disposable copies with a separate, verified backup. Keep originals does not provide automatic rollback or eliminate the risk of data loss.

[Česky](directory-sync-plugin-pilot-guide.md) · [Full user guide](directory-sync-plugin.en.md)

Plugin 0.2.1 ALPHA for Samandarin x64, with English and Czech interfaces. The unsigned package contains the compiled plugin; installation requires no compilation. This pilot is for **copies of local NTFS data**; SMB/network-drive tests and SDK changes are outside scope. Lowest tested host versions: 0.15.1 in user testing, and unreleased 0.16 in AI-automated tests of a custom developer build.

Click a heading to switch ascending/descending sorting within each directory. Right-click a heading or use **Settings → Columns** to hide/show optional columns. Both names, directory paths, and Action remain. Column choices/sorting persist; hiding a column does not exclude files from synchronization.

Results are grouped by relative directory (`\` is the root). Headings are not action rows and do not select descendants. Groups disappear when filtering leaves no visible entries. A directory proposed for deletion has its own row in its parent group.

## Profiles (0.2.0)

Since **0.2.1**, closing the main window with a modified saved profile offers three buttons: **Save**, **Don't save** and **Cancel**. Cancel keeps the window and edits open. Save validates directory paths and masks; a failed save keeps the window open. An unchanged profile or an unnamed task closes without this question. If work is running, Close first requests Stop and waits for completion; Cancel at the later profile question cannot resume already stopped work. Forced plugin unload bypasses interactive questions.

Use the **Profiles icon at the far right of the directory inputs**. Its menu lists saved profiles, marks the active one, and offers **Current panel directories** plus **Manage profiles…**. The panel entry restores the directories captured from the host panels when the window opened. Selecting an entry only loads settings; it never starts Compare or synchronization.

The active profile appears in the main window title and the icon's tooltip. An asterisk marks unsaved directory, mask, comparison-option or filter changes. In **Manage profiles…**, select a profile and use **Save current** to replace its settings with the current main-window values, **Save as…** to create a named variant, or Rename/Delete. The complete stored profile appears below the list: both paths, masks, five comparison choices and five result filters, each with an explicit On/Off state. Paths and masks are read-only and can be selected and copied. **OK** applies management changes and loads the selected settings when different; **Cancel** discards all changes made in that management session. Profile deletion never deletes files. Switching away from unsaved settings or existing results requires confirmation; a successful load clears the old comparison. Saving or renaming the current task keeps its comparison results.

The last selected saved profile opens next time. Only explicitly saved values are restored. **Keep originals, recovery directory, appearance, columns, sorting and relative display remain global**; the host's time tolerance is read for each comparison. Panel selection, highlighted rows, results and manual proposals are not saved. Use **Save profile as…** to keep a second variant. Profile management is disabled during an operation. Profiles have no import/export, scheduling or automatic execution in this version.

## Installation

1. Close host instances using an older plugin. Extract the ZIP's complete `dirsync` directory into `plugins` beside `salamand.exe`; retain `lang` with both SLGs.
2. In **Plugins → Plugins Manager → Add…**, choose `plugins/dirsync/dirsync.spl` if not registered. Launch **Directory Synchronization → Directory synchronization…** (Czech: Synchronizace adresářů).
3. In **Settings…**, initially keep **Keep originals** enabled and select an existing recovery directory outside both trees that is not an ancestor of either. Allow space for backups and the current transfer. Preferences persist; the host controls language and light/dark mode.

## Short workflow

1. Choose two separate test copies of directories. Set masks/options and click **Compare**. This only reads data.
2. Review proposals and reasons. Combine filters; right-click or Shift+F10 changes highlighted-row proposals. **Skip** omits an item. Action labels can be enabled in Settings.
3. **Synchronize…** offers separate new-copy, overwrite, and deletion commands, each with its own confirmation. **They execute all visible eligible proposals, not just highlighted rows.** Hidden and Skip rows are excluded. Check counts and recovery mode in the confirmation.
4. Deletion requires asymmetry: **left is the source**, and right-only entries are deletion candidates. Directory entries require Empty folders. Children go first, and directories are removed only when actually empty; an omitted child protects its parent. Roots and common empty directories remain.
5. Read each result, then Compare again. New comparisons discard manual proposals, so reapply Skip as needed. The six comparison checkboxes affect the next Compare; path/mask edits invalidate the old plan.
6. The × beside Compare or Synchronize stops that work. Closing waits for the worker. **Completed changes remain; automatic rollback is not provided.**

Try ordinary copying, overwriting, and a small removal of your own test data, then restore manually to a new checking directory. On larger results, watch responsiveness, filters, keyboard control, and resizing. Report experiences with different DPI or multiple monitors too. Repeating laboratory fault-injection tests is unnecessary.

## Direct removal from the context menu

Upper **Mark for copy… / Mark to skip** commands only change proposals. The separate **Remove selected… → Left… / Right… / Both sides…** command removes highlighted entries. It uses the Recycle Bin normally, or permanent deletion with Shift held while activating a side command. Check mode and actual paths in OK/Cancel confirmation. Keep originals does not apply; permanent removal has no plugin backup. Directories must be empty, and selecting a parent does not include children. The first error stops the batch; Compare again afterwards. Restore recycled entries through the Windows Recycle Bin.

## Recovery and main limits

- ON saves and verifies included originals before overwriting/deleting. The summary names `manifest.jsonl` and the execution journal. With OFF, synchronization commands **create no new backup or persistent execution journal and do not use the Recycle Bin**. Older backups are not automatically cleaned and continue to occupy space.
- Manual recovery: use `saved` on a successfully saved manifest item. Recreate directory structure in a new checking directory first, then copy files from their saved paths with ordinary F5. Do not copy the entire auxiliary run tree into the original root. Verified recovery covers structure, file data, ADS, and file times. F7 creates directories with new timestamps; exact directory metadata and original permissions are not automatically restored.
- Hidden/System entries are included by mask/scope even if the main panels hide them. H/S attributes are in row tooltips. By content compares only main data, not ADS; timestamp tolerance follows Configuration → General at each Compare (normally 2 seconds; exact timestamps when disabled). Hourly DST shifts are not inherited.
- Standalone copying of empty directories is not supported. Reparse/junction/symlink, EFS, and unsupported cloud/offline entries are blocked. Directories with their own ADS are rejected before deletion. Read-only attributes are not bypassed for overwrite/deletion.
- The first error stops remaining work. This is not transactional backup or a complete NTFS backup tool. Do not modify either test tree through another program during a run. Created empty parents, incomplete journals, or reported staging residue may remain.
- Transfers use the plugin's own worker, without the core's shared queue, background transfer, or speed limit. Runtime verification is x64. Live runs at other system DPI and cross-monitor moves remain to be checked; accessibility enumeration of thousands of rows previously caused delays.

See the [full guide](directory-sync-plugin.en.md), included as `README.md` in the ZIP. Detailed development records remain on GitHub and are not bundled.

## Reporting bugs

Report in [Issues](https://github.com/majklvi/salamander-directory-sync/issues). Copy a result summary with **Ctrl+Shift+C**, or selected rows' relative paths with **Ctrl+C**. Ideally include:

- ZIP name and SHA-256 from the release's `package-sha256.txt`; `files.json` also contains the SPL hash.
- Samandarin/Windows versions, language, light/dark scheme, monitor DPI, and confirmation of local NTFS.
- Exact steps, masks, comparison options, filters, manual proposals, operation type, and Keep originals ON/OFF.
- Expected/actual results, counts, and exact errors from the summary, optionally a screenshot. For delays, include row count, waiting time, and whether UI Automation was active.
- With ON, relevant `manifest.jsonl`, `delete.jsonl`, or `overwrite.jsonl` if present. Their absence with OFF is not a bug. Remove sensitive paths/names from copies before sharing; retain original journals for recovery. Do not send private file contents. A small synthetic reproducer is best.

After unexpected data changes, stop further writes, retain the summary and recovery run, and inspect copies in a separate directory first.

Since 0.1.3, **Settings → Columns** offers independent relative-size and relative-time display, both off by default. Differing paired values show differences from the other side; equal values and missing counterparts retain absolute display. Sorting still uses actual sizes/times. See the full guide.

Columns are symmetric: Name | Size | Modified | Action | Modified | Size | Name. The first three belong left, the final three right. **Settings → Columns → Show side names in headers**, off by default and remembered, adds explicit side labels.
