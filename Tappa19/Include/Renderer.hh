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

        //Debug (F6): disegna il frame con e senza occlusione e conta i pixel diversi
        int CountOcclusionDifferences(const World& world, Camera& camera, const RaycastHit& target, const Hotbar& hotbar){
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            size_t bufferSize = (size_t) viewport[2] * viewport[3] * 4;
            std::vector<unsigned char> withOcclusion(bufferSize);
            std::vector<unsigned char> withoutOcclusion(bufferSize);

            bool previousOcclusion = occlusionCullingEnabled;
            bool previousFreeze = cullingFrozen;
            cullingFrozen = false;

            occlusionCullingEnabled = true;
            Draw(world, camera, target, hotbar, 0.0f);
            glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, withOcclusion.data());

            occlusionCullingEnabled = false;
            Draw(world, camera, target, hotbar, 0.0f);
            glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, withoutOcclusion.data());

            occlusionCullingEnabled = previousOcclusion;
            cullingFrozen = previousFreeze;

            int differences = 0;
            for(size_t i = 0; i < bufferSize; i += 4){
                if(withOcclusion[i] != withoutOcclusion[i] || withOcclusion[i + 1] != withoutOcclusion[i + 1] || withOcclusion[i + 2] != withoutOcclusion[i + 2]){
                    differences++;
                }
            }
            return differences;
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

            //Totali: tutte le sezioni non vuote, indipendentemente dal culling
            for(const ChunkInstance& instance : world.GetChunks()){
                for(const Blocks::ChunkMesh& mesh : instance.meshes){
                    if(mesh.IsEmpty()) continue;
                    stats.sectionsTotal++;
                    stats.verticesTotal += mesh.GetVertexCount();
                    stats.trianglesTotal += mesh.GetTriangleCount();
                }
            }

            //Quali sezioni disegnare: BFS di visibilita' oppure solo frustum (F5 alterna)
            float renderDistance = camera.getFarPlane() * fogEndRatio;
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
