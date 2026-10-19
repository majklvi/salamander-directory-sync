# Stored-originals terminology

Decision and analysis, 2026-10-19, for the 0.3.0 development UI.

## What the record represents

One recovery manifest and its optional overwrite/deletion journal describe one attempt to preserve originals. It can be complete, incomplete, cancelled or damaged. It is not a saved synchronization profile, the lifetime of an open plugin window, or proof of a completed synchronization. A combined synchronization can create separate records for its overwrite and deletion phases.

| Term | Advantages | Possible misunderstanding |
| --- | --- | --- |
| Run / běh | Technically precise for one execution attempt; matches existing internal identifiers. | Czech **běh** sounds like implementation terminology rather than a user-facing group of records. |
| Session / relace | Compact, familiar grouping of related records; suitable for a list and its context menu. | Needs a definition here: it does not mean the entire time the window is open. |
| Sync session / synchronizační relace | Explicitly refers to the plugin's main purpose. | Too broad: a record may come from original preservation alone or a single destructive phase, and one combined synchronization can produce multiple records. |
| Recovery session / relace uchování originálů | Clarifies the scope when used outside the Stored originals dialog. | Must not imply that restoration was executed; the browser only reads preservation records. |
| Backup / záloha | Familiar to users. | Can imply a complete backup job, a retention policy or automatic restore, which this feature does not provide. |

## Chosen UI vocabulary

Use **Sessions / Relace** inside **Stored originals / Uložené originály**, including the list heading, column heading, counts, empty state and context-menu commands. In explanatory prose use **recovery sessions / relace uchování originálů** and define the scope above. Continue showing explicit copy availability and operation-result states; a session does not imply success.

This is a presentation change. Preserve the `run-*` directory layout, `runDirectory` and `event: run` schema fields, resource IDs such as `IDS_STORED_RUN`, and the internal `{runs}` formatting token. Do not rename existing recovery directories, rewrite manifests or migrate old data. Dated verification reports retain their original wording and screenshots as historical evidence; current user guides use the new terms. The English verb in “Runs visible new copies…” remains a verb, not the name of a recovery record.

The change is deliberately scoped to the recovery browser. Technical descriptions of an execution run elsewhere are not aliases for a stored-originals session.
