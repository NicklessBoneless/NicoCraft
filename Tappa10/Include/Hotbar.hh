#ifndef HOTBAR_HH
#define HOTBAR_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "Blocks.hh"

namespace fcg{

    //Quale BlockType occupa lo slot: deciso una volta all'avvio, non cambia mai
    struct HotbarSlotDefinition{
        Blocks::BlockType blockType;
    };

    //Stato runtime di un singolo slot (per ora solo la definizione statica, pronto per espansioni future)
    struct HotbarSlot{
        HotbarSlotDefinition definition;
    };

    //Hotbar in basso allo schermo: 5 slot selezionabili da tastiera (Num1-Num5), ognuno mostra
    //un'icona isometrica pseudo-3D del blocco. Disegnata via sf::RenderWindow, mixata con OpenGL
    //tramite pushGLStates()/popGLStates() (chiamati fuori da questa classe, in NicoCraft.cc)
    class Hotbar{
    private:
        std::vector<HotbarSlot> slots;
        int selectedIndex = 0;

        sf::Texture slotTexture;
        sf::Texture slotSelectedTexture;
        std::map<Blocks::BlockType, sf::Texture> topTextures;
        std::map<Blocks::BlockType, sf::Texture> sideTextures;

        int windowWidth = 1920;
        int windowHeight = 1080;

        static constexpr float slotSize = 72.0f;
        static constexpr float slotPadding = 0.0f;
        static constexpr float slotMargin = 20.0f;  //Distanza dal bordo inferiore dello schermo
        static constexpr float iconMargin = 10.0f;   //Margine tra il bordo dello slot e l'icona

    public:
        //resourcesDir e' il path relativo alle risorse (es. "../Resources/"), stesso usato per le altre texture
        Hotbar(const std::string& resourcesDir){
            slots = {
                {{Blocks::BlockType::GRASS}},
                {{Blocks::BlockType::DIRT}},
                {{Blocks::BlockType::STONE}},
                {{Blocks::BlockType::PLANK}},
                {{Blocks::BlockType::LOGWOOD}},
                {{Blocks::BlockType::LEAVES}},
                {{Blocks::BlockType::GLASS}}
            };

            LoadTextures(resourcesDir);
        }

        //Va richiamata all'avvio (come per Camera) e ad ogni Resized
        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
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

        void Draw(sf::RenderWindow& window){
            float slotCount = (float) slots.size();
            float totalWidth = slotCount * slotSize + (slotCount - 1) * slotPadding;
            float startX = ((float) windowWidth - totalWidth) * 0.5f;
            float startY = (float) windowHeight - slotSize - slotMargin;

            for(int i = 0; i < (int) slots.size(); i++){
                float x = startX + i * (slotSize + slotPadding);
                bool isSelected = (i == selectedIndex);

                DrawSlotBackground(window, x, startY, isSelected);

                float iconSize = slotSize - iconMargin * 2.0f;
                DrawIsoBlock(window, slots[i].definition.blockType, x + iconMargin, startY + iconMargin, iconSize);
            }
        }

    private:
        void LoadTextures(const std::string& resourcesDir){
            if(!slotTexture.loadFromFile(resourcesDir + "hotbarSlot.png")){
                std::cerr << "Errore nel caricamento di slot.png" << std::endl;
                if(!slotTexture.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                }
            }
            if(!slotSelectedTexture.loadFromFile(resourcesDir + "hotbarSelected.png")){
                std::cerr << "Errore nel caricamento di slotSelected.png" << std::endl;
                if(!slotSelectedTexture.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                }
            }

            //Top/side per ciascun blocco mostrato in hotbar (stessa logica di getTextureIndex in Blocks.hh,
            //ma qui servono come sf::Texture separate, non nella TextureArray OpenGL del mondo)
            LoadBlockTexture(resourcesDir, Blocks::BlockType::GRASS,  "grassTop.png", "grassSide.png");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::DIRT,   "dirt.png",     "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::STONE,  "stone.png",    "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::PLANK,  "woodplank.png",   "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::LOGWOOD,"logTop.png",   "logSide.png");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::LEAVES, "leaves.png",   "");
            LoadBlockTexture(resourcesDir, Blocks::BlockType::GLASS,  "glass.png",   "");
        }

        void LoadBlockTexture(const std::string& resourcesDir, Blocks::BlockType type,
                               const std::string& topFile, const std::string& sideFile){
            sf::Texture top, side;
            if(!top.loadFromFile(resourcesDir + topFile)){
                std::cerr << "Errore nel caricamento di " << topFile << std::endl;
                if(!top.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                }
            }
            if(sideFile == ""){
                topTextures[type] = top;
                sideTextures[type] = top;
                return;
            }
            if(!side.loadFromFile(resourcesDir + sideFile)){
                std::cerr << "Errore nel caricamento di " << sideFile << std::endl;
                if(!side.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                }
            }
            topTextures[type] = top;
            sideTextures[type] = side;
        }

        //Quad texturato con slot.png o slotSelected.png a seconda dello stato
        void DrawSlotBackground(sf::RenderWindow& window, float x, float y, bool isSelected){
            const sf::Texture& texture = isSelected ? slotSelectedTexture : slotTexture;
            sf::Vector2f texSize(texture.getSize());

            sf::Vertex vertices[6] = {
                sf::Vertex({x, y},                     sf::Color::White, {0.0f, 0.0f}),
                sf::Vertex({x + slotSize, y},           sf::Color::White, {texSize.x, 0.0f}),
                sf::Vertex({x + slotSize, y + slotSize}, sf::Color::White, {texSize.x, texSize.y}),
                sf::Vertex({x, y},                     sf::Color::White, {0.0f, 0.0f}),
                sf::Vertex({x + slotSize, y + slotSize}, sf::Color::White, {texSize.x, texSize.y}),
                sf::Vertex({x, y + slotSize},           sf::Color::White, {0.0f, texSize.y})
            };

            window.draw(vertices, 6, sf::PrimitiveType::Triangles, sf::RenderStates(&texture));
        }

        //Disegna un piccolo cubo isometrico dentro il quadrato (x,y,size): una losanga superiore
        //(faccia Top) e due parallelogrammi sotto (facce laterali), a formare un esagono.
        //Rapporto isometrico 2:1, come nei classici tile-renderer isometrici
        void DrawIsoBlock(sf::RenderWindow& window, Blocks::BlockType type, float x, float y, float size){
            float halfW = size * 0.5f;
            float diamondH = size * 0.5f; //Altezza della losanga superiore
            float sideH = size * 0.5f;    //Altezza delle facce laterali

            sf::Vector2f top(x + halfW, y);
            sf::Vector2f right(x + size, y + diamondH * 0.5f);
            sf::Vector2f center(x + halfW, y + diamondH);
            sf::Vector2f left(x, y + diamondH * 0.5f);
            sf::Vector2f leftBottom(left.x, left.y + sideH);
            sf::Vector2f centerBottom(center.x, center.y + sideH);
            sf::Vector2f rightBottom(right.x, right.y + sideH);

            const sf::Texture& topTex = topTextures.at(type);
            const sf::Texture& sideTex = sideTextures.at(type);

            //Faccia superiore: piena luminosita', texture "top" del blocco
            DrawIsoFace(window, topTex, sf::Color::White, top, right, center, left);

            //Faccia laterale sinistra: texture "lato", tinta piu' scura (luce simulata)
            DrawIsoFace(window, sideTex, sf::Color(140, 140, 140), left, center, centerBottom, leftBottom);

            //Faccia laterale destra: texture "lato", tinta intermedia
            DrawIsoFace(window, sideTex, sf::Color(190, 190, 190), center, right, rightBottom, centerBottom);
        }

        //Disegna un parallelogramma texturizzato (2 triangoli, dato che SFML 3 non ha piu' la
        //primitiva Quads) mappando i 4 angoli della texture uno-a-uno sui 4 angoli dati: e' proprio
        //questa corrispondenza "quadrato -> parallelogramma" a produrre lo scorcio isometrico
        void DrawIsoFace(sf::RenderWindow& window, const sf::Texture& texture, sf::Color tint,
                          sf::Vector2f v0, sf::Vector2f v1, sf::Vector2f v2, sf::Vector2f v3){
            sf::Vector2f texSize(texture.getSize());

            sf::Vertex vertices[6] = {
                sf::Vertex(v0, tint, sf::Vector2f(0.0f, 0.0f)),
                sf::Vertex(v1, tint, sf::Vector2f(texSize.x, 0.0f)),
                sf::Vertex(v2, tint, sf::Vector2f(texSize.x, texSize.y)),
                sf::Vertex(v0, tint, sf::Vector2f(0.0f, 0.0f)),
                sf::Vertex(v2, tint, sf::Vector2f(texSize.x, texSize.y)),
                sf::Vertex(v3, tint, sf::Vector2f(0.0f, texSize.y))
            };

            window.draw(vertices, 6, sf::PrimitiveType::Triangles, sf::RenderStates(&texture));
        }
    };
}

#endif
