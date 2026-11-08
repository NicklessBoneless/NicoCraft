#ifndef FRUSTUM_HH
#define FRUSTUM_HH

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

namespace fcg{
    /*
        Frustum della camera, ricavato dalla matrice proj * view (metodo di Gribb-Hartmann).
        Ogni piano e' (a, b, c, d) normalizzato: un punto e' dentro se a*x + b*y + c*z + d >= 0.
        I piani vengono dalla matrice stessa, quindi restano coerenti con cio' che si vede
        a schermo qualunque siano FOV e aspect ratio
    */
    class Frustum{
    private:
        std::array<glm::vec4, 6> planes;

    public:
        //Va chiamata una volta per frame con camera.viewProjMatrix
        void Update(const glm::mat4& projView){
            //glm e' column-major: m[colonna][riga], quindi la riga i si ricostruisce cosi'
            glm::vec4 row0(projView[0][0], projView[1][0], projView[2][0], projView[3][0]);
            glm::vec4 row1(projView[0][1], projView[1][1], projView[2][1], projView[3][1]);
            glm::vec4 row2(projView[0][2], projView[1][2], projView[2][2], projView[3][2]);
            glm::vec4 row3(projView[0][3], projView[1][3], projView[2][3], projView[3][3]);

            planes[0] = row3 + row0; //Left
            planes[1] = row3 - row0; //Right
            planes[2] = row3 + row1; //Bottom
            planes[3] = row3 - row1; //Top
            planes[4] = row3 + row2; //Near
            planes[5] = row3 - row2; //Far

            for(glm::vec4& plane : planes){
                float length = glm::length(glm::vec3(plane));
                plane /= length;
            }
        }

        //Test AABB contro i 6 piani: si prende il vertice del box piu' avanzato lungo la normale
        //del piano, se anche quello e' fuori allora tutto il box e' fuori
        bool IsBoxVisible(const glm::vec3& minCorner, const glm::vec3& maxCorner) const{
            for(const glm::vec4& plane : planes){
                glm::vec3 positiveVertex(
                    plane.x >= 0.0f ? maxCorner.x : minCorner.x,
                    plane.y >= 0.0f ? maxCorner.y : minCorner.y,
                    plane.z >= 0.0f ? maxCorner.z : minCorner.z
                );
                if(glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0.0f){
                    return false;
                }
            }
            return true;
        }
    };
}

#endif