# zu Aufgabe 1

## b) Behandlung von Werten ausserhalb des Wertebereichs

Die Behandlung von Werten ausserhalb des Wertebereichs ist gleich wie
bei Übung 2 Mal:

> Wird ein Wert ausserhalb des Wertebereichs erhalten (> 100), wird
die Ausgabe der Werte abgebrochen. Die LED wird auf einen sicheren
Wert (0) zurückgesetzt und dem Userspace wird der Fehlercode `EINVAL`
rückgegeben.

> Diese Herangehensweise wurde gewählt, um den Benutzer möglichst laut und
rasch auf eine falsche Eingabe hinzuweisen.

> Das Rücksetzen auf einen sicheren Wert ist je nach tatsächlicher Anwendung
sinnvoll oder gefährlich (z.B. bei der Ansteuerung eines Servo-Motors).

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
