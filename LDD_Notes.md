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

Beim Lesen werden die Daten von der Eingangs-FIFO gelesen und in den Userspace
kopiert (max. Puffergröße).

### 2. PWM-Controller

#### Schreiben
Der Schreibzugriff ist strukturiert: der erste Teil der in der write-Funktion
empfangenen Daten ist die PWM-Kanal-Nummer, der Rest ist der zu setzende Wert
dieses Kanals.

#### Lesen (optional)
Sollte es erforderlich sein, die aktuellen Werte zu lesen, werden im
einfachsten Fall die Werte der Duty-Cycle-Register gemeinsam an den Aufrufer
übermittelt.

Ist die Zahl der Kanäle zu groß, könnte zuerst nur die Kanalnummer an die
Schreib-Schnittstelle übermittelt werden (ohne Wert für den Duty-Cycle). Im
darauffolgenden Lesezugriff wird der Wert des geforderten Kanals übermittelt.

### 3. Testbild-Generator

#### Ein Konfigurationsregister
Die Bits des übertragenen Bytes haben spezielle Bedeutung (vgl.
Mikrocontroller). Dies kann in der Schreib-Schnittstelle interpretiert und in
der Lese-Schnittstelle wieder zusammengebaut werden.

Optional könnte auch eine Bitmaske übergeben werden, welche die zu beachtenden
Bits kennzeichnet.

#### Mehrere Konfigurationsregister

Ähnlich zum PWM-Controller, Strukturierung des Datenstroms in Addresse +
Registerwert.

#### großer Speicherbereich, Testbild zur Laufzeit konfigurierbar

Bereitstellung von Funktionen zum kompletten Neuschreiben des Speicherbereichs
oder gezieltem Überschreiben einer Sub-Region, gekennzeichnet durch eine
Startadresse.

Lesen ist bei dieser Schnittstelle wenig sinnvoll (Konfigurationsdaten werden
bereits vom Userspace vorgegeben). Die Ausgabe des Testbilds erfolgt
Hardwaremäßig auf anderem Weg (z.B. über HDMI, direkt aus dem FPGA heraus).

