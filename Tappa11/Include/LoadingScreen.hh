#ifndef LOADING_SCREEN_HH
#define LOADING_SCREEN_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <iostream>
#include <string>

namespace fcg{

    //Disegna UNA schermata di caricamento e la presenta subito a video (window.display()).
    //La generazione del mondo (World: chunk + mesh) e la costruzione di Renderer (shader,
    //texture) sono sincrone e bloccano il thread principale: chiamando questa funzione
    //appena PRIMA di quelle chiamate, l'utente vede un feedback immediato invece di uno
    //schermo bloccato per la durata della generazione. Se l'immagine manca, mostra solo
    //il testo (stesso spirito dei fallback gia' usati altrove nel progetto)
    inline void DrawLoadingScreen(sf::RenderWindow& window, const std::string& resourcesDir){
        sf::Font font;
        bool fontLoaded = font.openFromFile(resourcesDir + "pixelFont.ttf");
        if(!fontLoaded){
            std::cerr << "Errore (LoadingScreen): impossibile caricare pixelFont.ttf." << std::endl;
        }

        sf::Texture logoTexture;
        bool logoLoaded = logoTexture.loadFromFile(resourcesDir + "loadingScreen.png");
        if(!logoLoaded){
            std::cerr << "Attenzione (LoadingScreen): loadingScreen.png non trovato, mostro solo il testo." << std::endl;
        }

        int windowWidth = (int) window.getSize().x;
        int windowHeight = (int) window.getSize().y;

        window.clear(sf::Color(0, 0, 0)); //RGB 18,18,26
        window.pushGLStates();

        if(logoLoaded){
            sf::Sprite logoSprite(logoTexture);
            sf::Vector2u textureSize = logoTexture.getSize();

            //Il logo occupa al massimo un quarto dell'altezza della finestra, proporzioni mantenute
            float maxLogoHeight = windowHeight * 0.25f;
            float scale = maxLogoHeight / (float) textureSize.y;
            logoSprite.setScale({scale, scale});

            float logoWidth = textureSize.x * scale;
            float logoHeight = textureSize.y * scale;
            logoSprite.setPosition({
                (windowWidth - logoWidth) * 0.5f,
                (windowHeight - logoHeight) * 0.5f
            });

            window.draw(logoSprite);
        }

        if(fontLoaded){
            sf::Text loadingText(font, "Generazione del terreno in corso...", 26);
            loadingText.setFillColor(sf::Color::White);

            sf::FloatRect bounds = loadingText.getLocalBounds();
            loadingText.setPosition({
                (windowWidth - bounds.size.x) * 0.5f - bounds.position.x,
                windowHeight * 0.75f
            });

            window.draw(loadingText);
        }

        window.popGLStates();
        window.display();
    }
}

#endif
