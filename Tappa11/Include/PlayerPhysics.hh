#ifndef PLAYER_PHYSICS_HH
#define PLAYER_PHYSICS_HH

#include <glm/vec3.hpp>
#include <cmath>
#include "IWorld.hh"

namespace fcg{
    //Corpo fisico del player: gestisce gravita' e collisioni AABB contro il mondo voxel.
    //Separato dalla Camera: la Camera si limita a leggere la posizione occhi da qui.
    class PlayerPhysics{
    public:
        //Pubbliche perche' servono anche fuori (es. Player per riallineare la camera dopo il noclip)
        static constexpr float halfWidth = 0.3f;   //Meta' larghezza AABB (X e Z)
        static constexpr float height = 1.8f;
        static constexpr float eyeHeight = 1.6f;

    private:
        glm::vec3 position; //Piedi del player, coordinate mondo
        glm::vec3 velocity = {0.0f, 0.0f, 0.0f};
        bool onGround = false;
        bool isCeiling = false;

        static constexpr float gravity = 25.0f;
        static constexpr float terminalVelocity = 50.0f;
        static constexpr float jumpSpeed = 8.0f;
        static constexpr float groundCheckEpsilon = 0.05f; //Margine della sonda per il rilevamento stabile di onGround
        static constexpr float ceilingCheckEpsilon = 0.2f; //Margine della sonda per il rilevamento stabile del soffitto

    public:
        PlayerPhysics(glm::vec3 startFeetPosition) : position(startFeetPosition) {}

        //'world' e' l'interfaccia astratta (vedi IWorld.hh): PlayerPhysics non sa e non le
        //interessa che dietro ci sia una World o altro, sa solo che puo' chiederle "e' solido questo blocco?"
        void UpdatePlayerPosition(float deltaTime, IWorld& world, glm::vec3 horizontalVelocity){
            if(!onGround){
                velocity.y -= gravity * deltaTime;
                if(-velocity.y > terminalVelocity) velocity.y = -terminalVelocity;
            }

            glm::vec3 delta = horizontalVelocity * deltaTime;
            delta.y = velocity.y * deltaTime;

            MoveWithCollision(delta, world);
        }

        void Jump(IWorld& world){
            if(!onGround) return;
            float headroom = GetHeadroom(world);
            // Se lo spazio sopra la testa è inferiore a 0.25 blocchi (es. tunnel 2x1)
            if (headroom < 0.2f) {
                //Impostiamo una velocità iniziale ridotta proporzionale allo spazio rimasto,
                //sufficiente per dare una sensazione di molla/spinta dolce senza impattare violentemente.
                velocity.y = std::min(jumpSpeed * 0.25f, std::sqrt(2.0f * gravity * headroom * 0.8f));
            } else {
                velocity.y = jumpSpeed;
            }
            onGround = false;  
        }

        glm::vec3 GetEyePosition() const{
            return position + glm::vec3(0.0f, eyeHeight, 0.0f);
        }

        glm::vec3 GetFeetPosition() const{
            return position;
        }

        bool IsPlayerOnGround() const{
            return onGround;
        }

        //Riposiziona il corpo fisico senza passare dalla gravita' (usato quando si esce dal noclip)
        void Teleport(glm::vec3 newFeetPosition){
            position = newFeetPosition;
            velocity = {0.0f, 0.0f, 0.0f};
            onGround = false;
        }

        bool OccupiesBlock(int worldX, int worldY, int worldZ) const{
            int minX = (int) std::floor(position.x - halfWidth);
            int maxX = (int) std::floor(position.x + halfWidth);
            int minY = (int) std::floor(position.y);
            int maxY = (int) std::floor(position.y + height);
            int minZ = (int) std::floor(position.z - halfWidth);
            int maxZ = (int) std::floor(position.z + halfWidth);

            bool X = (worldX >= minX && worldX <= maxX);
            bool Y = (worldY >= minY && worldY <= maxY);
            bool Z = (worldZ >= minZ && worldZ <= maxZ);

            return X && Y && Z;
        }

    private:
        //Muove il player un asse alla volta, testando le collisioni dopo ogni spostamento parziale
        void MoveWithCollision(glm::vec3 delta, IWorld& world){
            //Asse X
            position.x += delta.x;
            if(CollidesAt(position, world)) position.x -= delta.x;

            //Asse Y
            isCeiling = false;
            if(delta.y > 0.0f){
                position.y += delta.y;
                if (CollidesAt(position, world)) {
                    // Invece di fare il semplice rollback e azzerare d'impatto la velocità,
                    // riallineiamo la posizione dei piedi al limite massimo consentito dal soffitto.
                    position.y -= delta.y; // Annulla lo spostamento
                    velocity.y = 0.0f;     // Azzera la velocità verticale
                    isCeiling = true;
                }
            }else if(delta.y < 0.0f) {
                position.y += delta.y;
                if (CollidesAt(position, world)) {
                    position.y -= delta.y;
                    velocity.y = 0.0f;
                    onGround = true;
                }else onGround = false;
            }else{
                glm::vec3 groundProbePosition = position;
                groundProbePosition.y -= groundCheckEpsilon;
                onGround = CollidesAt(groundProbePosition, world);
            }

            //Asse Z
            position.z += delta.z;
            if(CollidesAt(position, world)) position.z -= delta.z;
        }

        //Controlla se l'AABB del player (piedi in 'feetPosition') interseca un blocco solido.
        //Itera su tutte le celle intere comprese tra min e max del box: di solito poche celle.
        bool CollidesAt(glm::vec3 feetPosition, IWorld& world) const{
            int minX = (int) std::floor(feetPosition.x - halfWidth);
            int maxX = (int) std::floor(feetPosition.x + halfWidth);
            int minY = (int) std::floor(feetPosition.y);
            int maxY = (int) std::floor(feetPosition.y + height);
            int minZ = (int) std::floor(feetPosition.z - halfWidth);
            int maxZ = (int) std::floor(feetPosition.z + halfWidth);

            for(int x = minX; x <= maxX; x++){
                for(int y = minY; y <= maxY; y++){
                    for(int z = minZ; z <= maxZ; z++){
                        if(world.IsSolidAtWorld(x, y, z)) return true;
                    }
                }
            }
            return false;
        }

        float GetHeadroom(IWorld& world) const {
            // Effettuiamo un test veloce: se a 0.2 unità sopra la testa c'è già collisione,
            // misuriamo lo spazio libero a passi di 0.05f.
            glm::vec3 testPos = position;
            testPos.y += ceilingCheckEpsilon;

            if(!CollidesAt(testPos, world)){
                return ceilingCheckEpsilon; // C'è abbastanza spazio per un salto normale o parziale senza urtare subito
            }

            // Se c'è collisione a 0.2f, troviamo la massima distanza percorribile senza compenetrazione
            float step = 0.05f;
            float currentOffset = 0.0f;

            while (currentOffset + step < 0.2f) {
                glm::vec3 probe = position;
                probe.y += currentOffset + step;
                if (CollidesAt(probe, world)) {
                    break;
                }
                currentOffset += step;
            }

            return currentOffset;
        }
    };
}

#endif
