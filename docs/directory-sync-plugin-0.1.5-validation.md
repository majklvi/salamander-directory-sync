# Version 0.1.5: appearance and operation help

18 October 2026. Local x64 validation; no SMB tests or SDK changes.

## Changes

- Preferences adds **Color row text / Barvit text řádků**, off by default. It uses the configured operation palette, preserves Skip fading and native selection colors, and is remembered through the existing configuration API. OK applies it without repeating Compare; Cancel discards it.
- The left status pane explains the highlighted synchronization command, including the visible-plan scope and the current recovery choice for overwrite/delete. The same mechanism covers the context menu's Visible plan submenu. Existing SDK selection queries are polled only while the menu is open; closing it stops the timer and restores the previous status. No host interface was added.
- Plugin registration, SPL/SLG version resources, package verification and the CI package artifact use 0.1.5. README download links continue to target the existing public 0.1.4 release, explicitly distinguished from source version 0.1.5.

## Automated validation

Plugin and both language modules build locally for x64 Debug and Release with warnings treated as errors. All five suites pass in both configurations:

| Suite | Checks per configuration |
| --- | ---: |
| Engine | 542 |
| Copy/recovery/overwrite/delete | 3729 |
| Lifecycle and native control probes | 2366 |
| English resources/layout | 2467 |
| Czech resources/layout | 2504 |

Coverage includes both palettes, coloring enabled/disabled, Skip, copy directions, delete and blocked proposals; configuration round trips and invalid values; localized menu explanations; compiled dialog measurements at 96/120/144/192/240 DPI. These measurements are not live monitor-DPI tests.

## GUI evidence

The changes were exercised before the version-only bump in the installed x64 host identified as Samandarin 0.15.1. The isolated local NTFS fixture produced 185 visible rows and six blocked entries, with content comparison and empty directories enabled. Row text used the operation colors, and skipped rows remained gray.

Keyboard selection in Synchronize displayed distinct copy and overwrite descriptions; overwrite correctly reported recovery OFF. Escape restored Comparison complete and retained the separate row-count panes. The results were left open. No synchronization command was executed. Live mouse-hover selection, the context submenu and the Czech menu explanations were not separately verified in this GUI pass; both language resources were checked by the automated suites.

GitHub CI results belong to the pushed commit and must be checked separately; local success does not establish cloud build or CodeQL success.
