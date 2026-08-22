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
#include "Sky.hh"

namespace fcg
{
    //Tutto e solo il rendering: shader del mondo, texture array, uniform, draw call.
    //Non modifica mai lo stato del mondo e non gestisce input: legge World e Camera,
    //disegna. Possiede anche Crosshair, BlockOutline e Sky (ciclo giorno/notte,
    //Sole/Luna/stelle), che prima venivano istanziati direttamente in main()
    class Renderer{
    private:
        fcg::Shaders worldShader;
        Blocks::TextureArray textureArray;
        fcg::Crosshair crosshair;
        fcg::BlockOutline outline;
        fcg::Sky sky;

        GLint modelLoc = -1, viewLoc = -1, projLoc = -1, daylightLoc = -1;

    public:
        Renderer(const std::vector<ShaderFiles>& shaderSets,
                const std::string& res, int texturePixelSize, const std::string& shaderDir) : 
            worldShader(FindShaderFiles(shaderSets, "world").vertexFile,FindShaderFiles(shaderSets, "world").fragmentFile),
            crosshair(FindShaderFiles(shaderSets, "crosshair").vertexFile,FindShaderFiles(shaderSets, "crosshair").fragmentFile),
            outline(FindShaderFiles(shaderSets, "outline").vertexFile,FindShaderFiles(shaderSets, "outline").fragmentFile),
            sky(res, shaderDir)
        {
            InitializeTextures(res);
            Locations();
        }

        //Disegna un frame completo: cielo (Sole/Luna/stelle), mondo, outline del blocco
        //puntato (se presente), crosshair. deltaTime fa avanzare il ciclo giorno/notte
        void Draw(const World& world, Camera& camera, const RaycastHit& target, float deltaTime){
            sky.Update(deltaTime);

            glm::vec3 skyColor = sky.GetSkyColor();
            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            sky.Draw(camera);

            worldShader.use();
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);
            glUniform1f(daylightLoc, sky.GetDaylightFactor());

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
        static const ShaderFiles& FindShaderFiles(const std::vector<ShaderFiles>& shaderSets, const std::string& name){
            for(const ShaderFiles& s : shaderSets){
                if(s.name == name) return s;
            }
            std::cerr << "Errore (Renderer): shader '" << name << "' non trovato tra quelle disponibili." << std::endl;
            exit(1);
        }

        void InitializeTextures(const std::string& res){
            textureArray.LoadTextures(res);
        }

        void Locations(){
            worldShader.use();

            modelLoc    = glGetUniformLocation(worldShader.program, "model");
            viewLoc     = glGetUniformLocation(worldShader.program, "view");
            projLoc     = glGetUniformLocation(worldShader.program, "projection");
            daylightLoc = glGetUniformLocation(worldShader.program, "daylightFactor");

            GLint samplerLoc = glGetUniformLocation(worldShader.program, "textureArray");
            glUniform1i(samplerLoc, 0);
        }
    };
}

#endif
