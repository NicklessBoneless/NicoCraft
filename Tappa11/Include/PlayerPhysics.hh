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

        static constexpr float gravity = 25.0f;
        static constexpr float terminalVelocity = 50.0f;
        static constexpr float jumpSpeed = 8.0f;
        static constexpr float groundCheckEpsilon = 0.05f; //Margine della sonda per il rilevamento stabile di onGround

    public:
        PlayerPhysics(glm::vec3 startFeetPosition) : position(startFeetPosition) {}

        //'world' e' l'interfaccia astratta (vedi IWorld.hh): PlayerPhysics non sa e non le
        //interessa che dietro ci sia una World o altro, sa solo che puo' chiederle "e' solido questo blocco?"
        void UpdatePlayerPosition(float deltaTime, IWorld& world, glm::vec3 horizontalVelocity){
            if(!onGround){
                velocity.y -= gravity * deltaTime;
                if(velocity.y < -terminalVelocity) velocity.y = -terminalVelocity;
            }

            glm::vec3 delta = horizontalVelocity * deltaTime;
            delta.y = velocity.y * deltaTime;

            MoveWithCollision(delta, world);
        }

        void Jump(){
            if(onGround){
                velocity.y = jumpSpeed;
                onGround = false;
            }
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
            position.y += delta.y;
            if(CollidesAt(position, world)){
                position.y -= delta.y;
                if(delta.y < 0.0f) onGround = true; //stava cadendo ed ha toccato terra
                velocity.y = 0.0f;
            }
            else{
                //Nessuna collisione questo frame non vuol dire "in aria": se il player e' fermo
                //a terra, delta.y puo' essere 0 esatto e i piedi toccano il blocco senza overlap.
                //Sondiamo con un piccolo margine sotto i piedi per un rilevamento stabile, altrimenti
                //onGround sfarfetta true/false ogni frame e il salto risulta poco responsive.
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
    };
}

#endif
