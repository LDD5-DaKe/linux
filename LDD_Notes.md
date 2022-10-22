## Frage a

### API und ABI

Für Kompatibilität zwischen zwei Programmen sind zwei Schnittstellen wichtig.

Die [API](https://en.wikipedia.org/wiki/API) (Application Programming Interface, Programmierschnittstelle)
definiert dabei die Schnittstelle auf Sourcecode-Level.

Dazu zählen z.B. die Funktionsnamen, Anzahl und Reihenfolge der Parameter. Auch
Name und Datentyp von Feldern in Strukturen sind Teil der API. Einige andere
Änderungen (z.B. Reihenfolge von Feldern in Strukturen) können vorgenommen
werden, ohne die API zu ändern, da diese Information für den Sourcecode nicht
wichtig ist.


Die [ABI](https://en.wikipedia.org/wiki/Application_binary_interface)
(Application Binary Interface, Binärschnittstelle) definiert die Schnittstelle zwischen den
kompilierten Programmen.

Ein wichtiger Teil der ABI sind calling-conventions, die bestimmen, wie Daten an
Funktionen übergeben und zurück gegeben werden.
Hier werden auch Faktoren, wie das Layout einer Struktur relevant.

### stabile API und ABI

Unter einer Stabilen API versteht man ein Interface, das sich im Sourcecode nur
so ändert, dass alter Code noch kompiliert werden kann.

Unter einer Stabilen ABI versteht man ein Interface, wo sich auch die ABI nicht
ändert und dadurch auch bereits kompilierte Programme diese Funktion aufrufen
können.

### Im Linux Kernel

Schnittstellen innerhalb des Linux Kernels haben weder eine stabile API noch
ABI. Daher können Module, die für eine Version des Kernels kompiliert werden nur
für diese verwendet werden.

Ist ein Modul im Kernel Repo aufgenommen (mainlined), wird notwendige Wartung
zum einhalten geänderter Schnittstellen von demjenigen, der die Schnittstelle
ändert übernommen.

## Frage b

```
filename:       /home/alex/Documents/Studium/SEM5/LDD/linux/drivers/misc/ledpwm.ko
author:         Alexander Daum <alexander.daum@mailbox.org>
description:    Driver for the LED PWM component of the DE1-SoC Computer
license:        GPL
depends:
intree:         Y
name:           ledpwm
vermagic:       5.4.69 SMP mod_unload ARMv7 p2v8
```

* filename: Der komplette Pfad der Datei, Informationen stammt aus dem
  Dateisystem.
* author: Der Author des Moduls. Diese Information wird mit dem `MODULE_AUTHOR`
  Macro im Sourcecode bereitgestellt.
* license: Die Lizenz des Moduls. Die Information wird aus dem
  LPDX-License-Identifier Kommentar am Anfang der Datei extrahiert.
* depends: Abhängigkeiten des Moduls (in diesem Fall keine). Diese werden mit
  dem `depends on` Keyword in der Kconfig Datei definiert.
* intree: Zeigt an, ob das Modul im Kernel Source-Tree gebaut wurde, oder
  außerhalb. Y bedeutet hier ja. Bei einem intree build werden die Quellen des
  neuen Moduls innerhalb des Kernel Verzeichnisses erstellt und auch direkt in
  die entsprechenden Kconfig und Makefile Dateien integriert. Bei einem
  out-of-tree build liegen die Quelldateien für den Treiber in einem separaten
  Verzeichnis, nicht bei den Kernel sourcen.
  Diese Information kann das Build-System leicht erkennen, da für einen
  out-of-tree build die Option M beim make aufruf gesetzt werden muss (Quelle
  Documentation/kbuild/modules.rst)
* name: Der Name des Moduls. Dieser Name entspricht dem Namen der Quelldatei.
* vermagic: Information über die Version des Kernels für die das Modul gebaut
  wurde, diese beinhaltet die kernel version, wichtige Informationen zur
  Konfiguration (preempt, module unload support) und der Architektur (ARMv7).
  Dieser String wird in `include/linux/vermagic.h` als `VERMAGIC_STRING` definiert.
