# Plugin Manager metadata

2026-10-18. Inspection and update against the installed Samandarin 0.15.1 x64 host, with the existing SDK unchanged.

## Changes

- Copyright includes **Michal Várady**, retaining the Open Salamander Authors credit.
- Short descriptions: **Directory comparison and synchronization.** / **Porovnávání a synchronizace adresářů.**
- Web points to `https://github.com/majklvi/salamander-directory-sync`, using the existing `SetPluginHomePageURL` API.
- About displays the localized description, copyright/name and project URL.
- PE file properties identify Michal Várady and describe the actual synchronization plugin, replacing the obsolete “preview plugin” description. The SPL resource explicitly uses UTF-8 so the name survives resource compilation.
- Future package notices include both copyright credits. The published 0.1.6 release is not replaced by this local metadata update.

## Other fields inspected

| Manager field | Interpretation and follow-up |
| --- | --- |
| Version / location / loaded | Already provided by registration and the host. |
| Archives | Not applicable: the plugin does not implement archive formats. |
| File system | Not applicable: it is not a virtual file-system provider. |
| Thumbnails | Not applicable: it does not provide thumbnails. |
| Functions | Menu extension is the correct existing classification. |
| Network, external processes, scripts, active web, elevation | The host reads declarations from its central `plugin-capabilities.json` or package metadata, not from this plugin entry-point SDK. A future catalog integration can supply an audited declaration. Network must account for eventual UNC/SMB path access; it must not be marked “no” merely because this pilot tests NTFS. |
| Signing level / signer | Host verification of a real signature or trusted installer receipt. This unsigned plugin cannot fill these with an author-name string. |
| Package SHA-256 / source | Host installer receipts (`plugin-receipts.json`) or bundled metadata. The ZIP already ships checksums and source-revision information, but manual SPL registration does not create an installation receipt. Catalog/installer integration is separate future work. |
| Updates | Host catalog integration is separate from setting the homepage URL; no automatic update support is claimed. |

Inspection used the local host sources `src/dialogs5.cpp`, `src/pluginsecurity.cpp` and the Plugin Manager dialog resource. No central capability/receipt files, signing setup or SDK was modified.

## Validation

The Czech Plugin Manager displayed the complete short description, **Michal Várady** with correct accents, retained copyright credit and the project URL without clipping. About also displayed the description, copyright and complete URL in a compact native message box. Both descriptions are measured against the host's 273-DLU description field in the existing English/Czech resource suites at 96, 120, 144, 192 and 240 DPI; this is automated measurement, not a claim of live English or multi-DPI host testing.

All five x64 suites passed in Debug and Release. English/Czech resource suites now include 2,477 / 2,514 checks. The installed local binary and both language modules were rebuilt; file properties preserved the author name and accents in all three modules. No synchronization or deletion operation was run in the live host. No SMB tests or SDK changes.
