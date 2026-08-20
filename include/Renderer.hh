#ifndef RENDERER_HH
#define RENDERER_HH

#include <string>
#include <vector>

#include "Hotshaders.hh"
#include "Matrices.hh"
#include "Blocks.hh"
#include "World.hh"
#include "Camera.hh"
#include "Crosshair.hh"
#include "BlockOutline.hh"

namespace fcg
{
    //Tutto e solo il rendering: shader del mondo, texture array, uniform, draw call.
    //Non modifica mai lo stato del mondo e non gestisce input: legge World e Camera,
    //disegna. Possiede anche Crosshair e BlockOutline, che prima venivano istanziati
    //direttamente in main()
    class Renderer{
    private:
        fcg::Shaders worldShader;
        Blocks::TextureArray textureArray;
        fcg::Crosshair crosshair;
        fcg::BlockOutline outline;

        GLint modelLoc = -1, viewLoc = -1, projLoc = -1;

    public:
        Renderer(const std::string& worldVertexFile, const std::string& worldFragmentFile,
                 const std::string& crosshairVertexFile, const std::string& crosshairFragmentFile,
                 const std::string& outlineVertexFile, const std::string& outlineFragmentFile,
                 const std::vector<std::string>& texturePaths, int texturePixelSize)
            : worldShader(worldVertexFile, worldFragmentFile),
              crosshair(crosshairVertexFile, crosshairFragmentFile),
              outline(outlineVertexFile, outlineFragmentFile)
        {
            InitializeTextures(texturePaths, texturePixelSize);
            Locations();
        }

        //Disegna un frame completo: mondo, outline del blocco puntato (se presente), crosshair
        void Draw(const World& world, Camera& camera, const RaycastHit& target){
            worldShader.use();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUniformMatrix4fv(projLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);

            textureArray.Bind(0);

            //Disegna ogni chunk traslato nella sua posizione di griglia
            for(const auto& instance : world.GetChunks()){
                glm::mat4 modelMatrix = fcg::translation(
                    instance.chunkX * Blocks::CHUNK_SIZE_X,
                    0.0f,
                    instance.chunkZ * Blocks::CHUNK_SIZE_Z
                );
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
                instance.mesh.Draw();
            }

            if(target.hit){
                outline.Draw(target.blockX, target.blockY, target.blockZ, camera.viewMatrix, camera.projMatrix);
            }

            crosshair.Draw(camera.GetAspectRatio());
        }

    private:
        void InitializeTextures(const std::vector<std::string>& texturePaths, int texturePixelSize){
            textureArray.LoadTextures(texturePaths, texturePixelSize, texturePixelSize);
        }

        void Locations(){
            worldShader.use();

            modelLoc = glGetUniformLocation(worldShader.program, "model");
            viewLoc  = glGetUniformLocation(worldShader.program, "view");
            projLoc  = glGetUniformLocation(worldShader.program, "projection");

            GLint samplerLoc = glGetUniformLocation(worldShader.program, "textureArray");
            glUniform1i(samplerLoc, 0);
        }
    };
}

#endif
