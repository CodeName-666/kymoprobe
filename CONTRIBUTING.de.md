# Beiträge zu KymoProbe

[English](CONTRIBUTING.md) · **Deutsch**

Beiträge sind willkommen: Fehlermeldungen, Verbesserungsvorschläge und Pull
Requests.

## Rechteeinräumung bei Pull Requests

KymoProbe steht unter der MIT-Lizenz (siehe [LICENSE](LICENSE)). Damit das Projekt
seine Lizenzwahl auch für eingereichten Code offenhalten kann, wird ein Pull Request nur mit folgender
Zusicherung übernommen:

> Mit dem Einreichen eines Beitrags bestätige ich, dass
>
> 1. ich den Beitrag selbst erstellt habe oder die nötigen Rechte daran besitze,
>    und dass er keine Rechte Dritter verletzt;
> 2. ich Christof Seidel ein weltweites, zeitlich unbegrenztes, unwiderrufliches,
>    nicht ausschließliches und gebührenfreies Recht einräume, den Beitrag zu
>    nutzen, zu ändern, zu vervielfältigen, zu verbreiten und unter der MIT-Lizenz
>    sowie unter beliebigen anderen Lizenzen weiterzulizenzieren.
>
> Mein Urheberrecht am Beitrag behalte ich.

Bitte bestätige das mit der Checkbox in der Pull-Request-Vorlage:
**„Ich stimme der Rechteeinräumung in CONTRIBUTING.md zu.“** Eine Prüfung
blockiert Pull Requests ohne diese Bestätigung.

## Technische Hinweise

- Neue Quelldateien beginnen mit der SPDX-Zeile
  `SPDX-License-Identifier: MIT`.
- Änderungen an der Library selbst gehören nach
  [kymocore](https://github.com/CodeName-666/kymocore), siehe [README.de.md](README.de.md).
- Vor einem Pull Request `python tools/test_native.py` und `python tools/test_package.py` ausführen.
