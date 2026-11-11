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
#include "Hud.hh"
#include "Hotbar.hh"
#include "Frustum.hh"
#include "RenderStats.hh"
#include "SectionCuller.hh"

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
        fcg::Hud hud;
        fcg::Sky sky;

        //Frustum Culling
        fcg::Frustum frustum;
        RenderStats stats;
        bool frustumCullingEnabled = true;  
        
        //Section Culling
        fcg::SectionCuller sectionCuller;
        std::vector<int> visibleSections;
        bool cullingFrozen = false;
        bool occlusionCullingEnabled = true;

        GLint viewProjModelLoc = -1, daylightLoc = -1;
        GLint fogColorLoc = -1, fogStartLoc = -1, fogEndLoc = -1;

        //Dimensioni finestra centralizzate: usate da Hotbar (e in futuro Compass e menu)
        int windowWidth = 1920;
        int windowHeight = 1080;

        //Inizio e fine della nebbia come frazione del far plane della camera
        static constexpr float fogStartRatio = 0.85f;
        static constexpr float fogEndRatio = 0.95f;
    public:
        Renderer(const std::vector<ShaderFiles>& shaderSets, const std::string& res) : 
            worldShader(FindShaderFiles(shaderSets, "world").vertexFile,FindShaderFiles(shaderSets, "world").fragmentFile),
            crosshair(FindShaderFiles(shaderSets, "crosshair").vertexFile,FindShaderFiles(shaderSets, "crosshair").fragmentFile),
            outline(FindShaderFiles(shaderSets, "outline").vertexFile,FindShaderFiles(shaderSets, "outline").fragmentFile),
            hotbarRenderer(res, FindShaderFiles(shaderSets, "hotbar")),
            hud(res),
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
            glUniform3f(fogColorLoc, skyColor.r, skyColor.g, skyColor.b);
            glUniform1f(fogStartLoc, camera.getFarPlane() * fogStartRatio);
            glUniform1f(fogEndLoc, camera.getFarPlane() * fogEndRatio);
            textureArray.Bind(0);

            frustum.Update(camera.viewProjMatrix);
            stats = RenderStats{};

            if(hud.IsStatsVisible()){
                //Totali: tutte le sezioni non vuote, indipendentemente dal culling
                for(const ChunkInstance& instance : world.GetChunks()){
                    for(const Blocks::ChunkMesh& mesh : instance.meshes){
                        if(mesh.IsEmpty()) continue;
                        stats.sectionsTotal++;
                        stats.verticesTotal += mesh.GetVertexCount();
                        stats.trianglesTotal += mesh.GetTriangleCount();
                    }
                }
            }
            //Quali sezioni disegnare: BFS di visibilita' oppure solo frustum (F5 alterna)
            float renderDistance = camera.getFarPlane() * 2.0f;
            if(!cullingFrozen){
                if(occlusionCullingEnabled){
                    sectionCuller.Compute(world, camera, frustum, frustumCullingEnabled, renderDistance, visibleSections);
                }else{
                    sectionCuller.CollectAll(camera, frustum, frustumCullingEnabled, renderDistance, visibleSections);
                }
            }

            const std::vector<ChunkInstance>& chunks = world.GetChunks();
            for(int node : visibleSections){
                int sectionY = node % Blocks::SECTIONS_PER_CHUNK;
                const ChunkInstance& instance = chunks[node / Blocks::SECTIONS_PER_CHUNK];
                const Blocks::ChunkMesh& mesh = instance.meshes[sectionY];
                if(mesh.IsEmpty()) continue; //Aria o roccia piena: attraversata dalla BFS ma senza facce

                float originX = (float) (instance.chunkX * Blocks::CHUNK_SIZE_X);
                float originZ = (float) (instance.chunkZ * Blocks::CHUNK_SIZE_Z);
                glm::mat4 chunkPvm = camera.viewProjMatrix * fcg::translation(originX, 0.0f, originZ);
                glUniformMatrix4fv(viewProjModelLoc, 1, GL_FALSE, &chunkPvm[0][0]);
                mesh.Draw();

                stats.sectionsDrawn++;
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

        void UpdateHud(float yawDeg,float deltaTime){
            hud.Update(yawDeg,deltaTime);
        }

        void DrawHud(){
            hud.Draw();
            hud.DrawStats(stats,frustumCullingEnabled,occlusionCullingEnabled);
        }

        void ToggleStats(){
            hud.ToggleStats();
        }

        bool isStatsEnabled(){
            return hud.IsStatsVisible();
        }

        void ToggleCullingFreeze(){
            cullingFrozen = !cullingFrozen;
            std::cout << "Culling congelato: " << (cullingFrozen ? "ON" : "OFF") << std::endl;
        }

        void ToggleFrustumCulling(){
            frustumCullingEnabled = !frustumCullingEnabled;
        }

        bool IsFrustumCullingEnabled() const{
            return frustumCullingEnabled;
        }

        void ToggleOcclusionCulling(){
            occlusionCullingEnabled = !occlusionCullingEnabled;
        }

        bool IsOcclusionCullingEnabled() const{
            return occlusionCullingEnabled;
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

            fogColorLoc = glGetUniformLocation(worldShader.program, "fogColor");
            fogStartLoc = glGetUniformLocation(worldShader.program, "fogStart");
            fogEndLoc   = glGetUniformLocation(worldShader.program, "fogEnd");
  
            //Array delle texture viene passato come uniform al fragmentshader.
            GLint samplerLoc = glGetUniformLocation(worldShader.program, "textureArray");
            glUniform1i(samplerLoc, 0);
        }
    };
}

#endif
