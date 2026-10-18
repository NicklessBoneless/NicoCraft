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
#include "Hotbar.hh"
#include "Frustum.hh"
#include "RenderStats.hh"

namespace fcg{
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
        fcg::HotbarRenderer hotbarRenderer;
        fcg::Sky sky;
        fcg::Frustum frustum;
        RenderStats stats;
        bool frustumCullingEnabled = true;        

        GLint viewProjModelLoc = -1, daylightLoc = -1;

        //Dimensioni finestra centralizzate: usate da Hotbar (e in futuro Compass e menu)
        int windowWidth = 1920;
        int windowHeight = 1080;
    public:
        Renderer(const std::vector<ShaderFiles>& shaderSets,
                const std::string& res, int texturePixelSize, const std::string& shaderDir) : 
            worldShader(FindShaderFiles(shaderSets, "world").vertexFile,FindShaderFiles(shaderSets, "world").fragmentFile),
            crosshair(FindShaderFiles(shaderSets, "crosshair").vertexFile,FindShaderFiles(shaderSets, "crosshair").fragmentFile),
            outline(FindShaderFiles(shaderSets, "outline").vertexFile,FindShaderFiles(shaderSets, "outline").fragmentFile),
            hotbarRenderer(res, FindShaderFiles(shaderSets, "hotbar")),
            sky(res,FindShaderFiles(shaderSets, "sky"),FindShaderFiles(shaderSets, "stars"))
        {
            InitializeTextures(res);
            Locations();
        }

        //Disegna un frame completo (LA SCENA): 
        //Cielo (Sole/Luna/stelle), Mondo, Outline del blocco puntato, crosshair, hotbar. 
        //DeltaTime fa avanzare il ciclo giorno/notte
        void Draw(const World& world, Camera& camera, const RaycastHit& target, const Hotbar& hotbar, float deltaTime){
            sky.Update(deltaTime); //Update del cielo

            glm::vec3 skyColor = sky.GetSkyColor();
            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            sky.Draw(camera); //Draw del cielo

            //Disegniamo il mondo voxel 3D
            worldShader.use();

            glUniform1f(daylightLoc, sky.GetDaylightFactor());
            textureArray.Bind(0);

            frustum.Update(camera.viewProjMatrix);
            stats = RenderStats{};

            //Disegna ogni chunk traslato nella sua posizione di griglia, saltando quelli fuori dal frustum
            for(const auto& instance : world.GetChunks()){
                const Blocks::ChunkMesh& mesh = instance.mesh;
                if(mesh.IsEmpty()) continue;

                stats.chunksTotal++;
                stats.verticesTotal += mesh.GetVertexCount();
                stats.trianglesTotal += mesh.GetTriangleCount();

                float originX = (float) (instance.chunkX * Blocks::CHUNK_SIZE_X);
                float originZ = (float) (instance.chunkZ * Blocks::CHUNK_SIZE_Z);

                if(frustumCullingEnabled){
                    glm::vec3 minCorner(originX, mesh.GetMinY(), originZ);
                    glm::vec3 maxCorner(originX + Blocks::CHUNK_SIZE_X, mesh.GetMaxY(), originZ + Blocks::CHUNK_SIZE_Z);
                    if(!frustum.IsBoxVisible(minCorner, maxCorner)) continue;
                }

                glm::mat4 chunkPvm = camera.viewProjMatrix * fcg::translation(originX, 0.0f, originZ);
                glUniformMatrix4fv(viewProjModelLoc, 1, GL_FALSE, &chunkPvm[0][0]);
                mesh.Draw();

                stats.chunksDrawn++;
                stats.verticesDrawn += mesh.GetVertexCount();
                stats.trianglesDrawn += mesh.GetTriangleCount();
            }
        

            if(target.hit){
                outline.Draw(target.blockX, target.blockY, target.blockZ, camera.viewProjMatrix);
            }

            crosshair.Draw(camera.GetAspectRatio());
            hotbarRenderer.Draw(hotbar, windowWidth, windowHeight);
        }

        //Va richiamata all'avvio e ad ogni sf::Event::Resized
        void SetWindowSize(int width, int height){
           windowWidth = width;
           windowHeight = height;
        }

        const RenderStats& GetStats() const{
            return stats;
        }

        void ToggleFrustumCulling(){
            frustumCullingEnabled = !frustumCullingEnabled;
        }

        bool IsFrustumCullingEnabled() const{
            return frustumCullingEnabled;
        }

        int GetWindowWidth() const{
           return windowWidth;
        }

        int GetWindowHeight() const{
            return windowHeight;
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

            viewProjModelLoc = glGetUniformLocation(worldShader.program, "viewProjectionModel");
            daylightLoc = glGetUniformLocation(worldShader.program, "daylightFactor");

            //Array delle texture viene passato come uniform al fragmentshader.
            GLint samplerLoc = glGetUniformLocation(worldShader.program, "textureArray");
            glUniform1i(samplerLoc, 0);
        }
    };
}

#endif
