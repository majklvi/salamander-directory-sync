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

## Ověření převzetí publikovaného projektu

- Finální export ze zdrojové revize ac4b64fe: 67 vlastních commitů, 52 připnutých podpůrných souborů. Před prvním push auditor prošel 612 blobů, 74 commitů, 401 stromů a 4 631 historických cest; bez nálezu. Přenesena byla pouze větev main.
- Nový nezávislý checkout C:\Samandarin\salamander-directory-sync vznikl pomocí clone --no-local. Release x64 sestavil SPL i obě SLG. Všech pět Release sad prošlo: 542 / 3 729 / 2 366 / 2 419 / 2 451 kontrol. Dřívější úplná místní matice zůstává doložena v protokolu samostatného sestavení.
- První [cloudový build/test/package](https://github.com/majklvi/salamander-directory-sync/actions/runs/39031200486) na 5442ead prošel: Debug Win32/x64/ARM64, Release x64, pět sad Debug i Release, balíček. Stažený ZIP měl SHA-256 d1927bce957235e2ed564bd69b008e198075020221edd83c88668d4bbbded427; ověřeno všech 51 manifestových hashů, 52 položek celkem, nulové chybějící místní Markdown odkazy a nulové nežádoucí zmínky. LoadLibrary místního SPL ověřilo požadavek SDK 106.
- První CodeQL běh narazil na 105sekundový limit instrumentovaného sestavení. Samostatný opravný commit 5b9b505 přidává volitelný limit a pro CodeQL používá 900 sekund. Běžný build má nadále 105 sekund. Následný build/test/package na 5b9b505 opět prošel. V okamžiku sepsání tohoto záznamu nová analýza ještě běžela; rozhodující je výsledek konkrétní revize v Actions a code scanning, nikoli samotné zahájení workflow.

### Čistá instalace a skutečné restarty přes Computer Use

Testovací kořen C:\Samandarin\builds\dirsync-migration-20261017-final\gui je oddělený od uživatelské instalace. Hostitel 0.16 x64 má EXE SHA-256 f0b2a56885b363dc0f75926dfedd3036bb1954b775753348cbca0fc4d48574dd. Zkopírovány byly jen EXE/runtime DLL a odpovídající jazykové moduly; počáteční config.reg měl 0 B, configstorage.ini pouze vlastní RegFile cestu. Rozbalen byl skutečný lokální instalační ZIP z publikované revize 5442ead, bez předchozí registrace a konfigurace pluginu. Následná oprava CI nemění pluginové zdroje.

Český Správce pluginů automaticky nalezl Synchronizace adresářů 0.1.4. Nastavení mělo výchozí uchování ON a relativní zobrazení OFF/OFF. Přes GUI byly obě relativní volby zapnuty a uloženy. Po řádném ukončení a skutečném restartu bez jazykového přepínače zůstaly čeština, registrace 0.1.4 (před použitím načtených 0) a obě zapnuté volby zachované; kontrola přes Nastavení a nezávisle v config.reg (RelativeSizesV1=1, RelativeTimesV1=1, KeepOriginals=1).

České hlavní okno porovnalo izolované left/right: přesně jeden Unicode soubor žluťoučký-漢字.txt, 9 B, pouze vlevo, návrh doprava; stavové oddíly 1 řádek / 1 viditelný / 0 skrytých / 0 blokovaných. Žádná zápisová operace nebyla spuštěna. Následující skutečný start s -language english načetl anglickou nabídku, hlavní okno, ovládání i stavový řádek. Hostitelský tmavý vzhled byl zachován. Oba jazyky byly tedy načtené v hostiteli, nikoli pouze ověřené resource testy.

Minimální hostitelská kopie hlásila známý chybějící salmon.exe a při prvním startu nedokreslila hlavní panely; po restartu byly panely vykreslené. Jedno kliknutí přes UIA index otevřelo chybný prvek; navazující kontrola používala aktuální snímky a přímou vazbu dialogu. Jednou chybějící geometrie kliknutí se vyřešila novým snímkem. Jde o omezení testovacího hostitele/automatizace, nebyla prokázána chyba pluginu. Testovací instance byly řádně zavřeny a uživatelské instance se neměnily. Tento krok nepředstavuje nové úplné GUI ověření všech datových operací, SMB, změny DPI mezi monitory ani registry backendu.

### Dokončené cloudové převzetí

Revize 1fbf70f18c5aa210ae0c15aae99ff798c813df91 prošla [celým build/test/package workflow](https://github.com/majklvi/salamander-directory-sync/actions/runs/39031761058) i [skutečným CodeQL C/C++ security-extended](https://github.com/majklvi/salamander-directory-sync/actions/runs/39031761053). Analýza 1972694431 byla přijata bez chyby a bez varování o neúplné analýze. Instrumentovaná kompilace po opravě limitu uspěla.

Code scanning obsahuje čtyři otevřené posouzené nálezy, nikoli nulu:

| Nález | Posouzení pro migrační revizi |
| --- | --- |
| [1](https://github.com/majklvi/salamander-directory-sync/security/code-scanning/1), [2](https://github.com/majklvi/salamander-directory-sync/security/code-scanning/2), [3](https://github.com/majklvi/salamander-directory-sync/security/code-scanning/3): cpp/non-constant-format, note | Status a souhrn používají Text(id), který volá LoadStringW nad jazykovým modulem. Pevné ID vybírají vlastní kompilované EN/CZ formáty, nikoli názvy/cesty či obsah porovnávaných souborů. V dodaných modulech jsou jedno nebo dvě %llu podle počtu argumentů uint64_t a cílové pole má omezenou kapacitu swprintf_s. Formáty byly zkontrolovány; libovolně upravený cizí jazykový modul není tímto posouzením ověřený. |
| [4](https://github.com/majklvi/salamander-directory-sync/security/code-scanning/4): cpp/unsafe-strcat, warning | Nezměněný SDK soubor vendor/samandarin-sdk/dbg.cpp definuje historický obal _sal_lstrcatA. Vlastní zdroje pluginu jej ani lstrcat/strcat nevolají. Není k dispozici kapacita původní signatury; změna kontraktu SDK není součástí migrace. |

Nálezy nejsou potlačeny, automaticky uzavřeny ani vydávány za opravené bezpečnostní chyby. Zůstávají dohledatelné pro další údržbu. Žádný nález nemá severity error; úspěch workflow se nezaměňuje s nulovým počtem upozornění.

Stažený finální CI ZIP této revize má SHA-256 3c965bce5e446c0cb7ce6d02942c92f063548aac8d887ed6cd0bb5367d28e7da; prošlo všech 51 hashů, úplnost inventáře, odkazy a audit nežádoucích zmínek. Pět JSON výsledků v každé konfiguraci Debug/Release má exitCode 0. Nový čerstvý klon přímo z GitHubu prošel fsck i auditem: 615 blobů, 76 commitů, 408 stromů, 4 923 historických cest, bez nálezu. Metadata autorů/časů všech 67 mapovaných commitů a hashe všech 52 podpůrných vstupů odpovídají deklarovanému původu. Místní MSBuild read trackery pro finální checkout obsahují 495 jedinečných cest; žádná nevede do původního checkoutu (povoleny jen nový projekt a systémové VS/SDK/Windows).

Původní archivní větev dostala pouze navazující dokumentační commit eae895ca, její CI prošlo. Referenční samandarin-0.16 se nezměnila. Validační draft [#30](https://github.com/majklvi/salamander/pull/30) je po převzetí určen k uzavření bez merge. Stav CI pozdějších čistě dokumentačních revizí je vždy přímo v Actions; tento záznam označuje přesné ověřené revize a balíčky.
