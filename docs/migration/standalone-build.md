# Místní kandidát samostatného sestavení

17. října 2026. Zdrojový fork `fa7c9c0ca18a51de5d9367f2c889c2678554efec` byl znovu neutrálně exportován; vstupní místní export má HEAD `3801beeebc66a1bf5bd164009b8f08c6cf1ebc6b`. Zachováno 66 vlastních commitů a přidán jeden připnutý podpůrný snapshot. Původní fork a jeho refs se při tom nemění.

Současná vrstva přesouvá vlastní zdroje do `src/`, testy do `tests/`, projekty do `build-support/`, dokumentaci do `docs/` a obslužné skripty do `tools/`. Vendored SDK/UI zůstává připnuté. Ve vlastních C++ zdrojích jsou změněné pouze include cesty; kód operací ani konfigurace se nepřepracovává. Verze a exporty zůstávají 0.1.4 a DirectorySync.

Vlastní projekty importují systémové MSBuild/VC soubory a vlastní `common.props`. Automatické hledání nadřazených `Directory.Build.props/targets` je vypnuté. Není import uživatelských props, podpisového hooku, globálních tabulek baseaddr ani knihoven hostitele. SDK je kompilováno ze snapshotu, externí importní knihovny se nepoužívají. Jako předtím plugin používá dynamické Windows/Visual C++ runtime služby a hostitelské SDK za běhu.

Zachované build parametry zahrnují v145, Windows SDK 10, UTF-8 zdroje, unsigned char `/J`, dynamický CRT a warnings-as-errors. Plugin používá původní efektivní C++ standard `stdcpplatest`, standardní testovací EXE původní C++17; jejich sjednocení by měnilo význam UTF-8 literálů a není součást migrace. Knihovna vzhledu má lokální Unicode definice, nikoli globální Unicode přepnutí staršího SDK. Debug používá ASLR bez tabulek hostitele.

Lokálně již prošly Debug Win32/x64/ARM64, Release x64 a všech pět sad v Debug i Release x64: 542 engine assertions, 3 729 adaptér assertions, 2 366 lifecycle checks, 2 419 anglických a 2 451 českých resource/layout checks v každé konfiguraci. ARM64 a Win32 byly pouze sestavené. Historická administrátorská sonda není součástí standardního runneru.

Workflow jsou připnuté na konkrétní SHA a ověřené pomocí actionlint 1.7.12. Konkrétní image VS2026 je uvedená výslovně. CodeQL má manual build skutečného pluginu a žádnou podmínku podle změn jádra. Vzdálený CI/CodeQL běh nebyl proveden, protože neexistuje remote; lokální build/test/package ověření není tvrzením o úspěchu GitHub jobů.

Další ověření přenositelnosti probíhá z čistého nezávislého klonu v jiném adresáři. Výsledný datovaný protokol zůstane u původního forku mimo distribuční balíček, aby balíček mohl odkazovat na neměnný commit zdrojů. Před publikací je nutný nový úplný audit přenášené historie a balíčku a první skutečný cloudový CI/CodeQL běh.
