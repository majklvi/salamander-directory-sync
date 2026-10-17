# Použité komponenty a zdroje

SDK a společná podpora: Open Salamander / Samandarin contributors. Původní copyrightové a licenční hlavičky jsou zachované v `vendor/`; přesné vstupní a exportované hashe uvádí `EXPORT-SUPPORT.json`.

Knihovna vzhledu: darkmodelib od ozone10 a přispěvatelů. Licence MPL-2.0, MIT a licence dílčích komponent jsou přibalené v adresáři knihovny a v balíčku pod `licenses/`.

Referenční správce souborů a jeho komunita: podklady pro chování a použitelnost; pouze výzkum, bez převzetí kódu, grafiky nebo nápovědy. Původní podklady a autorství zachovává [archiv forku](https://github.com/majklvi/salamander/blob/af0e81bc1bccb6c733025fe31a97d53adf5cb32f/doc/third_party.md).

Sestavení nepotřebuje žádný externí repozitář za běhu build skriptů. GitHub runner a CodeQL vycházejí z [oficiálního seznamu runnerů](https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-VS2026-Readme.md) a [postupu pro kompilované jazyky](https://docs.github.com/en/code-security/how-tos/find-and-fix-code-vulnerabilities/manage-your-configuration/codeql-for-compiled-languages).

Místní kontrola workflow: [actionlint](https://github.com/rhysd/actionlint) od rhysd a přispěvatelů, verze 1.7.12. Pouze externí validační nástroj; do pluginu se nepřibaluje.
