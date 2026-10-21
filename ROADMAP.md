# Development roadmap

Updated 2026-10-21, after release 0.3.0. This separates completed development stages from planned directions; it is not a release commitment. Error-handling decisions R1–R5 are approved.

The current scope remains local NTFS with the existing SDK. SMB testing and SDK changes are excluded. Keep the reliability and compact native UI at the level of the host's ordinary file operations.

## 1. Interactive error handling — 0.4.0 development

Allow **Retry / Skip / Skip all / Cancel** for recoverable errors, such as a locked file or temporarily denied access, instead of always ending the entire batch. Retain the current stop-on-first-error behavior as an available policy.

Selected for research and discussion on 2026-10-21. Direct reference-GUI tests, review of the existing executor, approved decisions and the proposed implementation sequence are recorded in [interactive operation errors](docs/directory-sync-plugin-error-handling.md). R1–R5 were approved by the user on 2026-10-21:

- **R1:** interactive questions by default, with a remembered global setting to stop on the first error.
- **R2:** Skip all applies to matching operation/phase/error classes in the current batch only; no persistence or suppression of fatal failures.
- **R3:** continue independent confirmed operations after Skip, including later deletions, preserving phase order, plan validation and protection of retained children and their ancestors.
- **R4:** allow per-item decisions for recoverable saving failures, skipping the dependent overwrite/deletion as well; shared recovery-manifest or operation-journal write failures stop the entire batch.
- **R5:** space shortage before changing the target can offer item decisions; verification mismatch, invalidated plan, post-publication failure, uncertain removal and shared-protocol write failures stop the batch. No automatic rollback or repeating already completed operations.

**Stages 1–2 implemented:** common decisions/outcomes, remembered global policy, and cancellable EN/CS questions for the separate new-item copy command. [Implementation and tests](docs/directory-sync-plugin-errors-040.md). **Recovery/overwrite part of stage 3 is implemented:** row/side mapping, per-item original preservation, dependency-blocked overwrites and fatal shared-protocol handling. [Follow-up and tests](docs/directory-sync-plugin-original-errors-040.md). **Stage 3 is complete:** interactive asymmetric deletion now includes original-save dependencies and protection of all retained-child ancestors. [Deletion verification](docs/directory-sync-plugin-delete-errors-040.md). **Stage 4 is implemented:** combined synchronization continues independent confirmed operations and later phases after Skip, with per-command Skip all and fatal/cancellation boundaries. [Combined verification](docs/directory-sync-plugin-combined-errors-040.md). Native results and exports distinguish execution skips, blocked dependencies and unstarted work.

Implementation requirements:

- Which errors permit Retry or Skip, and which must stop the batch.
- A compact English/Czech error dialog showing the affected operation, paths and system error.
- Revalidation before retrying; a changed source or target must not be silently accepted.
- Dependencies after Skip: skipping a child must protect its parent from deletion, and later phases must not assume an unsuccessful prerequisite completed.
- Truthful summaries and exports distinguishing completed, skipped, failed and unstarted operations. Completed changes remain; there is no automatic rollback.

**Live combined-command verification completed:** six isolated local NTFS GUI cases cover Retry/Skip/Skip all, independent phase continuation and protected parents, plus Stop/Close while awaiting item decisions with recovery ON/OFF. Actual disk state, ADS, timestamps, saved originals and journals agree with the results; no code fix was needed. See the [live GUI report](docs/directory-sync-plugin-combined-errors-gui-040.md) for the precise boundaries. Active-I/O/Verify and further deletion-decision cancellation timings retain automated coverage; they were not additional live cases in this campaign.

The design replaces first-error-only behavior incrementally. Broader failure scenarios and release publication require separate requests.

**Focused native-dialog acceptance completed:** the common error question follows the host's basic file-error layout, retaining complete Unicode paths and the cancellable modeless channel. Four private EN/CS light/dark GUI cases cover plain confirmation/result captions, keyboard use and shutdown/forced unload with an unanswered question. Both Manager cases actually unmap SPL/SLG and reload successfully in the same host PID. See the [source comparison and live report](docs/directory-sync-plugin-native-errors-gui-040.md).

The [0.4.0 release checklist](docs/directory-sync-plugin-release-readiness-040.md) records the remaining report-consumer and distribution checks. Next: inspect mixed outcomes in CSV/text export and Stored originals, then final package installation/update checks. Restoration and large-list optimization remain separate work below.

## Compact synchronization confirmation — 0.4.0 development

Reference GUI research on 2026-10-21 found independent direction choices based on eligible visible proposals, including zero-count disabling and separate asymmetric deletion. Unequal file counts/bytes did not select a dominant direction. See the [observations and proposed compact confirmation](docs/directory-sync-plugin-sync-confirmation-design.md).

Propose a compact native confirmation with left-to-right/right-to-left choices, explicit deletion and recovery status, and **Details…** opening the existing tabular summary read-only. Direction choices restrict existing proposals; they do not reverse them. Confirmation, execution, progress and exports must use the same selected plan. Approved by the user and implemented on 2026-10-21. See the [implementation and verification](docs/directory-sync-plugin-sync-confirmation-040.md). No SMB tests or SDK changes are included.

## 2. Restore an individual stored original

Extend the existing read-only Stored originals browser with explicit restoration of one selected item. Start with a target preview, validation of the stored copy and an explicit collision decision. Preserve the recovery source and report the actual result.

Agree on destination and collision rules before implementation. Batch restoration, automatic restoration and backup cleanup remain outside this first stage.

## 3. Improve large-list responsiveness based on measurements

Selected as the next area to analyze before implementation (2026-10-20). Local baseline research identifies native-list rebuilding and report-finalization matching as bottlenecks. No optimization is implemented yet; measured evidence must accompany that future change.

Measure filtering, sorting, repainting, memory use and CSV/text export on large isolated local sets. Separate normal interactive behavior from UI Automation overhead; automation delays alone do not establish a product performance defect.

Optimize demonstrated bottlenecks, preserving grouping, selection, filters, Skip and export semantics. Avoid broad refactoring or repeating the full historical verification campaign without a concrete reason.

## Deferred host integration

Shared host transfer windows/queues and additional host configuration APIs remain conditional on an appropriate released SDK. See the [design record](docs/directory-sync-plugin-design.md) for the existing long-term integration plans; this roadmap does not authorize SDK changes.
