#ifndef PLAYER_HH
#define PLAYER_HH

#include <iostream>
#include "Camera.hh"
#include "PlayerPhysics.hh"
#include "PlayerInput.hh"
#include "IWorld.hh"


namespace fcg
{
    class Player{
    private:
        Camera camera;
        fcg::PlayerPhysics physics;

        bool sprinting = false;
        bool noclip = false;
        bool breakBlock = false;
        bool placeBlock = false;

        const float REACH_DISTANCE = 6.0f;
        static constexpr float sprintFovKick = 0.5f;
        static constexpr float MinimumYaxis = -40.0f;
        static constexpr glm::vec3 spawnPosition = {40.0f,40.f,40.0f};
        static constexpr float spawnYawDeg = 0.0f;
        static constexpr float spawnPitchDeg = 0.0f;
        float moveSpeed = 4.0f;

    public:
        Player() : physics(spawnPosition){
            camera.SetPosition(physics.GetEyePosition());
        }

        Camera& getCamera(){ return camera; }
        float getReach() const{ return REACH_DISTANCE; }

        void Look(float deltaX, float deltaY){
            camera.Look(deltaX, deltaY);
        }

        void ToggleNoclip(){
            noclip = !noclip;
            if(!noclip){
                glm::vec3 feetPosition = camera.getPosition() - glm::vec3(0.0f, fcg::PlayerPhysics::eyeHeight, 0.0f);
                physics.Teleport(feetPosition);
            }
        }

        void StartSprint(){
            if(!sprinting){
                moveSpeed *= 2;
                camera.AdjustFov(sprintFovKick);
                sprinting = true;
            }
        }

        void StopSprint(){
            if(sprinting){
                sprinting = false;
                moveSpeed /= 2;
                camera.AdjustFov(-sprintFovKick);
            }
        }

        void QueueBreakBlock(){
            breakBlock = true;
        }

        //Restituisce true se un break era stato richiesto, e resetta il flag
        //(va chiamato una sola volta per frame)
        bool ConsumeBreakBlock(){
            bool request = breakBlock;
            breakBlock = false;
            return request;
        }

        void QueuePlaceBlock(){
            placeBlock = true;
        }

        bool ConsumePlaceBlock(){
            bool request = placeBlock;
            placeBlock = false;
            return request;
        }

        bool IsPlayerOccupyingBlock(int worldX, int worldY, int worldZ) const{
            return physics.OccupiesBlock(worldX, worldY, worldZ);
        }

        void UpdatePosition(float deltaTime, fcg::IWorld& world, const PlayerInput& input){
            noclip ? camera.NoClipMove(deltaTime, moveSpeed, input) : NormalMove(deltaTime, world, input);
        }

    private:
        //Metodo helper che isola il polling di SFML e l'aggiornamento fisico
        void NormalMove(float deltaTime, fcg::IWorld& world, const PlayerInput& input){
            if(input.jump){
                physics.Jump();
            }
            
            glm::vec3 horizontalVelocity = camera.getHorizontalMovement(input) * moveSpeed;
            physics.UpdatePlayerPosition(deltaTime, world, horizontalVelocity);
            CheckPlayerPosition();
            camera.SetPosition(physics.GetEyePosition());
        }

        void CheckPlayerPosition(){
            //std::cerr << "feetY=" << physics.GetFeetPosition().y << std::endl; //DEBUG temporaneo
            if(physics.GetFeetPosition().y < MinimumYaxis){
                SpawnPlayer();
            }
        }

        void SpawnPlayer(){
            physics.Teleport(spawnPosition);
            camera.SetPosition(physics.GetEyePosition());
            camera.SetOrientation(spawnYawDeg, spawnPitchDeg);
        }
    };
}

#endif
