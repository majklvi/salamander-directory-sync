# Version 0.1.6: proposal menu validation

Validated on 2026-10-18. The Release x64 plugin and both language modules were built locally. The previously loaded plugin was unloaded before replacing its files; the installed Samandarin 0.15.1 x64 host then confirmed version 0.1.6 in Plugin Manager. Live tests used the Czech interface and light appearance.

## Isolated local GUI test

Fixture: `C:\Samandarin\builds\dirsync-menu-0.1.6-20261018`, outside Git. Its left/right trees contain eight four-byte files and one empty directory. Comparison produces five rows: newer left file, newer right file, left-only file, equal files and a file/directory conflict. All filters were enabled; asymmetric mode was disabled.

| Selection / current proposals | Expected and observed menu behavior |
| --- | --- |
| One default copy-right row | Restore default and mark right disabled; mark left enabled. |
| Same row after marking left | Restore default and mark right enabled; mark left disabled. Restoring returns the right arrow. |
| Two rows with opposite default directions | Both direction commands enabled; restore default disabled. |
| Those two rows after marking right | Only one proposal changes; status reports one changed and one unchanged. Mark right is now disabled; restore enabled. |
| Restore the two rows | Only the manually changed proposal returns to its default; one changed and one unchanged. |
| Opposite directions plus a left-only row, mark left | Command remains enabled. Only the copy-right row changes; the already-left and ineligible left-only rows remain unchanged. Status reports one changed and two unchanged. |
| Already-left row plus left-only row | Mark left disabled: neither selected row can change to that action. Mark right remains enabled; restore disabled because both retain their defaults. |
| Blocked file/directory conflict alone | All proposal commands disabled. |

Menus were opened with the mouse and Shift+F10; multi-selection used Shift+Down. Proposal edits and restoration updated arrows and status counts without starting file operations. No synchronization or immediate deletion command was invoked. After testing, all eight files retained their SHA-256 hashes, sizes and write timestamps; both file sets and the conflict directory remained present. No additional files appeared in either tree.

No additional GUI defect was found. Menu availability now requires at least one valid selected row whose action would actually change; unchanged or ineligible rows do not veto a useful command for other selected rows.

## Automated validation

All five x64 suites passed in both Debug and Release before GUI validation; the tested implementation did not change afterward:

- Engine: 542 assertions.
- Copy, recovery, overwrite and deletion: 3,729 assertions.
- Lifecycle: 2,366 checks, including the proposal probe extended from 77 to 93 internal checks.
- English resources/layout: 2,467 checks.
- Czech resources/layout: 2,504 checks.

The local x64 package passed manifest and binary validation. The working ZIP is a local test artifact, not a published 0.1.6 release. README download links intentionally continue to target the available 0.1.5 release.

This change does not modify dialog geometry, file executors or the SDK. This report claims no new dark-mode, live DPI, SMB or file-operation coverage. Current-head hosted build/test/package and CodeQL results are available in GitHub Actions for the commit containing this report.
