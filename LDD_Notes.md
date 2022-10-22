## Frage a

Ein Kernel-Modul muss so gut wie immer andere Funktionen aus dem Kernel aufrufen.
Der Code, der zum aufrufen der Funktionen notwendig ist hängt von der Signatur der Funktion und den calling conventions ab.
Dieses Interface auf binärebene wird als [ABI](https://en.wikipedia.org/wiki/Application_binary_interface) bezeichnet.
Da der Linux Kernel keine stabile ABI aufweist, kann es zu Problemen kommen, wenn ein Modul eine Funktion mit falschen Parametern aufruft.

Um solche Probleme zu vermeiden wird geprüft, ob das Modul genau für den korrekten Kernel kompiliert wurde.

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
