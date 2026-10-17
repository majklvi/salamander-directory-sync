# Directory Synchronization 0.1.4

Samostatný nativní plugin pro Samandarin 0.16. Porovnání adresářů, úprava návrhů, kopírování, přepisy a asymetrické odstranění s volitelným uchováním originálů. Český a anglický modul, vzhled podle hostitele. SDK se nerozšiřuje.

Samostatný vývoj a hlášení chyb probíhají v [majklvi/salamander-directory-sync](https://github.com/majklvi/salamander-directory-sync). Zdrojový archiv a původ závislostí jsou uvedeny v `EXPORT-ATTRIBUTIONS.md` a `EXPORT-SUPPORT.json`. Historické protokoly v `docs/` popisují své tehdejší etapy, nikoli dnešní omezení.

## Sestavení a testy

Windows, Visual Studio 2026 s C++ desktop tools (v145, potřebné x86/x64/ARM64 komponenty), Windows SDK 10 a PowerShell 7.3 nebo novější. Není potřeba checkout ani knihovna Salamanderu, Python, vcpkg, certifikát nebo síťové stahování závislostí. MSBuild se vyhledává pomocí systémového vswhere.

```powershell
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Platform x64
pwsh -NoProfile -File tools/build.ps1 -Configuration Debug -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Debug
pwsh -NoProfile -File tools/package.ps1 -OutputDirectory out/package-01
```

`-Configuration Debug/Release`, `-Platform Win32/x64/ARM64`; standardní runtime testy jsou x64. Balíček je zatím pouze Release x64. Výstupy jsou v `out/`, samostatné mezisoubory pro každý projekt/config/platform. `-Project` umožňuje sestavit jednotlivou součást. Pro ruční práci ve Visual Studiu otevřít `build-support/dirsync.vcxproj`; obě SLG jsou jeho závislosti. Ruční historická sonda `protocol_probe` není standardní CI test a nespouští se automaticky.

## Instalace a použití

Rozbalit `dirsync/` do adresáře `plugins/` hostitele a registrovat `dirsync.spl` přes Plugins Manager > Add. Zvolit český či anglický modul. Nejprve testovat na kopii místních dat. [Aktuální návod](docs/directory-sync-plugin.md), [pilotní pokyny](docs/directory-sync-plugin-pilot-guide.md) a [ruční obnova](docs/directory-sync-plugin-recovery.md) popisují postup, známá omezení a hlášení chyb. SMB nebylo ověřeno; neslibuje se crash replay ani automatický rollback. Hotové operace po Stop zůstávají provedené.

## CI

Workflow `ci.yml` sestavuje Debug Win32/x64/ARM64 a Release x64, obě x64 konfigurace všech pěti testovacích sad a ověřený ZIP. `codeql.yml` obsahuje skutečný ruční build C/C++, bez podmínky omezující analýzu na jádro hostitele. Akce jsou připnuté na konkrétní SHA; runner je výslovně `windows-2025-vs2026`. Aktuální výsledky jsou v [GitHub Actions](https://github.com/majklvi/salamander-directory-sync/actions). Balíček x64 je artefaktem úspěšného workflow; instalační ZIP se nepodepisuje. Lokálně lze spustit stejné build/test/package příkazy.

## Původ historie

Projekt byl oddělen od původního forku se zachováním celé vlastní historie a dokumentace. [Mapa commitů](docs/migration/commit-map.tsv) spojuje původní a exportované SHA; [migrační záznam](docs/migration/repository-migration.md) vymezuje změny sestavení a archiv původního vývoje. Starší datované protokoly popisují tehdejší stav, včetně přípravných místních klonů.
