# MISNFAtoDFA

Konverze MISNFA na DFA a jeho operace

## Cíl projektu:
Cílem projektu je implementovat algoritmus pro převod Modifikovaného Nedeterministického Konečného Automatu (MISNFA) na Deterministický Konečný Automat (DFA) a poskytnout sadu operací pro práci s DFA, jako je doplnění přechodové funkce (totalizace), odstranění neužitečných stavů a doplněk automatu.

## Funkcionalita:
### 1. Konverze MISNFA → DFA

- Použití algoritmu subset construction (převod množin stavů do jednotlivých stavů DFA).
- Identifikace konečných stavů DFA na základě konečných stavů MISNFA.
### 2. Totalizace DFA

- Doplnění přechodové funkce, aby byl automat úplný přidáním sink (pohlcujícího) stavu, pokud je potřeba.
### 3. Odstranění neužitečných stavů z DFA

- Identifikace užitečných stavů pomocí zpětného prohledávání od konečných stavů.
- Vytvoření podmnožinového DFA pouze s užitečnými stavy.
### 4. Doplněk DFA

- Převod MISNFA na DFA.
- Totalizace DFA, aby byla přechodová funkce úplná.
- Invertování konečných stavů (konečné stavy se stanou nekonečnými a naopak).
- Odstranění neužitečných stavů.
### 5. Běh slova na DFA

- Simulace průchodu daného slova automatem a ověření, zda je přijato.

## Technologie:
- Jazyk: C++
- Standard: C++20 (využití operator== pro DFA)
- Datové struktury: `std::set`, `std::map`, `std::queue`
- Testovací framework: `assert`
