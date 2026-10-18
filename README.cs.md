# Directory Synchronization for Samandarin

> [!CAUTION]
> **ALFA VERZE — NEBEZPEČÍ ZTRÁTY DAT. NENÍ URČENO PRO PRODUKČNÍ POUŽITÍ.**
> Použití výhradně na vlastní nebezpečí. Autoři a přispěvatelé neposkytují **ŽÁDNOU ZÁRUKU A NENESOU ODPOVĚDNOST ZA JAKOUKOLI ZTRÁTU NEBO POŠKOZENÍ DAT**. Testujte pouze na postradatelných kopiích a mějte nezávislou ověřenou zálohu. Uchování originálů nezajišťuje automatické vrácení změn ani neodstraňuje riziko ztráty dat.

**[Stáhnout hotový plugin 0.1.6 ALFA pro x64](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.1.6/dirsync-0.1.6-x64.zip)** · [Kontrolní součty SHA-256](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.1.6/package-sha256.txt)

ZIP obsahuje zkompilovaný plugin a oba jazykové moduly. **Pro instalaci není potřeba nic kompilovat, instalovat Visual Studio ani SDK.**

[English](README.md)

**Verze 0.1.6 ALFA · místní NTFS pilot · x64.** Příkazy pro změnu návrhu jsou neaktivní, pokud by nezměnily žádnou vybranou položku. U smíšeného výběru zůstanou dostupné, pokud lze změnit alespoň jednu položku. Název souboru se zobrazuje pouze na straně, kde položka existuje; párové položky mají názvy na obou stranách.

Samostatný nativní plugin pro Samandarin. Porovnání adresářů, úprava návrhů, kopírování, přepisy a asymetrické odstranění s volitelným uchováním originálů. Český a anglický modul, vzhled podle hostitele. SDK se nerozšiřuje.

<img width="917" height="796" alt="image" src="https://github.com/user-attachments/assets/14b296c8-2def-40cb-8b13-33a4a101a29f" />

*Náhled porovnání adresářů s návrhy synchronizačních operací.*

Samostatný vývoj a hlášení chyb probíhají v [majklvi/salamander-directory-sync](https://github.com/majklvi/salamander-directory-sync). Zdrojový archiv a původ závislostí jsou uvedeny v `EXPORT-ATTRIBUTIONS.md` a `EXPORT-SUPPORT.json`. Historické protokoly v `docs/` popisují své tehdejší etapy, nikoli dnešní omezení.

**Vývojová verze: 0.2.0 ALFA.** Pojmenované profily ukládají dvojici adresářů, masky, volby porovnání a filtry. Obnova a vzhled zůstávají společné; načtení profilu nespouští operaci. Odkaz ke stažení výše nadále vede na vydanou verzi 0.1.6.

[Používání profilů](docs/directory-sync-plugin.md#profily-020)

## Sestavení ze zdrojů a testy (pro vývojáře)

Windows, Visual Studio 2026 s C++ desktop tools (v145, potřebné x86/x64/ARM64 komponenty), Windows SDK 10 a PowerShell 7.3 nebo novější. Není potřeba checkout ani knihovna Salamanderu, Python, vcpkg, certifikát nebo síťové stahování závislostí. MSBuild se vyhledává pomocí systémového vswhere.

```powershell
pwsh -NoProfile -File tools/build.ps1 -Configuration Release -Platform x64
pwsh -NoProfile -File tools/build.ps1 -Configuration Debug -Mode Tests
pwsh -NoProfile -File tools/test.ps1 -Configuration Debug
pwsh -NoProfile -File tools/package.ps1 -OutputDirectory out/package-01
```

`-Configuration Debug/Release`, `-Platform Win32/x64/ARM64`; standardní runtime testy jsou x64. Balíček je zatím pouze Release x64. Výstupy jsou v `out/`, samostatné mezisoubory pro každý projekt/config/platform. `-Project` umožňuje sestavit jednotlivou součást. Pro ruční práci ve Visual Studiu otevřít `build-support/dirsync.vcxproj`; obě SLG jsou jeho závislosti. Ruční historická sonda `protocol_probe` není standardní CI test a nespouští se automaticky.

## Testované verze hostitele

Nejnižší testované verze Samandarinu jsou **0.15.1** při uživatelském testování a **0.16** při automatizovaném testování pomocí AI z vlastního vývojářského sestavení. Verze 0.16 v době testování ještě nebyla vydaná.

## Instalace a použití

Rozbalit `dirsync/` do adresáře `plugins/` hostitele a registrovat `dirsync.spl` přes Plugins Manager > Add. Zvolit český či anglický modul. Nejprve testovat na kopii místních dat. [Aktuální návod](docs/directory-sync-plugin.md), [pilotní pokyny](docs/directory-sync-plugin-pilot-guide.md) a [ruční obnova](docs/directory-sync-plugin-recovery.md) popisují postup, známá omezení a hlášení chyb. SMB nebylo ověřeno; neslibuje se crash replay ani automatický rollback. Hotové operace po Stop zůstávají provedené.

## CI

Workflow `ci.yml` sestavuje Debug Win32/x64/ARM64 a Release x64, obě x64 konfigurace všech pěti testovacích sad a ověřený ZIP. `codeql.yml` obsahuje skutečný ruční build C/C++, bez podmínky omezující analýzu na jádro hostitele. Akce jsou připnuté na konkrétní SHA; runner je výslovně `windows-2025-vs2026`. Aktuální výsledky jsou v [GitHub Actions](https://github.com/majklvi/salamander-directory-sync/actions). Balíček x64 je artefaktem úspěšného workflow; instalační ZIP se nepodepisuje. Lokálně lze spustit stejné build/test/package příkazy.

## Původ historie

Projekt byl oddělen od původního forku se zachováním celé vlastní historie a dokumentace. [Mapa commitů](docs/migration/commit-map.tsv) spojuje původní a exportované SHA; [migrační záznam](docs/migration/repository-migration.md) vymezuje změny sestavení a archiv původního vývoje. Starší datované protokoly popisují tehdejší stav, včetně přípravných místních klonů.

## Čím plugin vyniká

- Volitelné relativní velikosti a časy souborů.
- Vlastní barvy podle světlého či tmavého schématu hostitele.
- Symetrické sloupce, skupiny adresářů, řazení a kombinovatelné filtry.
- Upravitelné návrhy operací a porovnání bez zápisů.
- Volitelné, zapamatované uchování originálů před přepisy a mazáním.
- Nativní rozhraní, Unicode a dlouhé cesty, zobrazení podle hostitele.

## Relativní porovnávání

V **Nastavení → Sloupce** lze nezávisle zapnout relativní velikosti a časy; obě volby jsou výchozím stavem vypnuté. Například soubory o 8 KiB a 4 KiB ukážou „o 4 096 B větší/menší“, čas může uvádět „o 2 d 3 h novější/starší“.

Rozdíl velikosti je přesný počet bajtů s oddělováním číslic podle hostitele. Čas je uplynulá doba z UTC údajů, den má 24 hodin; rozdíl pod sekundou má vlastní popisek. Shodné hodnoty, jednostranné položky a adresáře ponechávají absolutní zobrazení. Volby nemění pevnou toleranci času 3 sekundy, návrhy ani řazení podle skutečných hodnot. Použijí se bez nového skenu a pamatují se přes restart.

## Nastavení vzhledu

Otevřete **Settings… / Nastavení…** v náhledu nebo **Plugins Manager → Configure…**. Barevné tlačítko u každé položky otevře nativní výběr barvy. **Restore default colors** obnoví pouze barvy aktuálního světlého/tmavého režimu. OK změny uloží, Cancel je zahodí; výsledky a ruční návrhy zůstanou zachované.

<img width="461" height="498" alt="image" src="https://github.com/user-attachments/assets/f3c9586c-fbc3-4c6a-9ff2-c04b76f4bdb8" />

*Nastavení barev, sloupců, relativních údajů a uchování originálů.*

## Stažení a instalace alfa verze

Stáhněte [hotový x64 ZIP z release 0.1.6](https://github.com/majklvi/salamander-directory-sync/releases/download/v0.1.6/dirsync-0.1.6-x64.zip). Obsahuje zkompilovaný `dirsync.spl` i český a anglický jazykový modul; žádná kompilace není potřeba. Balíček je nepodepsaný. Celý adresář `dirsync` rozbalte do `plugins` vedle `salamand.exe`, zachovejte podadresář `lang` a přes Správce pluginů přidejte `dirsync.spl`, pokud již není registrovaný. Podrobnosti uvádí úplný návod.

## Běžný postup a rozsah operací

1. Zkontrolujte levý a pravý adresář, masku a volby skenu. **Compare / Porovnat** pouze čte a připraví náhled.
2. Prohlédněte výsledky a kombinujte filtry. Pravé tlačítko nebo Shift+F10 mění návrhy zvýrazněných řádků; označení ke kopírování ještě nic nekopíruje. **Skip / Přeskočit** položku vynechá.
3. **Synchronize…** nabízí zvlášť nové kopie, přepisy a asymetrické mazání. **Provádějí se všechny viditelné způsobilé návrhy, nikoli pouze zvýrazněné řádky.** Skryté a vynechané řádky se neprovedou. Před potvrzením ověřte počty, směr a režim obnovy.
4. Po dávce přečtěte souhrn a znovu porovnejte. Nové porovnání zahodí ruční návrhy. Změna checkboxů se použije při příštím Porovnat; změna cesty nebo masky zneplatní plán.
5. Malý křížek vedle Porovnat či Synchronizovat zastaví příslušnou práci. Zavření počká na worker; již dokončené změny se automaticky nevracejí.

V asymetrii je levý adresář zdroj. Položky pouze vpravo jsou kandidáty k odstranění. Pro adresářové řádky zapněte Prázdné adresáře; adresář se odstraní jen skutečně prázdný po zařazených potomcích. Vynechaný či skrytý potomek chrání rodiče.

Samostatné **Remove selected… / Odstranit vybrané…** v kontextovém menu pracuje naopak se zvýrazněnými položkami na zvolené straně. Bez Shiftu jde do Koše Windows, se Shiftem při aktivaci konkrétního příkazu strany maže trvale. **Nepoužívá uchování originálů ani protokol synchronizace.** Adresáře musejí být prázdné; výběr rodiče nezařadí potomky. Obnovu z Koše provádějte v Koši Windows.

## Obnova, omezení a hlášení chyb

Uchovat originály je standardně ON a nastavení se pamatuje. Zvolte adresář obnovy mimo oba stromy, který není ani jejich předkem; počítejte s místem pro zálohy a staging. Při ON se před přepisem či mazáním originály uloží a ověří. Při OFF nevzniká nová záloha ani trvalý protokol a synchronizační příkazy nepoužívají Koš. Staré zálohy se automaticky neuklízejí.

Ruční obnova: podle souhrnu najděte běh a manifest.jsonl, u úspěšně uložené položky spárujte original a saved. Kopii nejprve přeneste do odděleného kontrolního adresáře; až po kontrole ji vraťte na původní místo. Strukturu vytvořte podle adresářových záznamů, nekopírujte celý pomocný strom běhu. Původní ACL ani všechny NTFS vlastnosti se automaticky neobnovují.

Ověřen je místní NTFS a x64. SMB je neotestované. Skryté a systémové položky zahrnuje maska a rozsah i při jejich skrytí v panelech; atributy ukazuje tooltip. Porovnání obsahu nezahrnuje ADS. Samostatné kopírování prázdných adresářů, historie, profily dvojic, automatická smíšená dávka, obnova a úklid záloh nejsou implementované. Reparse body, EFS a nepodporované cloud/offline položky se blokují; read-only cíle se násilně nepřepisují ani nemažou. První chyba zastaví další práci, hotové změny zůstanou. Přenosy zatím nemají společnou frontu hostitele, pozadí ani rychlostní limit. Přechod mezi monitory s různým DPI zůstává neověřený.

Chyby hlaste přes [Issues](https://github.com/majklvi/salamander-directory-sync/issues): verze a hash balíčku, prostředí/jazyk/schéma/DPI, masky/volby/filtry, obnova ON/OFF, přesný postup a očekávaný/skutečný výsledek. Souhrn lze kopírovat Ctrl+Shift+C, relativní cesty vybraných řádků Ctrl+C. Z přiložených kopií protokolů odstraňte soukromé údaje; ideální je malá syntetická reprodukce. Při neočekávané změně dat zastavte další zápisy a ponechte souhrn i obnovovací běh.

[English README](README.md) · [Licence GPL-2.0-or-later](LICENSE) · [Atribuce závislostí](docs/third_party.md)
