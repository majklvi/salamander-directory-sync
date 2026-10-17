# Pravidla samostatného pluginu

- Zachovat binární kompatibilitu se současným SDK Samandarin 0.16. SDK aktualizovat jen jako výslovnou změnu připnuté závislosti; nepřidávat požadavky na nové rozhraní hostitele.
- Používat široká Win32 API a zachovat dlouhé a Unicode cesty. Nepřidávat `MAX_PATH` buffery ani převody přes ACP tam, kde už existují široká data.
- GUI má odpovídat kompaktním nativním dialogům Salamanderu: `MS Shell Dlg` 8, běžné malé mezery, OK/Cancel, lokalizace češtiny i angličtiny a vzhled řízený hostitelem. Nevytvářet vlastní vizuální styl.
- Zachovat staging, ověření dat/metadat a zapamatovanou volbu uchování originálů. Mazání adresářů je pouze explicitně naplánované a pouze prázdných adresářů; žádná rekurzivní náhrada při chybě.
- Sestavení, testy a balení používat přes vlastní `tools/`. Nepřidávat závislost na okolním checkoutu, sdílených props hostitele, lokálním certifikátu nebo globálním výstupním adresáři.
- Verifikovat všech pět x64 sad v Debug a Release a potřebnou build matici. ARM64 build není runtime test. SMB, živé GUI a skutečný cloudový CodeQL nevydávat za ověřené bez příslušného testu.
- Zachovat licence a původ vendored souborů. Výsledky datovaných historických protokolů nepřepisovat na tvrzení o současném stavu.
- Hlavní vývoj probíhá v `majklvi/salamander-directory-sync`, větev `main`. Kontrolovat vlastní CI; nepoužívat validační helper původního forku. Publikaci verzí a destruktivní změny historie provádět jen podle zadání uživatele.
