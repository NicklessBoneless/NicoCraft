#ifndef HOTBAR_HH
#define HOTBAR_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <vector>
#include <string>
#include "Blocks.hh"

namespace fcg{
    class Hotbar{
    private:
        std::vector<Blocks::BlockType> slots;
        int selectedIndex = 0;

        static constexpr float slotSize = 64.0f;
        static constexpr float slotPadding = 8.0f;
        static constexpr float slotMargin = 20.0f;  //Distanza dal bordo inferiore dello schermo
        static constexpr float iconMargin = 6.0f;   //Margine tra il bordo dello slot e l'icona

    public:
        Hotbar(){
            
        }

        Blocks::BlockType GetSelectedBlockType() const{
            return slots[selectedIndex].blockType;
        }

        //Seleziona lo slot 'index' (0-based). Richiesta ignorata se fuori range
        void SelectSlot(int index){
            if(index >= 0 && index < (int) slots.size()){
                selectedIndex = index;
            }
        }

        void Draw(sf::RenderWindow& window){
            sf::Vector2u windowSize = window.getSize();
            float slotCount = (float) slots.size();
            float totalWidth = slotCount * slotSize + (slotCount - 1) * slotPadding;
            float startX = ((float) windowSize.x - totalWidth) * 0.5f;
            float startY = (float) windowSize.y - slotSize - slotMargin;

            for(int i = 0; i < (int) slots.size(); i++){
                float x = startX + i * (slotSize + slotPadding);
                bool isSelected = (i == selectedIndex);

                sf::RectangleShape background({slotSize, slotSize});
                background.setPosition({x, startY});
                background.setFillColor(sf::Color(40, 40, 40, 180));
                background.setOutlineThickness(isSelected ? 3.0f : 1.0f);
                background.setOutlineColor(isSelected ? sf::Color::White : sf::Color(120, 120, 120));
                window.draw(background);

                float iconSize = slotSize - iconMargin * 2.0f;
                DrawIsoBlock(window, slots[i], x + iconMargin, startY + iconMargin, iconSize);
            }
        }

    private:
        //Disegna un piccolo cubo isometrico dentro il quadrato (x,y,size): una losanga superiore
        //(faccia Top) e due parallelogrammi sotto (facce laterali), a formare un esagono.
        //Rapporto isometrico 2:1 (larghezza doppia dell'altezza della losanga), come nei classici
        //tile-renderer isometrici
        void DrawIsoBlock(sf::RenderWindow& window, HotbarSlot& slot, float x, float y, float size){
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

            //Faccia superiore: piena luminosita', texture "top" del blocco
            DrawIsoFace(window, slot.topTexture, sf::Color::White, top, right, center, left);

            //Faccia laterale sinistra: texture "lato", tinta piu' scura (luce simulata)
            DrawIsoFace(window, slot.sideTexture, sf::Color(140, 140, 140), left, center, centerBottom, leftBottom);

            //Faccia laterale destra: texture "lato", tinta intermedia
            DrawIsoFace(window, slot.sideTexture, sf::Color(190, 190, 190), center, right, rightBottom, centerBottom);
        }

        //Disegna un parallelogramma texturizzato (2 triangoli, dato che SFML 3 non ha piu' la
        //primitiva Quads) con i 4 angoli v0..v3 dati in ordine, mappando i 4 angoli della texture
        //uno-a-uno sui 4 angoli dati: e' proprio questa corrispondenza "quadrato -> parallelogramma"
        //a produrre lo scorcio isometrico, sia per la losanga in alto sia per i lati
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