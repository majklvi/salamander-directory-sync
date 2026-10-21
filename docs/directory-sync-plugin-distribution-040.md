# 0.4.0 x64 distribution: clean installation and update from 0.3.0

Verified on 2026-10-22, using the executable sources at `c96ff6cbf4b1267d71437b9f770bef46cdabe705`. This task prepares the final local distribution candidate; it does not publish a release or tag. The public release remains 0.3.0 until publication is requested.

> [!CAUTION]
> **ALPHA SOFTWARE — RISK OF DATA LOSS. NOT FOR PRODUCTION USE.**
> No warranty is provided. Authors and contributors accept no responsibility or liability for loss or corruption of data. Use disposable copies and an independent verified backup. Recovery copies are not automatic rollback.

## Environment and package

Computer Use operated two independent file-backed configurations under `C:\Samandarin\builds\dirsync-040-install-20261022`: `clean/host` and `upgrade/host`. Both use the existing custom Samandarin 0.16 x64 developer host, numeric version 5.0.0.197, SHA-256 `f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd`. Runtime DLLs and matching host English/Czech language modules accompany the host. Existing installed-host windows were retained.

This is local NTFS, the existing system DPI and dark appearance. The clean configuration began as a zero-byte `config.reg`, with no plugin configuration or registration/catalog entries in `configstorage.ini`. Only the unpacked plugin files were supplied. The host discovered the plugin automatically. This verifies unpack-and-discovery registration, not a new manual Add-dialog or signed-installer path. Updating replaces the complete payload while the private host is stopped; registration is not removed/re-added.

The fresh Release x64 build used `tools/build.ps1`; `tools/package.ps1` checked all three modules for x64 PE type and **0.4.0 (x64)** version, then extracted the ZIP again and verified hashes and exact inventory. The package has **15 files**: SPL, English/Czech SLGs, four user guides, notice, licenses and `files.json`. It contains no `docs` directory, SDK, compiler, PDB or import library. Installation requires no compilation. Alpha/data-loss warnings remain in all four guides and NOTICE.

| GUI-tested module | Bytes | SHA-256 |
| --- | ---: | --- |
| `dirsync.spl` | 788480 | `eb4eacf7c3bc5f7b1e033b7a4b3eb081c99abdf1c8737c8373f3bc625a9ae645` |
| `lang/english.slg` | 56832 | `bde4a3e3f76db22161025f4bb24bb20127af9e8f08007f65fa2c83024bc078b0` |
| `lang/czech.slg` | 57344 | `30df5fd1945b0bca01812951a1bc790162ff0bc75076864c8c7d53d6c592ef4a` |

The initial GUI-test archive has SHA-256 `1a3b6bf44ffa3b21c734f92ad6be5392ccbd2e2856ebd4af9f2ee20b46ccd8a3`. It contains the earlier guide text and is retained only as test evidence. The final archive is generated after committing the corrected guides and this report, at `out/releases/0.4.0-20261022-final/dirsync-0.4.0-x64.zip`. Its adjacent `package-sha256.txt` identifies the deliverable without a self-referential checksum commit. `NOTICE.txt` pins its source commit. The three final modules must match the GUI-tested hashes above; both stopped test installations are then brought to the complete final extraction and reverified.

The packaging script conservatively marks the working tree as modified when the unrelated untracked historical-analysis documents are present. Those documents are not package inputs and are not included or committed by this task. All packaged guide/build inputs are tracked in the recorded source commit; final module hashes are checked against the tested build.

## Clean installation and Czech restart

The first empty-configuration startup showed the developer host's missing-`salmon.exe` warning and blank panels. After normal exit/restart the panels populated. This repeats the host behavior recorded in the earlier distribution test; it did not prevent automatic plugin registration. Only the private host warnings were acknowledged.

- English Plugins Manager showed one registered plugin, version **0.4.0**, the short description, Michal Várady attribution and repository URL. Configure loaded the plugin successfully.
- English Preferences had **Keep originals OFF**, an empty disabled recovery path, **Stop at first error OFF**, and relative sizes/times and row coloring OFF. These are new-configuration defaults, not changes forced onto existing saved choices.
- Enabled **Color row text** and **Stop at first error**. Compared the isolated `left-žluťoučký` and `right-漢字` directories: one rightward new-file proposal, no blocked rows. No synchronization started.
- Created and selected **Čistá 0.4.0 – 漢字** through Manage profiles → Save as. Closed the preview and host normally.
- Changed only the stopped private host's Language value to `czech.slg`, preserving UTF-8 configuration encoding. After restart, the existing registration loaded the Czech plugin. Its selected profile, complete Unicode roots, mask `*.*`, Subfolders and all five filters returned; the initial list was empty and no work started automatically.
- Czech Compare produced the same single proposal. Czech Preferences retained **Barvit text řádků ON**, **Zastavit při první chybě ON**, recovery OFF and the empty recovery path. The Cancel button reads **Zrušit**. Closed normally without changing settings.

Independent parsing found all **17 plugin name/value pairs identical** before and after the Czech restart, including the selected profile and shared Unicode directory history. The one-profile `ProfilesV1` SHA-256 is `0db32a40fec4c16171f0bd24779fe7317eb42eb253911562f88018392b5b6359`.

## In-place update from the published 0.3.0 package

The old package is the published [`dirsync-0.3.0-x64.zip`](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.3.0/dirsync-0.3.0-x64.zip), SHA-256 `64d266aa37e250ff0778f33decd1c8aa0c59aaa48e9a1477d65c43b46be671b8`, independently matched to the GitHub release asset digest.

A synthetic configuration contains two profiles, an existing custom dark color, both complete palette arrays, action labels, relative sizes/times, columns/sorting and recovery ON with a dedicated Unicode recovery directory. The actual published 0.3.0 modules loaded it in English Preferences, enabled row coloring and saved it. Plugins Manager showed **loaded: Yes, version 0.3.0** before normal shutdown and the baseline snapshot. The baseline is therefore saved by 0.3.0, not fabricated by a newer plugin.

| Preserved profile | Mask | Options | Filters |
| --- | --- | --- | --- |
| `Pilot A – žluťoučký 漢字 😀` | `*.*` | Subfolders | all five ON |
| `Pilot B – přejmenováno & 漢字` | `*.txt;*.md` | Subfolders + By content | all five ON |

Profile B, ID 2, remains selected. Both profiles refer only to this campaign's synthetic directories. The new complete plugin payload replaced the old one while the host was stopped. Neither configuration nor recovery data was removed.

On the first 0.4.0 start in Czech, profile B and its mask/options returned without automatic work. Compare found one equal `alpha.txt` row with binary-content reason. Preferences retained recovery ON and the complete Unicode path, row coloring/action labels ON, relative sizes/times ON and the custom colors. The new Stop at first error preference was OFF. The profile menu and manager showed both profiles; inspecting A displayed its own roots, mask and options. Cancel retained active profile B.

After another normal exit, only the private host language changed to English while stopped. The English restart loaded profile B again without work. Compare and Preferences showed the same results and choices. Normal exit persisted version 0.4.0 in the existing registration/catalog; there was still exactly one plugin entry. Cached unloaded menu names can remain in the previous language until the new plugin loads, as in the earlier distribution campaign.

All **13 old name/value pairs** exactly match the 0.3.0 baseline after both Czech and English starts: KeepOriginals, RecoveryRootUtf16, ActionLabels, ColorRowsV1, RelativeSizesV1, RelativeTimesV1, SideNamesV1, VisibleColumnsV1, SortColumnV1, SortDescendingV1, LightColorsV1, DarkColorsV1 and ProfilesV1. The two-profile binary SHA-256 remains `ecd09e7b1414b9c0314946f2ad434ef884e9a3e9fb259c864c658a5bbe0f4e6c`.

Version 0.4.0 adds four values: StopOnFirstErrorV1 (OFF), TransferSpeedV1 (unlimited), LastTransferSpeedV1 (1 MiB/s suggestion) and DirectoryHistoryUtf16V1 (the two completed-comparison roots). All **17 resulting values** are identical across the Czech/English restart. Since 0.3.0 had no saved error-policy switch, upgrading intentionally adopts the new interactive default; the guide explicitly explains how to restore immediate stopping.

## Independent checks, documentation and limits

The outside-Git `verify.py` passed **86 checks**: published-asset identity, exact installed package inventories/hashes, registration after restarts, complete old/new settings comparisons, profile decoding and selected IDs, complete Unicode history, unchanged source hashes and empty unused right/recovery directories. Evidence includes the three upgrade configuration snapshots, two clean snapshots, baseline/verification JSON and GUI accessibility records. Both private hosts ended normally; existing user instances remain open.

No production or SDK change was required. The four current guides now identify the 0.4.0 candidate and explain migration/defaults; the short EN/CS guides no longer claim that empty-directory copying is unsupported, and describe the main Synchronize button versus its arrow and Details correctly. The repository README download links and changelog release date remain at the published 0.3.0 until a separate publication request.

The same executable sources already passed all five x64 suites in Debug/Release and the Debug Win32/x64/ARM64 + Release x64 build matrix in the preceding real-VHD checkpoint; ARM64 is compile-only. This task freshly builds Release x64 and checks the new published-head CI separately. The guide/report edits do not require another identical local runtime sweep; current-head CI rebuilds/tests/packages the full matrix.

This is distribution/configuration/localization acceptance, not a new destructive-operation, SMB, light-mode, monitor-DPI, registry-storage, another-account or other-host-version campaign. GUI comparison never synchronized files. Existing local NTFS operation, error, Stop/Close/unload and real-low-space reports retain their separate coverage. The final alpha package is ready for a publication decision, not a guarantee against data loss.
