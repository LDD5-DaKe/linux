# Verwendete Sychronisationsmechanismen

## Gerätedatei

Um nur einen gleichzeitigen Zugriff auf die Gerätedatei zu erlauben, wurde ein Mutex verwendet.
Dieser wird in open gesperrt und in release (close der Datei) wieder freigegeben.

Dadurch kann immer nur ein Thread gleichzeitig die Datei geöffnet haben.
Zum Sperren wird `mutex_lock_interruptible()` verwendet, damit die Funktion mit einem
Signal (z.B. SIGINT durch drücken von ^C) abgebrochen werden kann.

## Interrupt

Zum benachrichtigen des read-Threads bei Tastendruck wird eine waitqueue verwendet.

Zum Warten auf ein Ereignis wird `wait_event_interruptible()` verwendet, damit
kann das Warten wieder durch ein Signal abgebrochen werden.

Bei wait_event_interruptible muss eine zusätzliche Bedingung angegeben werden, die bestimmt ob das Ereignis tatsächlich aufgetreten ist.
Hier wird als Bedingung `!kfifo_is_empty()` verwendet, da der Thread warten muss, bis der Interrupt Daten in den FIFO legt.

## FIFO

Laut Dokumentation muss die kfifo bei nur einem Consumer und einem Producer nicht synchronisiert werden.
