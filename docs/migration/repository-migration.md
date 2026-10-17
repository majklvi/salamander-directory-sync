# Oddělení samostatného vývoje — 2026-10-17

Vývoj Directory Synchronization 0.1.4 pokračuje v [samostatném veřejném repozitáři](https://github.com/majklvi/salamander-directory-sync), výchozí větev `main`. Původní [fork](https://github.com/majklvi/salamander/tree/feature/directory-sync-plugin) zůstává historickým archivem; jeho historie se nepřepisuje ani nemaže.

## Historie a původ

Export vychází z `ac4b64fe4ab96838c28ed36a93de03dc718d7d62`, nad referencí `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`. Zachovává všech 67 vlastních commitů pluginu včetně autorů a časů. [Mapa SHA](commit-map.tsv) propojuje původní a exportované commity; [manifest](source-manifest.json) uvádí rozsah a identitu projektu. Přípravné commity samostatného sestavení na export navazují.

Očištění historických textů a názvů proběhlo pouze v izolovaném exportu. Úplný audit dosažitelných commitů, stromů, cest a textových i binárních blobů je podmínkou publikace. Nejde o OCR ani dekompilaci. Původní fork a jeho kompletní záložní bundle zůstávají mimo publikovaný export. Pravidla očištění s původními hledanými výrazy se nepřenášejí.

Samostatné sestavení obsahuje připnutý snapshot 52 potřebných podpůrných souborů. Licence, copyrighty a původ závislostí zůstávají zachované; soupis je v kořenových souborech `EXPORT-SUPPORT.json`, `EXPORT-ATTRIBUTIONS.md` a v `docs/third_party.md`. SDK se funkčně nemění.

## Rozsah změny

Vlastní MSBuild projekty, skripty `tools/build.ps1`, `test.ps1`, `package.ps1` a dva workflow nahrazují závislost na okolním checkoutu hostitele. Změny pluginových zdrojů pro sestavení jsou omezeny na cesty include a webovou adresu projektu. Verze 0.1.4, identita `DirectorySync`, názvy SPL/SLG, konfigurace i formát obnovy zůstávají zachované.

CI sestavuje Debug Win32/x64/ARM64 a Release x64, spouští pět x64 sad v Debug i Release, vytváří ověřený balíček a samostatně analyzuje C/C++ pomocí CodeQL. Stav konkrétní publikované revize je ověřitelný v [Actions](https://github.com/majklvi/salamander-directory-sync/actions); lokální kontrola sama o sobě není výsledkem cloudové analýzy.

Historické protokoly jsou datované záznamy, nikoli současný návod. Pro instalaci a běžné použití platí aktuální návod a pilotní pokyny. Testy SMB nejsou součástí tohoto přesunu. Nevzniká nová verze ani automaticky publikované vydání.
