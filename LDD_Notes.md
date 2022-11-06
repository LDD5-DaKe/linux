Notizen fuer Uebung02
===

## 1. Character Devices
### c) Behandlung von Werten ausserhalb des Wertebereichs

Wird ein Wert ausserhalb des Wertebereichs erhalten (> 100), wird
die Ausgabe der Werte abgebrochen. Die LED wird auf einen sicheren
Wert (0) zurückgesetzt und dem Userspace wird der Fehlercode `EINVAL`
rückgegeben.

Diese Herangehensweise wurde gewählt, um den Benutzer möglichst laut und
rasch auf eine falsche Eingabe hinzuweisen.

Das Rücksetzen auf einen sicheren Wert ist je nach tatsächlicher Anwendung
sinnvoll oder gefährlich (z.B. bei der Ansteuerung eines Servo-Motors).

## 3. Verständnisfragen
### 1. UART-Controller

Die zu kommunizierten Daten werden direkt über die Character-Device
Schnittstelle ausgetauscht.

Beim Schreiben werden die Daten an die Ausgangs-FIFO angefügt. Sollte die FIFO
voll sein, kann der Fehlercode `EBUSY` retourniert werden, sodass der
Schreibvorgang später wiederholt wird.
Alternativ kann der Treiber auch blockieren, bis Platz im FIFO ist.

Beim Lesen werden die Daten von der Eingangs-FIFO gelesen und in den Userspace
kopiert (max. Puffergröße).

### 2. PWM-Controller

Da hier viele separate Register mit geringer Datenmenge angesprochen werden
müssen bietet sich eine sysfs Schnittstelle an.

Es wird für jeden PWM Kanal ein file im sysfs erstellt, das die PWM Einstellung
dieses Kanals erlaubt. Sind die Register des PWM Controllers auch lesbar, kann
auch das Lesen der Register über sysfs implementiert werden.

### 3. Testbild-Generator

#### Ein Konfigurationsregister

Hier bietet sich wieder eine sysfs Schnittstelle an. Für einfaches Arbeiten mit
der Schnittstelle, wird jedes zusammengehörige Feld des Registers als separate
Datei dargestellt. Für schnellere Zugriffe kann auch noch eine Datei für das
gesamte Register erstellt werden, dem der Wert in hex geschrieben wird um
Menschenlesbar zu sein.

#### Mehrere Konfigurationsregister

Wie bei einem Register, allerdings kann pro Register ein Unterordner im sysfs angelegt werden.

#### großer Speicherbereich, Testbild zur Laufzeit konfigurierbar

Der Speicherbereich für das Testbild wird als Character Device implementiert,
dadurch kann die Bildatei einfach verändert werden und auch schnelles streaming
wäre möglich.

Für etwaige Kontroll und Konfigurationsregister, die zusätzlich existieren,
werden sysfs Dateien implementiert, damit müssen diese Zugriffe nicht über die
gleiche Schnittstelle wie die Bildatei gesendet werden.
