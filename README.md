# NicoCraft
Clone "barebones" di Minecraft sviluppato in C++ con SFML 3.0, OpenGL 4.1 e GLM, come progetto d'esame.

## Build
Il progetto compila tutte le tappe con un unico comando, dalla root del repository:

```bash
cmake -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```
Gli eseguibili vengono generati nella cartella `build/`.

## Esecuzione
### Tappa01 
- Nessun argomento da riga di comando richiesto.

```bash
./build/Tappa01
```

## Comandi
### Mouse
- **Tasto sinistro (tenuto premuto) + movimento**: ruota la visuale della camera (look-around)

### Tastiera
- **W / A / S / D**: muove la camera in avanti / sinistra / indietro / destra
- **LShift**: "Sprinta", aumenta la velocità della camera
- **Esc**: chiude il programma

## Struttura del progetto ---
NicoCraft/
├── CMakeLists.txt
├── include/ # header condivisi tra le tappe (matrici, shader loader...)
├── Tappa01/ # sorgenti e shader della Tappa01
└── Tappa02/ # sorgenti e shader della Tappa02
....

## Dipendenze

- SFML 3.0.2 (finestra e input)
- OpenGL 4.1 (rendering)
- glad 2.0.8 (loader OpenGL, incluso in `include/glad/`)
- GLM 1.0.3 (algebra lineare)

Tutte le dipendenze sono scaricate automaticamente da CMake tramite `FetchContent` nel CMakeLists.
