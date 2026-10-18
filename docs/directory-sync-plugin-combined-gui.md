# Combined synchronization GUI verification — 2026-10-19

## Scope and environment

Live Computer Use checks of the released 0.2.1 x64 plugin in an isolated Czech/dark Samandarin 0.16 developer host. Source revision: `4eed0f502dd04d3d1479d832e3019cbf9c58f7b4`; SPL SHA-256: `0b5eea61ee6d679322f567fdca8db318877e851b413bd7489258f5e0301c0385`. The host and configuration are private copies under `C:\Samandarin\builds\dirsync-combined-gui-20261019\host`. The installed user instance is outside the test scope.

All fixtures are synthetic, outside Git, on a volume independently identified as NTFS. No SMB tests, SDK changes, artificial worker delays or custom test-hook plugin builds. Subdirectories, Asymmetric and Empty directories are enabled, mask `*.*`, and all result filters are enabled. Execution uses all visible eligible proposals without selecting rows. Each destructive GUI batch receives its own action-time user confirmation.

The minimal developer host reports its known missing `salmon.exe` crash-reporter warning at startup; this was dismissed. Fixture modification times were set after writing ADS, since writing an ADS updated the containing file's modification time. The corrected fixture was compared again before execution; the initial equal-time preview was not executed.

## Completed mixed batches

Separate `on` and `off` fixtures contain one new Unicode-named file with an ADS, one new physically empty Unicode-named directory, one newer source file replacing its right counterpart, and a right-only `delete-tree` containing `delete.txt` and `empty-leaf`. Both sides also contain an equal `keep.txt`.

| Check | Recovery ON | Recovery OFF |
| --- | --- | --- |
| New copies completed/planned | 2/2 (one file, one directory) | 2/2 (one file, one directory) |
| Overwrites completed/planned | 1/1 | 1/1 |
| Deletions completed/planned | 3/3 (one file, two directories) | 3/3 (one file, two directories) |
| Errors/cancelled items | 0 | 0 |
| Unstarted operations | 0 | 0 |
| Other omitted visible proposals | 1 (equal control file) | 1 (equal control file) |
| Recovery | Four originals saved, two complete phase manifests | No new recovery files; previous ON files unchanged |

Each batch used one shared confirmation and produced one combined result, without secondary phase confirmations. Confirmation counts and the remembered recovery mode matched the actual plan. The ON confirmation reported 31 main-data bytes copied/overwritten and 29 original main-data bytes; OFF reported 33 and 31 respectively. ADS bytes are explicitly excluded from these confirmation totals.

Independent post-run reads checked new/overwritten file data, ADS and modification times against the sources; the new empty directory exists and is physically empty. The right-only deletion tree is absent. Source files and both equal control files retained their before-run hashes and modification times.

With ON, both manifests end with `complete`. The original overwrite target and deleted file retain their original main data and test ADS in recovery, and both directory records point to existing saved directories. `overwrite.jsonl` records the replacement; `delete.jsonl` records the file, deepest empty directory and parent deletions, in that order. The combined result lists both phase manifests and execution journals. With OFF, the result exposes no new recovery manifest or execution journal; a path/hash inventory of the existing recovery tree is unchanged.

## Stop during a mixed batch

The final `stop-compact` fixture has eight result rows: three new files (`0-done.txt`, `a-large.bin`, `z-next.txt`), an overwrite, a right-only file and two directory deletions, plus the equal control file. The large source contains 1.75 GiB of actual data (1,879,048,192 bytes). Recovery is ON. Only ordinary production copying and verification run.

After confirmation, a screenshot showed item 2 of 3 new copies, **759,169,055 copied bytes**, **62 verified bytes** and **one completed item**. Thus the large file was still being copied, with the first small file and its ADS already verified. Alt+Z invoked the synchronization Stop control.

The combined result reported **1/3 new copies**, **0/1 overwrites**, **0/3 deletions**, **one cancelled item** (Windows error 1223) and **five unstarted operations**. It required a new comparison and stated that completed new items remain. The complete key-and-result-snapshot roundtrip measured 1,719 ms; this is a single automation measurement, not a device-independent cancellation bound.

Independent inspection found only the completed `0-done.txt` and its correct ADS added to the destination. The large target and `z-next.txt` are absent. The overwrite target still contains its old main data and ADS, and the entire deletion tree remains. No staging residue remains at the destination. The UI returned from the result dialog to an idle preview.

## Close during a mixed batch

The separate `close-compact` fixture has the same eight-row plan, with recovery OFF. To avoid duplicating 1.75 GiB of source data, its large source is a second hardlink to the same synthetic file used by the Stop fixture; destinations and control items are separate. The actual destination copy uses the ordinary adapter, not a hardlink.

After confirmation, the screenshot showed item 2 of 3 new copies, **649,068,576 copied bytes**, **64 verified bytes** and **one completed item**. Clicking the preview's Close button during this transfer returned directly to the host, with no end-of-run result dialog or unsaved-profile prompt (the task is unnamed). The click-and-host-snapshot roundtrip measured 911 ms, again a single automation observation rather than an internal cancel/join timing guarantee.

Independent inspection confirmed the completed small file and its ADS remain, the large target and next new file are absent, the old overwrite data/ADS remain, and the full deletion tree and equal control file remain. No destination staging residue or new recovery files were found; existing recovery files retained their hashes. All 1,792 one-MiB source blocks matched the preparation block hash after both cancellation tests. An exclusive source-file open succeeded after Close, providing an additional handle-release check. The test host was then asked to exit normally; the installed user instance was left running.

These four live checks complete the ordinary mixed-batch ON/OFF and active-transfer Stop/Close coverage in this report. They do not test a live Stop inside Verify, during overwrite/deletion, or every recovery mode crossed with every cancellation point; automated NTFS tests continue to cover phase-boundary failures/cancellation. No functional defect was demonstrated in these final small-row GUI cases, and no production code change was required.

## Large-list automation limitation

An initial timing fixture used 6,000 new 32 KiB files, with overwrite/deletion sentinels: 6,005 rows overall. Its comparison completed, but subsequent Computer Use menu/click/key operations suffered long delays and timeouts, including screenshot-only requests. A diagnostic stack sample caught the main thread in `NtUserWaitMessage`/`IsDialogMessage`, rather than in a copy routine. Other threads included UI Automation. This single sample does not establish the cause or prove a continuously blocked UI thread.

No synchronization was confirmed for that fixture. After failed ordinary close inputs, only that isolated idle host process was terminated, its path and absence of the bulk destination checked first. This is test-environment recovery, **not** evidence of graceful Close or forced-unload behavior during a transfer. A fresh host and the small-row/large-file fixture were used for the actual cancellation check above. No speculative accessibility, SDK or file-operation changes were made. The broader large-list accessibility/automation issue remains separate from the successful small-row workflow, consistent with the earlier [UI Automation investigation](directory-sync-plugin-ui-automation.md).

## Local evidence

The fixture root contains `fixture.json`, before-run inventories for ON/OFF, their `gui-result.txt`/`verification.json` files, phase manifests/journals under `recovery-漢字`, the Stop result and independent verification under `stop-compact`, Close evidence under `close-compact`, `large-source-verification.json`, and `large-list-stack.txt`. These local synthetic artifacts are not distributed or committed. Historical 0.2.1 implementation records remain dated evidence; this report extends their live-GUI coverage.
