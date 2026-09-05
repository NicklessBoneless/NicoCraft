# NicoCraft

Clone "barebones" di Minecraft sviluppato in C++ con SFML 3.0, OpenGL 4.1 e GLM, come progetto d'esame.

Il progetto è organizzato in tappe di sviluppo successive (`Tappa01` ... `Tappa11`), ciascuna compilabile ed eseguibile autonomamente. `Tappa11` è la versione più completa e comprende menu principale, pausa, pannello opzioni, salvataggio delle impostazioni e un mondo di gioco più esteso.

## Build

Il progetto compila **tutte** le tappe con un unico comando, dalla root del repository:

```bash
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

`Release` può essere sostituito con `Debug` se necessario. Gli eseguibili vengono generati nella cartella `build/`.

## Esecuzione

Gli eseguibili vanno lanciati dalla cartella `build/` (o comunque mantenendo la struttura del repository), perché ogni tappa carica le risorse condivise con il percorso relativo `../Resources/`:

```bash
cd build
./Tappa01
./Tappa02
...
./Tappa11
```

Nessuna tappa richiede argomenti da riga di comando.

**Nota su Tappa11:** al primo avvio crea, nella cartella da cui viene lanciato l'eseguibile, un file `nicocraft_settings.cfg` che salva le preferenze dell'utente (risoluzione e FOV). Questo file non va versionato: la risoluzione salvata si applica al **prossimo** avvio, mentre il FOV si applica subito.

## Comandi

### Tappa01 – Tappa10

**Mouse**
- **Movimento del mouse**: ruota la visuale della camera (cursore catturato dalla finestra, stile FPS — non serve tenere premuto alcun tasto)
- **Tasto sinistro**: rompe il blocco puntato
- **Tasto destro**: piazza il blocco selezionato nella hotbar
- **Rotellina**: cambia lo slot selezionato nella hotbar

**Tastiera**
- **W / A / S / D**: muove il player avanti / sinistra / indietro / destra
- **Spazio**: salta (in noclip: vola verso l'alto)
- **LCtrl**: vola verso il basso (solo in noclip)
- **LShift** (tenuto premuto): sprint, aumenta la velocità di movimento
- **F**: attiva/disattiva il noclip (volo libero senza collisioni)
- **1-7**: seleziona lo slot corrispondente della hotbar
- **Esc**: chiude il programma

### Tappa11 (in aggiunta/modifica rispetto a Tappa10)

Tappa11 introduce un **menu principale** e una **schermata di pausa**, entrambi navigabili col mouse:

- All'avvio compare il **Menu Principale**: *Genera Mondo* (avvia la partita), *Opzioni* (FOV e risoluzione), *Esci*
- **Esc**, durante il gioco, non chiude più il programma ma apre il **menu di Pausa**: *Ritorna al gioco*, *Opzioni*, *Menu Principale*, *Esci dal gioco*
- Nel pannello **Opzioni** (raggiungibile sia dal menu principale che dalla pausa): pulsanti `-`/`+` per il FOV e `<`/`>` per la risoluzione, più *Indietro*
- Tutte le interazioni con i menu avvengono con il **tasto sinistro del mouse**; il cursore è visibile e libero mentre un menu è attivo, e torna catturato (invisibile) durante il gioco

I comandi di gioco veri e propri (WASD, salto, sprint, noclip, hotbar, rompi/piazza blocco) restano identici a quelli di Tappa10.
