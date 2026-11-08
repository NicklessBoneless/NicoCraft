# NicoCraft

Clone "barebones" di Minecraft sviluppato in C++ con SFML 3.0, OpenGL 4.1 e GLM, come progetto d'esame.

Il progetto è organizzato in tappe di sviluppo successive (`Tappa01` ... `Tappa20`), ciascuna compilabile ed eseguibile autonomamente. 
La `Tappa20` è la versione più completa e comprende menu principale, pausa, pannello opzioni, salvataggio delle impostazioni e un mondo di gioco più esteso, inoltre aggiunge la generazione del terreno e delle caverne.

## Build

Il progetto compila **tutte** le tappe con un unico comando, dalla root del repository:

```bash
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

`Release` può essere sostituito con `Debug` se necessario. Gli eseguibili vengono generati nella cartella `Executables/`.

Le dipendenze (SFML 3.0.2, GLM 1.0.3, Dear ImGui 1.91.9, ImGui-SFML 3.0) vengono scaricate automaticamente da CMake tramite `FetchContent`. 
Su macOS viene collegato anche `CoreGraphics`, necessario per il movimento raw del mouse.

## Esecuzione

Gli eseguibili vanno lanciati dalla cartella `Executables/` (o comunque mantenendo la struttura del repository), perché ogni tappa carica le risorse condivise con il percorso relativo `../Resources/`:

```bash
cd Executables
./Tappa10
./Tappa11
...
./Tappa20
```

**Nessuna** tappa richiede argomenti da riga di comando.

**Nota (Tappa11 e successive):** 
Al primo avvio viene creato, nella cartella da cui si lancia l'eseguibile, un file `nicocraft_settings.cfg` che salva le preferenze dell'utente (risoluzione e FOV). ù
Questo file non va versionato: la risoluzione salvata si applica al **prossimo** avvio, mentre il FOV si applica subito.

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

### Tappa11 – Tappa20 (in aggiunta/modifica rispetto a Tappa10)

Da Tappa11 sono presenti un **menu principale** e una **schermata di pausa**, navigabili col mouse (dalla Tappa13 in poi sono disegnati con ImGui):

- All'avvio compare il **Menu Principale**: *Genera Mondo* (mostra una schermata di caricamento e avvia la partita), *Opzioni* (FOV e risoluzione), *Esci*
- **Esc**, durante il gioco, apre il **menu di Pausa**: *Ritorna al gioco*, *Opzioni*, *Menu Principale*, *Esci dal gioco*. Esc in pausa riprende il gioco
- Il cursore è visibile e libero mentre un menu è attivo, e torna catturato (invisibile) durante il gioco
- La camera usa il **movimento raw del mouse** (`rawmouse.hh`), senza mouse warping

I comandi di gioco veri e propri (WASD, salto, sprint, noclip, hotbar, rompi/piazza blocco) restano identici a quelli di Tappa10.

### Tappa20 – Generazione procedurale

Il mondo non è più piatto: viene generato con **Perlin Noise** (Improved Perlin Noise, tabella di permutazione derivata dal seed), quindi lo stesso seed produce sempre lo stesso mondo.

- **Superficie**: heightmap 2D con fBm (ottave, frequenza, lacunarity, persistence, ampiezza)
- **Caverne**: sacche a "spugna" (soglia sul rumore 3D), tunnel a "vermi" (due campi 3D vicini a zero) e tunnel "spaghetti" 2D
- **Alberi**: posizionati in modo deterministico a partire da seed e coordinate del chunk
- **Pannello "Generazione terreno"** (visibile nel menu di Pausa): seed, parametri del terreno e delle caverne, pulsante *Rigenera* e tempo dell'ultima generazione in ms. Dopo la rigenerazione il player viene riportato allo spawn calcolato sull'altezza del terreno
- **Culling**: frustum culling e occlusion culling a grafo di visibilità tra sezioni 16x16x16

**Tasti aggiuntivi in gioco**
- **F3**: mostra/nasconde le statistiche di rendering (sezioni, vertici, triangoli)
- **F4**: attiva/disattiva il frustum culling
- **F5**: attiva/disattiva l'occlusion culling
- **F6**: congela/sblocca il culling (utile per ispezionare cosa viene scartato)

L'HUD mostra sempre in alto a sinistra il punto cardinale verso cui guarda il player e in alto a destra gli FPS.

I comandi di gioco veri e propri (WASD, salto, sprint, noclip, hotbar, rompi/piazza blocco) restano identici a quelli di Tappa10.
