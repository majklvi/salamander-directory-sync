# Shared directory history — 0.4.0 development

Verified 2026-10-21. No SDK change, SMB test, synchronization, release or remote CI work.

## Behavior

The two root inputs are native editable dropdowns, with a single shared history of up to 20 normalized absolute directory paths. A completed comparison with a usable snapshot adds both roots, left first, followed by the right root and older entries. Case-insensitive ordinal comparison eliminates duplicates, including trailing separators normalized by the existing path helper. Reusing a pair does not cause another configuration write if the final history order is unchanged.

The history is global, independent of profiles. It is saved through the existing SDK configuration callback as bounded UTF-16 binary multi-string data (`DirectoryHistoryUtf16V1`), preserving long and Unicode paths. Old configurations start with an empty history. Loading rejects malformed size, terminators, invalid Unicode, noncanonical/relative paths, duplicate entries and over-limit lists. The main comparison still performs its normal accessibility and safety checks; history parsing does not access a directory or network.

The closed inputs retain their original row, widths, browse buttons, mnemonics, Tab order and busy/Panel-selection restrictions. The dropdown width grows to fit paths, bounded by the monitor work area; resize preserves the dropdown's height. The native arrow or Alt+Down opens the list. Selecting a path changes only that root, never starts Compare or Synchronize, and uses the existing manual-proposal discard confirmation. Arrow navigation inside an open list does not prompt until the selection is committed. Selection notifications defer validation until the native edit has received its final text. Folder browsing explicitly follows the same invalidation path because native combos do not emit edit notifications after programmatic text changes.

## Automated verification

All five x64 suites pass in Debug and Release: engine, file operations, lifecycle, EN resources and CS resources. The settings probe now includes 85 round-trip checks covering shared-history limits, newest-first ordering, normalized/case-insensitive duplicates, invalid inputs, long Unicode paths, persistence and malformed registry data. Native preview tests use actual editable combo controls and verify the same list on both sides, full stored values, and rejected history selections preserving the compared root, snapshot and manual proposals in both languages. Resource geometry passes at 96/120/144/192/240 DPI. Debug and Release builds pass for Win32/x64/ARM64; ARM64 is compilation only.

## Live Computer Use

The private Czech dark Samandarin 0.16 developer host under `C:\Samandarin\builds\dirsync-proposal-preview-20261021\cs-dark\host` was used with its existing synthetic NTFS pair. The user's installed windows/configuration were untouched.

1. Compare completed and populated the history with `left-ž` and `right-漢字`.
2. A synthetic new-file proposal was set to Skip. Navigating the left history with Down retained the results; committing the other root with Enter displayed the discard question. Escape restored `left-ž` and preserved Skip and all three rows.
3. The default proposal was restored, and only the private preview/host were closed. The isolated configuration contained both complete Unicode roots in the new history value.
4. After restart with the final build, both entries were immediately available before another comparison. The widened left list showed both complete paths, including their distinguishing endings.
5. Selecting the former right path on the left changed the edit without starting work. After maximizing the preview, the right dropdown still showed the same two entries. Up/Enter selected the former left path on the right, again with zero rows and no worker started.
6. The private preview and host were closed. Independent checks confirmed all 10 prepared files across both fixture pairs, their ADS and modification times unchanged; recovery directories remained empty and no staging files appeared.

The prepared EN/light host was not used in this live campaign; EN and multi-DPI coverage above is automated. No synchronization or deletion was performed. Changes remain local.
