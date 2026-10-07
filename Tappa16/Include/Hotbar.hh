#ifndef HOTBAR_HH
#define HOTBAR_HH

#include <SFML/Graphics/Image.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "Hotshaders.hh"
#include "Blocks.hh"

namespace fcg{

    //Quale BlockType occupa lo slot: deciso una volta all'avvio, non cambia mai
    struct HotbarSlotDefinition{
        Blocks::BlockType blockType;
    };

    struct HotbarSlot{
        HotbarSlotDefinition definition;
    };

    /*
        Stato puro della Hotbar: quali blocchi contiene e quale slot e' selezionato.
        Nessuna dipendenza da OpenGL: la parte grafica vive in HotbarRenderer, piu' in basso in questo file
    */
    class Hotbar{
    private:
        std::vector<HotbarSlot> slots;
        int selectedIndex = 0;

    public:
        Hotbar(){
            slots = {
                {{Blocks::BlockType::GRASS}},
                {{Blocks::BlockType::DIRT}},
                {{Blocks::BlockType::STONE}},
                {{Blocks::BlockType::PLANK}},
                {{Blocks::BlockType::LOGWOOD}},
                {{Blocks::BlockType::LEAVES}},
                {{Blocks::BlockType::GLASS}}
            };
        }

        int GetSlotCount() const{
            return (int) slots.size();
        }

        Blocks::BlockType GetBlockTypeAt(int index) const{
            return slots[index].definition.blockType;
        }

        int GetSelectedIndex() const{
            return selectedIndex;
        }

        Blocks::BlockType GetSelectedBlockType() const{
            return slots[selectedIndex].definition.blockType;
        }

        //Seleziona lo slot 'index' (0-based). Richiesta ignorata se fuori range
        void SetSelected(int index){
            if(index >= 0 && index < (int) slots.size()){
                selectedIndex = index;
            }
        }

        void ScrollSelected(int delta){
            int count = (int) slots.size();
            selectedIndex = ((selectedIndex + delta) % count + count) % count; //Modulo "sicuro" anche per delta negativi
        }
    };

    /*
        Disegna la Hotbar in basso allo schermo (icona isometrica pseudo-3D per slot).
        Vista OpenGL Core: possiede shader, texture e VAO, ma non lo stato. Legge lo
        stato da un oggetto Hotbar passato a Draw(), non lo possiede e non lo modifica mai
    */
    class HotbarRenderer{
    private:
        Shaders shader;
        GLuint vao = 0, vbo = 0;
        GLint screenSizeLoc = -1, tintLoc = -1;

        GLuint slotTexture = 0;
        GLuint slotSelectedTexture = 0;
        std::map<Blocks::BlockType, GLuint> topTextures;
        std::map<Blocks::BlockType, GLuint> sideTextures;
        std::vector<GLuint> ownedTextures; //Tutte le texture create, per il Cleanup

        static constexpr float slotSize = 72.0f;
        static constexpr float slotPadding = 0.0f;
        static constexpr float slotMargin = 20.0f;  //Distanza dal bordo inferiore dello schermo
        static constexpr float iconMargin = 10.0f;  //Margine tra il bordo dello slot e l'icona

    public:
        //resourcesDir: path relativo alle risorse (es. "../Resources/")
        HotbarRenderer(const std::string& resourcesDir, const ShaderFiles& shaderFiles) :
            shader(shaderFiles.vertexFile, shaderFiles.fragmentFile)
        {
            BuildQuadBuffer();
            Locations();
            LoadTextures(resourcesDir);
        }

        ~HotbarRenderer(){
            Cleanup();
        }

        HotbarRenderer(const HotbarRenderer&) = delete;
        HotbarRenderer& operator=(const HotbarRenderer&) = delete;

        void Draw(const Hotbar& hotbar, int windowWidth, int windowHeight){
            //Overlay 2D: niente depth test, niente culling (l'asse Y invertito ribalta l'avvolgimento), blending attivo
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            shader.use();
            glUniform2f(screenSizeLoc, (float) windowWidth, (float) windowHeight);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(vao);

            int slotCount = hotbar.GetSlotCount();
            float totalWidth = slotCount * slotSize + (slotCount - 1) * slotPadding;
            float startX = ((float) windowWidth - totalWidth) * 0.5f;
            float startY = (float) windowHeight - slotSize - slotMargin;

            for(int i = 0; i < slotCount; i++){
                float x = startX + i * (slotSize + slotPadding);
                bool isSelected = (i == hotbar.GetSelectedIndex());

                DrawSlotBackground(x, startY, isSelected);

                float iconSize = slotSize - iconMargin * 2.0f;
                DrawIsoBlock(hotbar.GetBlockTypeAt(i), x + iconMargin, startY + iconMargin, iconSize);
            }

            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            //Ripristino dello stato usato dal rendering 3D
            glDisable(GL_BLEND);
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
        }

    private:
        //Buffer dinamico con 6 vertici (2 triangoli) da 4 float ciascuno: posizione xy + uv
        void BuildQuadBuffer(){
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }

        void Locations(){
            shader.use();
            screenSizeLoc = glGetUniformLocation(shader.program, "screenSize");
            tintLoc = glGetUniformLocation(shader.program, "tint");
            glUniform1i(glGetUniformLocation(shader.program, "hotbarTexture"), 0);
        }

        //Carica un PNG (decodificato da sf::Image) in una GL_TEXTURE_2D. Ritorna 0 se anche il fallback fallisce
        GLuint LoadTexture(const std::string& resourcesDir, const std::string& fileName){
            sf::Image image;
            if(!image.loadFromFile(resourcesDir + fileName)){
                std::cerr << "Errore nel caricamento di " << fileName << std::endl;
                if(!image.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                    return 0;
                }
            }

            sf::Vector2u size = image.getSize();

            GLuint textureID = 0;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) size.x, (GLsizei) size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

            //Filtraggio pixel-art, nessuna ripetizione
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0);
            ownedTextures.push_back(textureID);
            return textureID;
        }

        void LoadTextures(const std::string& resourcesDir){
            slotTexture = LoadTexture(resourcesDir, "hotbarSlot.png");
            slotSelectedTexture = LoadTexture(resourcesDir, "hotbarSelected.png");

            LoadBlockTexture(resourcesDir, Blocks::BlockType::GRASS,  "grassTop.png", "grassSide.png");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::DIRT,   "dirt.png",     "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::STONE,  "stone.png",    "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::PLANK,  "woodplank.png","");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::LOGWOOD,"logTop.png",   "logSide.png");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::LEAVES, "leaves.png",   "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::GLASS,  "glass.png",    "");
        }

        void LoadBlockTexture(const std::string& resourcesDir, Blocks::BlockType type,
                              const std::string& topFile, const std::string& sideFile){
            GLuint top = LoadTexture(resourcesDir, topFile);
            topTextures[type] = top;

            //Senza file laterale, stessa texture (e stesso id) su top e side
            sideTextures[type] = (sideFile == "") ? top : LoadTexture(resourcesDir, sideFile);
        }

        //Disegna un quad texturato: i 4 angoli della texture sono mappati uno-a-uno sui 4 vertici dati
        void DrawQuad(GLuint texture, glm::vec4 tint, glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3){
            float vertices[] = {
                v0.x, v0.y, 0.0f, 0.0f,
                v1.x, v1.y, 1.0f, 0.0f,
                v2.x, v2.y, 1.0f, 1.0f,
                v0.x, v0.y, 0.0f, 0.0f,
                v2.x, v2.y, 1.0f, 1.0f,
                v3.x, v3.y, 0.0f, 1.0f
            };

            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform4f(tintLoc, tint.r, tint.g, tint.b, tint.a);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        void DrawSlotBackground(float x, float y, bool isSelected){
            GLuint texture = isSelected ? slotSelectedTexture : slotTexture;
            DrawQuad(texture, glm::vec4(1.0f),
                     {x, y}, {x + slotSize, y}, {x + slotSize, y + slotSize}, {x, y + slotSize});
        }

        /*
            Disegna un piccolo cubo isometrico dentro il quadrato (x,y,size): una losanga superiore
            (faccia Top) e due parallelogrammi sotto (facce laterali), a formare un esagono.
            Rapporto isometrico 2:1, come nei classici tile-renderer isometrici
        */
        void DrawIsoBlock(Blocks::BlockType type, float x, float y, float size){
            float halfW = size * 0.5f;
            float diamondH = size * 0.5f; //Altezza della losanga superiore
            float sideH = size * 0.5f;    //Altezza delle facce laterali

            glm::vec2 top(x + halfW, y);
            glm::vec2 right(x + size, y + diamondH * 0.5f);
            glm::vec2 center(x + halfW, y + diamondH);
            glm::vec2 left(x, y + diamondH * 0.5f);
            glm::vec2 leftBottom(left.x, left.y + sideH);
            glm::vec2 centerBottom(center.x, center.y + sideH);
            glm::vec2 rightBottom(right.x, right.y + sideH);

            GLuint topTexture = topTextures.at(type);
            GLuint sideTexture = sideTextures.at(type);

            //Faccia superiore: piena luminosita'
            DrawQuad(topTexture, glm::vec4(1.0f), top, right, center, left);

            //Faccia laterale sinistra: tinta piu' scura (luce simulata)
            DrawQuad(sideTexture, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f), left, center, centerBottom, leftBottom);

            //Faccia laterale destra: tinta intermedia
            DrawQuad(sideTexture, glm::vec4(0.75f, 0.75f, 0.75f, 1.0f), center, right, rightBottom, centerBottom);
        }

        void Cleanup(){
            for(GLuint textureID : ownedTextures){
                glDeleteTextures(1, &textureID);
            }
            ownedTextures.clear();

            if(vao){
                glDeleteVertexArrays(1, &vao);
                vao = 0;
            }
            if(vbo){
                glDeleteBuffers(1, &vbo);
                vbo = 0;
            }
        }
    };
}

#endif