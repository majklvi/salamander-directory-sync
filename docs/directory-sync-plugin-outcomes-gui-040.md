# Mixed outcomes in the GUI, exports and stored originals — 0.4.0

Verified on 2026-10-21 with commit `e7d0b8551329240b9cc6c6a253fb4f5dfce34656`. Version 0.4.0 remains in development. This is a local NTFS x64 acceptance check, not a release, SMB test or SDK change.

## Isolation and method

Computer Use operated the shipping English dialogs in a private dark-mode Samandarin 0.16 developer host. The host configuration, two synthetic Unicode directory pairs, separate recovery directory, deliberate file-lock worker and evidence are under `C:\Samandarin\builds\dirsync-outcomes-gui-20261021`, outside Git. Existing user windows and data were not test targets.

The plugin and both language modules were copied from the previously verified Release x64 build without source changes. The tested SPL SHA-256 is `7857ffdbd073d8713d4ce7d7728e63114e33e51f721c345da3b667d5c023ffaa`. Each synthetic file has a named `test` stream and a captured main-data hash, stream value and modification timestamp. Reports are saved outside both comparison trees and the recovery directory.

This campaign checks how genuine executor outcomes reach the result dialog, native CSV/text Save dialogs and read-only recovery browser. It does not inject reports, manufacture journal records or substitute automated rendering for the observed GUI.

## Mixed batch

The asymmetric comparison loaded 12 rows. The equal filter hid `equal.txt`; `3-manual-skip.txt` and `6-excluded.txt` were manually marked Skip. Despite the latter fixture filename, it is a manual Skip in this batch, not an exclusion in confirmation. Only that row was highlighted when reports were saved; selection did not restrict export.

With Keep originals ON and the external `recovery-漢字` directory selected through Preferences, the user confirmed the exact disposable destructive plan immediately before execution. Ordinary file locks produced error 32 without changing the compared metadata:

- `0-done.txt` copied successfully.
- Skip all at `1-skip-all.txt` skipped it explicitly; the matching error at `2-auto.txt` was skipped automatically without another question.
- Skipping the unavailable original `5-blocked.txt` blocked its dependent overwrite, while the independent `4-overwrite.txt` was preserved and replaced.
- `9-delete.txt` and the empty `empty-delete` directory were removed. Skipping removal of the locked `tree-ž\locked-漢.txt` retained that file and blocked deletion of `tree-ž` without attempting the parent removal.

The actual **Synchronization result** showed:

| Field | Observed value |
| --- | --- |
| New copies, completed / planned | 1 / 3 |
| Overwrites, completed / planned | 1 / 2 |
| Deletions, completed / planned | 2 / 4 |
| Skipped during execution | 3 |
| Prerequisite or child retained | 2 |
| Errors / cancelled; not started | 0; 0 |
| Other visible proposals omitted | 2 |
| Excluded in confirmation | 0 |

The explanation explicitly says that synchronization is incomplete and completed changes remain. The failed original-save prerequisite is not double-counted as an additional skipped main operation.

## CSV and text

After closing the result without comparing again, **Synchronize → Export comparison and results…** remained available. The actual native Save dialog created `reports-ž漢\smíšená.csv`; choosing **Text report** changed the default extension to `.txt`, and the second export created `smíšená.txt`.

Independent parsing of both files verified all 12 comparison rows, including the hidden equal row and both manual Skip rows. The original and current proposals remain distinct. The two manual Skip rows have no executed-operation records. The combined batch is incomplete but not cancelled, and its selected directions, overwrite/deletion authorization, recovery root and frozen visibility agree with execution.

The main operation records reproduce the GUI counts exactly. The first copy skip has `automatic_skip=false`, the next matching skip has `automatic_skip=true`; both retain their attempts and last error. The blocked overwrite and parent deletion have zero attempts. The parent records the complete Unicode blocking-child path. Saved-original records are separate from the destructive operations, so a retained deletion child can truthfully have a saved recovery copy and a skipped removal.

## Stored originals

Opening **Settings → Stored originals…** found the two genuine recovery sessions:

- The overwrite session is **Incomplete**. `4-overwrite.txt` shows **Saved (recorded)** and **Replaced (recorded)**. `5-blocked.txt` shows **Skipped during execution** for its recovery copy and **Not executed: prerequisite or child retained** for its overwrite. Double-clicking the latter opens details with both full paths, size 17 and error 32.
- The deletion session is **Manifest complete**, because all four originals were preserved. Its operation column independently shows **Deleted (recorded)** for the file and empty directory, **Skipped during execution** for the locked child, and **Not executed: prerequisite or child retained** for its parent. A complete preservation manifest therefore does not claim that deletion or synchronization completed.

After a real host restart, Keep originals and its Unicode path remained available. The browser reopened the same two sessions; Refresh retained their recorded outcomes and counts. No new recovery session was created by the later new-copy-only cancellation case. The browser remains a read-only record viewer: it does not reverify contents or restore data automatically.

## Stop and confirmation exclusion

A second independent pair contained three left-only files and one right-only file. Its non-asymmetric confirmation had both copy directions available. Turning off right-to-left excluded `3-excluded.txt` without changing its proposal or hiding its comparison row. There were no overwrites or deletions in this batch.

The first rightward copy completed, and the next deliberately locked source opened an error question. The preview's explicit Stop control ended the command. The result showed **new copies 1/3**, **one cancelled item**, **one not started**, **one excluded in confirmation**, and zero execution skips or dependency blocks.

Both `zastavená.csv` and `zastavená.txt` were then saved through the GUI. They contain `copied`, `cancelled` and `not_started` operation records, a cancelled/incomplete batch, and the fourth comparison row with `confirmation_excluded=true`, `execution_selected=false` and `visible=true`. The excluded row has no operation record. The previously saved originals remain intact, and neither cancelled/unstarted target nor the excluded left target exists.

## Independent verification and limits

The outside-Git `verify.py` passed **201 checks**, covering:

- Equivalent CSV/text records after decoding text escapes and allowing different generation times; UTF-8 BOM, CRLF, full Unicode paths and exact byte sizes.
- GUI counts, individual statuses, visibility/manual Skip/confirmation exclusion, attempts, automatic skips and blocking paths.
- Agreement between exported recovery/overwrite/deletion records and actual JSONL statuses, attempts, last errors and blocking paths.
- Unchanged source and retained-target hashes, ADS and timestamps; correct completed copies/replacement; absent deleted, skipped, cancelled, unstarted and excluded targets; retained parent and no staging residue.
- Verified saved-file data/ADS/timestamps and saved-directory existence, with no supposed copy for the skipped original.
- Unchanged hashes of all 11 report/recovery files across read-only catalog viewing, restart and Refresh.

| GUI-created report | Bytes | SHA-256 |
| --- | ---: | --- |
| `smíšená.csv` | 18,717 | `653b2de1f496a4b566c5a07fa08d9fc73e0f8bd5223de84145cff1526e4107f3` |
| `smíšená.txt` | 17,176 | `41f86aa6c8752014aae3bd0aa54f3f0544cec944277dc645ec43cac7d2493221` |
| `zastavená.csv` | 11,085 | `20cbf80298baf3e8018e550ac626814bf204e8a983ba63b787e146720ebffd94` |
| `zastavená.txt` | 6,311 | `abf0e49abd8a91f43bbcad47aebef0cdb79ddc8500ae7435abdf24be12da8706` |

**No executor, export or catalog defect was found; no production-code fix was needed.** Evidence includes `baseline.json`, question/result/catalog accessibility captures, the four GUI-created reports, actual recovery records, `read-only-inventory.json`, `verify.py` and `verification.json`.

Preparation first added a BOM to the disposable REGEDIT4 configuration, which the host rejected unchanged. Removing the test-only BOM allowed startup. An initially hidden process launch also left host panels uninitialized and produced inappropriate table colors; a subsequent normal visible launch initialized the panels and restored readable dark catalog tables without changing plugin code. This does not establish a general product-theme regression or a fix. The launcher additionally opened an installed-host instance; only that additional instance and the private test host were cleanup targets, not the pre-existing user window.

The automation sometimes exposed owned-dialog elements through their disabled owner with unusable indexes, or returned no accessibility text immediately after Refresh. Fresh screenshot-based input completed the real dialogs; the post-Refresh table retained the expected statuses visually. No production ownership change was made for automation.

This campaign used English/dark GUI at the current desktop DPI. It does not claim another Czech/light or live multi-DPI campaign, every error/timing combination, a new manual-restore test, real disk-full/VHD testing, SMB, SDK changes or release installation. Current-head cloud checks are inspected separately after the documentation push. Remaining release checks are tracked in the [readiness checklist](directory-sync-plugin-release-readiness-040.md).
