# Fragen

## a)

    ledpwm0: ledpwm@0xff203080 {
        compatible = "ldd,ledpwm";
        reg = <0xff203080 0x1>;
    };

Die Syntax um ein neues Gerät im Device Tree zu erzeugen ist wie folgt:
`label: unique_name` typischerweise wird für den einzigartigen Namen die Adresse des Geräts.
In dem Eintrag sind auf jeden Fall die folgenden Felder notwendig
- compatible: Zeigt an, welcher Treiber dieses Gerät bedienen kann
- reg: Liste von Adresse/Länge paaren von Registeradressen, die dieses Gerät verwendet.

Die Einträge wurden unter den Knoten `soc` gesetzt, dieser spiegelt das Bussystem des SoCs wieder.

## b)

Die meisten SoC Systeme verwenden Bussysteme, bei denen angeschlossene Geräte nicht automatisch erkannt werden können (z.B. AXI).

Es benötigt daher einen Mechanismus um im Kernel festzustellen, welche Geräte an welchen Adressen vorhanden sind, sowie welche Treiber dafür benötigt werden.

Bevor Device Trees dafür verwendet wurden, wurde diese Konfiguration in Form von Sourcecode durchgeführt. Das bedeutet aber, dass für jedes SoC System ein bestimmter Kernel kompiliert werden muss.

Mit Device Trees wird diese Konfiguration in eine separate Datei ausgelagert,
die dem Kernel beim starten vom Bootloader mitgegeben wird, dadurch kann der
selbe Kernel für mehrere SoC Systeme verwendet werden, lediglich die
Device-Tree Datei muss angepasst werden.
