# Synchronizace adresářů — pilotní testování

> [!CAUTION]
> **ALFA VERZE — NEBEZPEČÍ ZTRÁTY DAT. NENÍ URČENO PRO PRODUKČNÍ POUŽITÍ.**
> Použití výhradně na vlastní nebezpečí. Autoři a přispěvatelé neposkytují **ŽÁDNOU ZÁRUKU A NENESOU ODPOVĚDNOST ZA JAKOUKOLI ZTRÁTU NEBO POŠKOZENÍ DAT**. Testujte pouze na postradatelných kopiích a mějte nezávislou ověřenou zálohu. Uchování originálů nezajišťuje automatické vrácení změn ani neodstraňuje riziko ztráty dat.

[English](directory-sync-plugin-pilot-guide.en.md)

Vydaný základ 0.3.0 ALFA a výslovně označené vývojové změny 0.4.0 pro **Samandarin x64**, čeština i angličtina. Balíček obsahuje zkompilované moduly; instalace nevyžaduje kompilaci. Balíček je nepodepsaný a obsahuje pouze plugin. Pilot je určen pro **kopie místních NTFS dat**; SMB, síťové disky a změny SDK jsou mimo tento pilot.

Novinka 0.3.0 ALFA: **Nastavení → Uložené originály…** zobrazí existující relace uchování originálů bez jejich změny, také při vypnutém uchování originálů. Vyberte relaci, dvojklikem otevřete podrobnosti položky, zkopírujte původní či uloženou cestu nebo otevřete její adresář. Ctrl+C / Ctrl+Shift+C kopírují původní/uloženou cestu ze seznamu položek; Aktualizovat znovu načte záznamy. Zaznamenaný úspěch není novým ověřením obsahu. Přehled neobsahuje automatickou obnovu ani úklid. Podrobnosti v [úplném návodu](directory-sync-plugin.md#uložené-originály-030).

Klikáním do záhlaví sloupce přepínáte vzestupné/sestupné řazení souborů uvnitř adresářů. Pravé tlačítko na záhlaví nebo sekce **Sloupce / Columns** v nastavení pluginu skryje či zobrazí volitelné sloupce. Názvy vlevo i vpravo, cesta adresáře a operace zůstávají. Volby sloupců a řazení se pamatují; skrytí sloupce nevynechává soubory ze synchronizace.

Výsledky jsou seskupené podle relativních adresářů (`\` je kořen). Pod záhlavím jsou názvy souborů; záhlaví samo není položka operace a nevybírá své potomky. Prázdné skupiny po filtrování zmizí. Adresář určený k odstranění má vlastní řádek v nadřazené skupině.

Nejnižší testované verze hostitele: 0.15.1 uživatelsky a nevydaná 0.16 automatizovaně pomocí AI z vlastního vývojářského sestavení.

V 0.3.0 uloží **Synchronizovat… → Exportovat porovnání a výsledky…** CSV/text se všemi načtenými řádky včetně skrytých/Skip a zaznamenanými výsledky poslední potvrzené dávky. Exportujte před novým porovnáním či zavřením. Cíl zvolte mimo porovnávané stromy a adresář obnovy; před sdílením zkontrolujte cesty. Podrobnosti v [návodu k exportu](directory-sync-plugin.md#export-porovnání-a-výsledků-operací-030).

V 0.3.0 se při potvrzených souborových operacích otevře také samostatné okno **Průběh synchronizace**. Zastavit, Escape i jeho křížek požádají o přerušení a počkají na ukončení pracovníka; dokončené změny zůstanou. Celkový ukazatel počítá kroky včetně uchování originálů. Viz [průběh přenosu](directory-sync-plugin.md#průběh-přenosu-030).

## Interaktivní chyby (vývoj 0.4.0)

V této vývojové verzi otevře **Synchronizovat…** přímo kompaktní potvrzení s nezávislými směry kopírování, samostatným asymetrickým mazáním, viditelným stavem obnovy a **Podrobnostmi…** pouze k prohlížení. Vypnutí skupiny vyřadí její návrhy z dávky; neobrátí šipky ani neskryje řádky. Bez vybrané operace je OK neaktivní. Samostatné příkazy a export zůstávají pod **Viditelným plánem** v kontextové nabídce. Výsledky a CSV/text rozlišují vyloučení při potvrzení od Přeskočit během provádění. Viz [implementace a rozsah ověření](directory-sync-plugin-sync-confirmation-040.md).

Stejná chybová pravidla platí pro samostatné operace i **Provést viditelný plán…**. Zkontrolujte potvrzení, režim uchování a konečné počty přeskočených/blokovaných položek. Přeskočit vše platí jen pro stejnou operaci, fázi, stranu a kód Windows v tomto příkazu; odpovídající rozhodnutí při uchování platí před přepisy i před mazáním. Stop/Close zabrání dalším etapám, hotové změny zůstanou a před další operací je třeba nové porovnání. Viz [aktuální návod](directory-sync-plugin.md#interaktivní-chyby-operací-vývoj-040) a [ověřené scénáře](directory-sync-plugin-combined-errors-040.md).

## Profily (0.2.0)

Od **0.2.1** se při zavření hlavního okna se změněným uloženým profilem zobrazí tři tlačítka: **Uložit**, **Neukládat** a **Zrušit**. Zrušit ponechá okno i úpravy otevřené. Při ukládání se ověřují cesty a masky; neúspěšné uložení okno nezavře. Nezměněný profil nebo nepojmenovaná úloha se zavřou bez dotazu. Během operace zavření nejprve požádá o Stop a počká na dokončení; Zrušit v následném dotazu na profil již zastavenou práci neobnoví. Nucené uvolnění pluginu interaktivní dotazy vynechává.

Použijte **ikonu Profilů úplně napravo od polí s adresáři**. Nabídka obsahuje uložené profily, označení aktivního profilu, **Aktuální adresáře panelů** a **Správa profilů…**. Položka panelů vrátí adresáře převzaté z hostitele při otevření okna. Výběr pouze načte nastavení; nikdy sám nespustí porovnání ani synchronizaci.

Aktivní profil je vidět v titulku hlavního okna a tooltipu ikony. Hvězdička označuje neuložené změny adresářů, masek, voleb porovnání či filtrů. Ve **Správě profilů…** vyberte profil: **Uložit aktuální** nahradí jeho nastavení hodnotami z hlavního okna, **Uložit jako…** vytvoří pojmenovanou variantu; další tlačítka umožňují přejmenování a odstranění. Pod seznamem je úplný obsah vybraného profilu: obě cesty, masky, pět voleb porovnání a pět filtrů výsledků, u každé volby výslovně Zapnuto/Vypnuto. Cesty a masky jsou pouze pro čtení; lze je označit a zkopírovat. **OK** potvrdí změny správy a načte vybrané nastavení, pokud se liší; **Cancel** zahodí všechny změny této správy. Odstranění profilu nikdy nemaže soubory. Před opuštěním neuloženého nastavení nebo výsledků se plugin zeptá; načtení jiného nastavení zruší staré porovnání. Uložení či přejmenování aktuální úlohy výsledky zachová.

Příště se načte naposledy vybraný uložený profil, pouze s výslovně uloženými hodnotami. **Uchovat originály, adresář obnovy, vzhled, sloupce, řazení i relativní zobrazení zůstávají společné**; časová tolerance hostitele se čte při každém porovnání. Výběr panelů, zvýrazněné řádky, výsledky ani ruční návrhy se neukládají. Další variantu zachováte pomocí **Uložit profil jako…**. Během operace je správa profilů zakázaná. Tato verze nemá plánování ani automatické provádění.

## Instalace

1. Zavřete instance Samandarinu používající starší plugin. Rozbalte ZIP a celý adresář `dirsync` vložte do `plugins` vedle `salamand.exe`; zachovejte podadresář `lang` s oběma SLG.
2. V **Plugins → Plugins Manager → Add…** vyberte `plugins/dirsync/dirsync.spl`, pokud již není registrovaný. Spusťte **Directory Synchronization → Directory synchronization…** (česky Synchronizace adresářů).
3. V **Settings… / Nastavení…** ponechte pro začátek **Keep originals / Uchovávat originály** zapnuté a vyberte existující adresář obnovy mimo oba porovnávané stromy, který není ani jejich předkem. Zajistěte místo pro zálohy i právě přenášený soubor. Volby se pamatují. Jazyk a světlý/tmavý režim určuje hostitel.

## Krátký pracovní postup

1. Vyberte dvě oddělené testovací kopie adresářů. Nastavte masku a volby, pak **Compare / Porovnat**. Tento krok pouze čte data.
2. Prohlédněte návrhy a důvody. Filtry lze kombinovat; pravým tlačítkem nebo Shift+F10 měňte návrhy zvýrazněných řádků. **Skip** položku vynechá. Popisky akcí lze zapnout v nastavení.
3. **Synchronize… / Synchronizovat…** nabízí zvlášť nové kopie, přepisy a mazání. Každá operace má vlastní potvrzení. **Provádějí se všechny viditelné způsobilé návrhy, nikoli jen zvýrazněné řádky.** Skryté a Skip řádky jsou vynechané; zkontrolujte počty i režim obnovy v potvrzení.
4. Mazání je dostupné v asymetrii: **levá strana je zdroj**, pravostranné položky chybějící vlevo jsou kandidáty k odstranění. Adresáře vyžadují volbu Empty folders / Prázdné adresáře. Obsah se maže před rodiči, adresář jen když je skutečně prázdný; vynechaný potomek chrání rodiče. Kořeny a společné prázdné adresáře zůstávají.
5. Po každé operaci přečtěte souhrn a spusťte nové Compare. Nové porovnání zahodí ruční změny návrhů, takže potřebný Skip znovu nastavte. Změny šesti voleb skenu se uplatní až při příštím Compare; změna cesty či masky zneplatní dosavadní plán.
6. Křížek v pravé části Compare nebo Synchronize zastaví příslušnou práci. Zavření počká na worker. **Již dokončené změny zůstávají provedené**, automatické vrácení není součástí pluginu.

Vyzkoušejte běžné kopírování, přepis a malé vlastní odstranění; poté ruční obnovu do nového kontrolního adresáře. U většího seznamu sledujte odezvu, filtry, klávesnici a změny velikosti okna. Uveďte i zkušenost s jiným DPI či více monitory. Není nutné opakovat laboratorní chybové testy.


## Přímé odstranění z kontextového menu

Horní „Označit ke kopírování…“ / „Označit k přeskočení“ pouze mění návrhy. Oddělené **Odstranit vybrané… → Vlevo… / Vpravo… / Na obou stranách…** odstraňuje právě zvýrazněné položky. Bez Shiftu do Koše, se Shiftem při aktivaci příkazu strany trvale. Režim a skutečné cesty ověřte v potvrzení OK/Zrušit. Uchovat originály se na tento příkaz nevztahuje; trvalé mazání nemá zálohu pluginu. Adresáře musí být prázdné, rodič nevybírá potomky. První chyba zastaví dávku a po běhu je nutné nové Porovnat. Obnovu z Koše provádějte v Koši Windows.

## Obnova a hlavní omezení

- Při ON se před přepisem/mazáním nejprve uloží a ověří zařazené originály. V cestě uvedené v souhrnu najdete `manifest.jsonl` a prováděcí protokol. U synchronizačních příkazů při OFF **nevzniká nová záloha ani trvalý protokol a nepoužívá se Koš**. Starší zálohy se automaticky neuklízejí a zabírají místo.
- Ruční obnova: v manifestu použijte `saved` u úspěšně uložené položky. Strukturu podle adresářových záznamů nejprve vytvořte v novém kontrolním adresáři; soubory přeneste běžným F5 z jejich uložených cest. Nekopírujte celý pomocný strom relace do původního kořene. Ověřená obnova zahrnuje strukturu, souborová data, ADS a souborový čas. F7 vytvoří adresáře s novými časy; přesná obnova adresářových metadat a původních oprávnění není automatická.
- Skryté a systémové položky jsou zahrnuté podle masky a rozsahu, i když je hlavní panely nezobrazují. Atributy H/S ukazuje tooltip řádku. By content porovnává pouze hlavní data, nikoli ADS; tolerance časů se při každém Compare přebírá z Konfigurace → Obecné (obvykle 2 sekundy; při vypnutí přesná shoda časů). Hodinové posuny DST se nepřebírají.
- Samostatné kopírování prázdných adresářů zatím není podporované. Reparse/junction/symlink, EFS a nepodporované cloud/offline položky se blokují; adresáře s vlastními ADS se před mazáním odmítnou. Read-only cíle se nepřepisují ani nemažou obcházením atributů.
- Ve vydané 0.3.0 první chyba zastaví zbývající práci. Vývojová 0.4.0 se při opravitelné chybě dotáže, není-li zapnuté zastavení při první chybě; po přeskočení mohou pokračovat nezávislé operace. Závažná chyba nebo Stop nadále ukončí zbytek. Nejde o transakci ani úplný NTFS zálohovací nástroj; během běhu neměňte testovací stromy jiným programem. Mohou zůstat vytvoření prázdní rodiče, neúplný protokol nebo oznámený zbytek stagingu.
- Přenos používá vlastní okno pluginu s Pozastavit/Pokračovat; nezapojuje se do společné fronty jádra. Ve vývojové 0.4.0 si pamatuje rychlostní limit společně pro další operace i po restartu, při nízkém limitu zmenšuje skutečné bloky čtení/zápisu a Zrušit/křížek okna průběhu vyvolá dotaz Ano/Ne. Vydaná 0.3.0 používá limit pouze pro aktuální dávku a okamžité zrušení. Panely hostitele zůstávají v nemodálním režimu dostupné. Ověřený runtime je x64. Běh na jiném systémovém DPI a přechody mezi monitory zbývají ověřit; procházení tisíců řádků přes UI Automation dříve způsobovalo prodlevy.

Podrobnosti jsou v [aktuálním návodu](directory-sync-plugin.md) (`README.cs.md` v ZIP).

## Jak hlásit chybu

Chybu hlaste v [Issues](https://github.com/majklvi/salamander-directory-sync/issues); souhrn výsledku lze zkopírovat přes **Ctrl+Shift+C**, cestu vybraného řádku přes **Ctrl+C**. Hlášení ideálně obsahuje:

- Identifikaci balíčku: název ZIP a SHA-256 z přiloženého `package-sha256.txt`; hash `dirsync.spl` je také v `files.json`.
- Verzi Samandarinu a Windows, jazyk, světlý/tmavý režim, DPI monitorů a potvrzení místního NTFS.
- Přesný postup, masku, volby skenu, filtry, ruční návrhy, druh operace a Keep originals ON/OFF.
- Očekávaný a skutečný výsledek; počty a přesnou chybu ze souhrnu, případně snímek okna. U prodlev počet řádků, délku čekání a zda běžel nástroj UI Automation.
- Při ON příslušné `manifest.jsonl` a `delete.jsonl` či `overwrite.jsonl`, pokud existují. Při OFF jejich nepřítomnost není chyba. Před odesláním odstraňte citlivé cesty/názvy z kopie hlášení; původní protokoly si ponechte pro obnovu. Neposílejte skutečná soukromá data; nejlepší je malá syntetická sada reprodukující problém.

Při neočekávané změně dat další zápisy zastavte, uchovejte souhrn i relaci uchování originálů a nejprve zkontrolujte kopie v odděleném adresáři.

Od 0.1.3 lze v Nastavení → Sloupce nezávisle zapnout relativní velikost a relativní čas. Obě volby jsou standardně vypnuté. U rozdílných hodnot dvojic souborů zobrazují rozdíl vůči protější straně; shodné hodnoty a chybějící protějšek ponechávají absolutní údaje. Řazení se nadále řídí skutečnou velikostí a časem. Podrobnosti uvádí README.cs.md.

Tabulka má symetrické pořadí Název | Velikost | Změněno | Operace | Změněno | Velikost | Název. První trojice patří levému adresáři, poslední pravému. Volba Nastavení → Sloupce → Uvádět stranu v záhlaví přidá do popisků vlevo/vpravo; standardně je vypnutá a pamatuje se.

Podrobné vývojové záznamy zůstávají na GitHubu; adresář docs se do instalačního ZIP nepřibaluje.

## Přenos profilů a prázdné adresáře (0.2.1)

Správa profilů nabízí **Export…** celého zobrazeného seznamu a **Import…** z verzovaného souboru `.dsprofiles`. Export zapíše právě zobrazený seznam ihned, nezávisle na pozdějším OK/Zrušit ve správě. Import pouze přidá profily do rozepsaných změn správy; **OK** je uloží, **Cancel** import zahodí. Při kolizi názvů se doplní číselná přípona. Existující profily i aktivní výběr zůstanou zachované. Samotný import nenačte profil, nezruší porovnání ani nespustí operaci. Cesty adresářů se přenášejí beze změny a na jiném počítači je může být nutné upravit. Obnova, vzhled, výsledky, ruční návrhy ani aktivní výběr se nepřenášejí. Jde o velikostně omezený binární formát podle verze úložiště profilů, nikoli textovou konfiguraci nebo spustitelný skript.

Před porovnáním zapněte **Prázdné adresáře**. **Kopírovat viditelné nové položky…** přenáší viditelné způsobilé návrhy skutečně prázdných adresářů oběma směry společně s novými soubory. Respektuje Skip i filtry. Nové cíle dědí cílová oprávnění; adresáře zachovávají čas změny a podporované atributy. ADS adresářů se odmítají. Obsah vyloučený maskou neznamená, že je adresář skutečně prázdný. Existující cíle se nenahrazují. Stop zastaví zbývající položky a podle možností uklidí vlastní nepublikovaný staging; dokončené cíle a potřebné vytvořené nadřazené adresáře zůstávají. Po provedení znovu porovnejte.

## Souhrnná synchronizace viditelného plánu (0.2.1)

Příkaz **Synchronizovat… → Provést viditelný plán…** po jednom potvrzení provede všechny způsobilé viditelné návrhy. Zvýraznění jej neomezuje; Skip, skryté, blokované a nerozhodnuté řádky se vynechají. Potvrzení ukazuje nové soubory/adresáře, přepisy, mazané soubory/adresáře, směry a objem hlavních dat, objem původních dat, vynechané/skryté položky i pevně převzatou volbu uchování originálů a cestu obnovy. Při vypnutém uchování nemají přepsané/smazané originály kopii pro obnovu v pluginu; Koš se nepoužívá.

Pořadí: nejprve nové cíle, potom přepisy existujících souborů, nakonec pravostranné asymetrické mazání (soubory před prázdnými adresáři od nejhlubších). Každý adaptér zachová své ověřování a obnovu. Před další etapou musí čekající položky stále odpovídat potvrzeným identitám a metadatům; dokončené operace neopravňují převzít cizí změny čekajících položek. Ve vývojové 0.4.0 může opravitelná chyba nabídnout Opakovat / Přeskočit / Přeskočit vše / Zrušit. Přeskočení umožní pokračovat nezávislými potvrzenými operacemi i dalšími etapami; neuchovaný originál blokuje svou operaci a ponechaný potomek chrání rodiče. Závažná chyba, neplatný plán, chyba protokolu nebo Stop/Close zastaví zbytek. Platí i společná volba zastavení při první chybě. Dokončené změny zůstanou, bez automatického návratu. Výsledek uvádí hotové/plánované, přeskočené a blokované položky, chyby, nezahájené operace a dostupné manifesty/protokoly obnovy. Před opakováním znovu porovnejte. Zavření požádá o Stop a počká před dotazem na uložení profilu. Nejde o plánování ani automatické spouštění.
