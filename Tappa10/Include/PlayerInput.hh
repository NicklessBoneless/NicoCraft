#ifndef PLAYER_INPUT_HH
#define PLAYER_INPUT_HH

//Snapshot dell'input del giocatore per il frame corrente. Riempito da CapturePlayerInput()
//in NicoCraft.cc, letto da Camera (movimento orizzontale/noclip) e da Player (salto)
struct PlayerInput{
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool flyUp = false;   //Usato in NoClip
    bool flyDown = false; //Usato in NoClip
};

#endif
