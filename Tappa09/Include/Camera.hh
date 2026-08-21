#ifndef CAMERA_HH
#define CAMERA_HH

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include "Matrices.hh"
#include "PlayerInput.hh"

namespace fcg
{
    class Camera{
    public:
        glm::mat4 viewMatrix;
        glm::mat4 viewProjMatrix;
        glm::mat4 projMatrix;

    private:
        float fovDegrees = 70.0f;
        float aspectRatio = 1.0f;

        glm::vec3 cameraPos = {40.0f, 40.0f, 40.0f};
        float yawDeg = 0.0f;
        float pitchDeg = 0.0f;

        const float mouseSensitivity = 0.15f;

    public:
        //aspectRatio di default 1.0f: chi crea la Camera deve chiamare SetWindowSize()
        //appena conosce le dimensioni reali della finestra (vedi main() e Handle(Resized))
        Camera(){
            ViewProjection();
        }

        void SetWindowSize(int width, int height){
            aspectRatio = ((float) width) / (float) height;
            ViewProjection();
        }

        void Look(float deltaX, float deltaY){
            yawDeg += deltaX * mouseSensitivity;
            pitchDeg += deltaY * mouseSensitivity;
            pitchDeg = pitchDeg > 89.9f ? 89.9f : pitchDeg;
            pitchDeg = pitchDeg < -89.9f ? -89.9f : pitchDeg;
            ViewProjection();
        }

        float GetAspectRatio() const{
            return aspectRatio;
        }

        glm::vec3 getPosition() const{
            return cameraPos;
        }

        //Impone direttamente la posizione della camera (usato quando e' il corpo fisico a comandare il movimento)
        void SetPosition(glm::vec3 position){
            cameraPos = position;
            ViewProjection();
        }

        void SetOrientation(float yawDeg, float pitchDeg){
            Camera::yawDeg = yawDeg;
            Camera::pitchDeg = pitchDeg;
            ViewProjection();
        }

        glm::vec3 GetForward() const{
            float yawRad = glm::radians(yawDeg);
            float pitchRad = glm::radians(pitchDeg);
            return glm::vec3(
                glm::sin(yawRad) * glm::cos(pitchRad),
                -glm::sin(pitchRad),
                -glm::cos(yawRad) * glm::cos(pitchRad)
            );
        }

        //Direzione di movimento orizzontale (piano XZ) da WASD, in base allo yaw corrente.
        //Usa SOLO le prime 4 celle del binding array (W,S,D,A): Space/LControl (se presenti,
        //per il noclip) vengono ignorati a prescindere, cosi' non c'e' rischio di introdurre
        //una componente Y "fantasma" quando questa funzione viene chiamata durante la camminata normale
        glm::vec3 computeHorizontalMovement(const PlayerInput& input, bool isNoClip) const{
            float yawRad = glm::radians(yawDeg);
            glm::vec3 forward = {glm::sin(yawRad), 0.0f, -glm::cos(yawRad)};
            glm::vec3 right   = {glm::cos(yawRad), 0.0f,  glm::sin(yawRad)};
            glm::vec3 up{0.0f, 1.0f, 0.0f};
            glm::vec3 moveDirection{0.0f};

            float x, y, z;
            x = y = z = 1.0f;

            z = input.moveForward - input.moveBackward;
            x = input.moveRight - input.moveLeft;
            y = isNoClip ? input.flyUp - input.flyDown : 0.0f;

            moveDirection += (forward * z) + (right * x) + (up * y);

            if(glm::length(moveDirection) > 0.0001f){
                moveDirection = glm::normalize(moveDirection);
            }

            return moveDirection;
        }

        glm::vec3 getHorizontalMovement(const PlayerInput& input) const{
            return computeHorizontalMovement(input, false);
        }

        //Movimento libero (NOCLIP): usato solo per debug, vola in ogni direzione senza collisioni
        void NoClipMove(float deltaTime, float speed, const PlayerInput& input){
            glm::vec3 moveDirection = computeHorizontalMovement(input, true);
            if(glm::length(moveDirection) < 0.0001f) return;

            cameraPos += moveDirection * speed * deltaTime;
            ViewProjection();
        }

        //Modifica il FOV di un delta (usato per il "kick" visivo durante lo sprint)
        void AdjustFov(float delta){
            fovDegrees += delta;
            ViewProjection();
        }

        glm::mat4 ViewProjection(){
            float nearPlane = 0.1f;
            float farPlane = 100.0f;

            glm::mat4 ry = fcg::rotation_y(yawDeg);
            glm::mat4 rx = fcg::rotation_x(pitchDeg);
            glm::mat4 t  = fcg::translation(-cameraPos.x, -cameraPos.y, -cameraPos.z);

            viewMatrix = rx * ry * t;

            float perspectiveA = (farPlane + nearPlane) / (nearPlane - farPlane);
            float perspectiveB = 2.0f * farPlane * nearPlane / (nearPlane - farPlane);
            float focalDistance = 1.0f / glm::tan(glm::radians(fovDegrees / 2.0f));

            projMatrix = glm::mat4(
                focalDistance,  0.0,                     0.0,          0.0,
                0.0,            focalDistance * aspectRatio, 0.0,       0.0,
                0.0,            0.0,                     perspectiveA, -1.0,
                0.0,            0.0,                     perspectiveB,  0.0
            );

            return projMatrix * viewMatrix;
        }
    };
}

#endif
