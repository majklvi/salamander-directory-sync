# Real low-space verification of buffered copying — 0.4.0

Verified on 2026-10-21, starting from commit `988727545b20343bd39b602ddc7fb6c6516ee408`. Version 0.4.0 remains in development. This campaign repeats the relevant real-volume cases after the transfer adapter changed from system copying to buffered main/ADS writes. Injected errors and the older real-VHD results are not substitutes for this check.

## Isolation and volume identity

All fixtures, private host configuration, observation probe and evidence are outside Git under `C:\Samandarin\builds\dirsync-vhd-040-20261021`. A new fixed 256 MiB VHD was created there. The user ran the guarded administrator preparation script and reported `READY: E:\dirsync-fixture`. The operator then independently checked the mounted image/partition/volume association:

| Property | Observed value |
| --- | --- |
| Image | `space-test.vhd`, fixed file length 268,435,968 bytes |
| Virtual capacity | 268,435,456 bytes |
| Associated device | `\\.\PhysicalDrive1` |
| Volume identity | `\\?\Volume{59d1e8d2-80a4-4035-b12d-f9a042745ddf}\` |
| Filesystem / label | NTFS / `DIRSYNC_SPACE` |
| Filesystem capacity | 251,588,608 bytes |
| Initial free space | 236,388,352 bytes |
| Free space before each failure case | 33,554,432 bytes (32 MiB) |

The control script checks the image path, attachment, capacity, partition/device mapping, volume identity, label, filesystem and test-root boundary before each action. It rejects reparse roots/reserves. It fills only its own non-sparse `fixture-control\space-reserve.bin` on this image, using actual writes; Release truncates only that reserve. The C: host disk is not filled. Sources and the recovery directory are outside the VHD. Existing user windows and data are preserved.

Computer Use operated a private English/dark Samandarin 0.16 x64 developer host under the ordinary user account. A fresh Release x64 plugin and both language modules were built through `tools/build.ps1`, with zero warnings/errors. Initial tested SPL SHA-256: `9c714831b34377c1fc86778ea72e7f3c8450c74d4cfbf8897e8f51c53bd1e7ff`. Keep originals was OFF, and every confirmed plan contained only three new rightward copies, without overwrites or deletions. A remembered 2 MiB/s limit made live progress observable.

## Main-data failure and a fresh plan

The source set contains `0-done.txt` (4 bytes plus an 8-byte named stream), `a-large.bin` (64 MiB plus an 8-byte named stream) and `z-next.txt` (4 bytes). Sources contain real data, with recorded hashes, attributes and modification times.

With **Stop at first error ON**, the first item completed. The large item then failed with genuine Windows error **112 — There is not enough space on the disk**. The result showed new copies **1/3**, one error, one unstarted item and zero skips/blocked operations. There was no interactive question in this policy mode.

Only `0-done.txt` existed in the target afterward. Neither the large final name, subsequent target nor staging remained. Free space returned to the original 32 MiB. After releasing the reserve, a fresh Compare marked the first item equal and confirmed only the two remaining copies. They completed **2/2** without errors. The first target's creation timestamp remained identical to the timestamp captured after failure; it was not replaced by this second command.

## ADS failure and interactive Retry

The second set has the same first/last items, while `a-large.bin` contains a 16 MiB main stream and a **64 MiB `漢字` named stream**. With **Stop at first error OFF**, the large item displayed an actual error-112 question after some data had been written. The first item remained complete; the third had not started.

Choosing **Retry** while the reserve still occupied the volume produced error 112 again. Cleanup again left only the completed first item and restored 32 MiB free. Releasing the reserve while the second question was pending, then choosing Retry, completed the current item and the subsequent item in the same confirmed command. The final result showed **3/3**, zero errors, zero unstarted items and zero execution skips or dependency blocks. The first item's creation timestamp remained unchanged across both retries.

The total byte counter includes actual writes from failed attempts; it is not the final payload size. ADS are not enumerated in advance for the confirmation estimate, so the initial main-size estimate is approximate. These limitations do not weaken full-stream verification before publication.

## Direct observation of partial writes

A separate native probe compiled the current `src/engine.cpp` and `src/copy.cpp` with the existing test observation hook. It did not compile the space-fault injector, replace Win32 calls or manufacture error codes. Two fresh target directories on the same image, each starting with 32 MiB free, provided precise staging-stream observations:

| Source | Last observed staging sizes before real error 112 |
| --- | --- |
| 64 MiB main-data case | Main stream 33,554,432 bytes |
| 16 MiB main + 64 MiB ADS case | Main stream 16,777,216 bytes; ADS 16,777,216 bytes |

Both probes recorded first item `copied`, current item `failed` with error 112, `published=false`, no remaining stage, and last item `not_started` with zero attempts. Free space after cleanup equalled free space before execution. These are native adapter observations, separate from the shipping-plugin GUI checks above.

## Small presentation correction

The real question exposed misleading context: a buffered target-write failure said **prepare or publish target**, although it occurred during data transfer. The error dialog now labels a target-site failure in the Copying phase **transfer data**, using the existing English/Czech strings. Source-read and target-preparation/publication labels remain distinct. This changes presentation only; phase/site classification, Skip all keys and retry safety are unchanged. Native dialog/channel regressions cover the transfer and preparation captions for copy, preservation and overwrite, retaining deletion and Stop/Close cases.

## Independent disk verification

The outside-Git `verify.py` passed **138 checks** before the presentation-only rebuild. It enumerated the complete stream sets and checked lengths/SHA-256, modification times and attributes for all six unchanged sources and all six completed GUI targets. It verified completed-first-item creation timestamps, exact successful/failed target inventories, probe outcomes and partial-stream observations, restored free space, empty recovery storage and a released reserve. Exclusive opens succeeded for every checked stream, confirming that those handles were no longer held. The verifier's .NET/FILETIME comparison explicitly accounts for their different epochs.

Evidence includes `baseline.json`, `campaign-state.json`, `volume-ready.json`, the main/ADS comparison/confirmation/error/result accessibility records, first-failure inventories, volume/free-space JSON snapshots, `probe-main-log.txt`, `probe-ads-log.txt` and `verification.json`. An intermediate accessibility capture can lag visible progress; exact native partial-write counts above come from the probe, not from a guessed screenshot time.

## Final build and regression checks

The presentation-only rebuild produced a 788,480-byte Release x64 SPL with SHA-256 `4f5c43f4958577b1ca47eaa2b19bd308a202d4f8c70c9579731a31094bd44916`. Computer Use loaded it in the restarted private host and copied the ADS set into a third fresh target with 32 MiB free. The real error-112 question now visibly said **Copy new item — transfer data**. Cancel from that question ended the batch with one completed item, one cancelled item and one unstarted item. Only the first target remained, with correct data/ADS; no staging or later target remained. Free space returned to 32 MiB before the reserve was released.

The final verifier passed **149 checks**, including all previous disk checks plus the final-build target, complete stream verification, exclusive opens and corrected live caption. The final mounted snapshot reported 85,393,408 bytes free and a zero-length reserve. Both private host instances were closed normally; the two existing installed-host windows were left open.

All five x64 suites passed through the repository test script in **Debug and Release**, per configuration:

- Engine: 2,216 assertions.
- Copy/recovery/overwrite/delete: 10,612 assertions.
- Lifecycle: 2,451 checks, including native EN/CS operation questions and their existing Stop/Close/unload cases.
- English resources/layout: 6,166 checks.
- Czech resources/layout: 6,267 checks.

Builds used the repository build script with `OutputRoot=out/vhd-040/` to avoid other live hosts' locked default outputs. Fresh test outputs were staged into the standard `out/tests` paths for `tools/test.ps1`. Debug Win32/x64/ARM64 and Release x64 plugin/language builds passed with zero warnings/errors. ARM64 is compilation only. Resource metrics cover 96–240 DPI; this campaign's live visual check is English/dark, not a new live Czech/light or monitor-DPI matrix.

After all test I/O and private hosts had ended, the user detached the image from an administrator PowerShell on 2026-10-22. An independent `Get-DiskImage` check confirmed **Attached=false**, no device path and the unchanged fixed image length of 268,435,968 bytes. The image and outside-Git evidence are retained, not deleted. Published-head CI is checked separately after pushing this report and the caption fix.

## Coverage limits

This is local NTFS x64 coverage of new-file copying, real error 112, stop-on-first-error, fresh-plan completion, interactive Retry with and without newly available space, main/ADS cleanup and publication verification. It does not newly test error 39, disk removal, crash durability, recovery-volume exhaustion, every metadata-allocation boundary, overwrites/deletions under low space, SMB or another filesystem. No SDK change or release publication is involved.
