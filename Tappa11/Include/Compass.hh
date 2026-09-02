#ifndef COMPASS_HH
#define COMPASS_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <iostream>
#include <string>

namespace fcg{

    //Scritta in alto a destra con il punto cardinale approssimato verso cui guarda il
    //player, dedotto dallo yaw della Camera. Solo le 4 direzioni principali (N/S/E/O),
    //nessuna diagonale: sufficiente per orientarsi nel mondo
    class Compass{
    private:
        sf::Font font;
        sf::Text directionText;

        int windowWidth = 1920;
        static constexpr float marginTop = 20.0f;
        static constexpr float marginLeft = 20.0f;

    public:
        Compass(const std::string& resourcesDir) :
            directionText(LoadFont(resourcesDir), "South", 26)
        {
            directionText.setFillColor(sf::Color::White);
            Layout();
        }

        void SetWindowSize(int width, int height){
            windowWidth = width;
            Layout();
        }

        //yawDeg viene da Camera::GetYaw(). Va richiamata una volta per frame prima di Draw()
        void Update(float yawDeg){
            directionText.setString(DirectionLabel(yawDeg));
            Layout(); //La stringa cambia larghezza (N vs NE, qui sempre 1 lettera, ma per sicurezza)
        }

        void Draw(sf::RenderWindow& window){
            window.draw(directionText);
        }

    private:
        sf::Font& LoadFont(const std::string& resourcesDir){
            if(!font.openFromFile(resourcesDir + "pixelFont.ttf")){
                std::cerr << "Errore (Compass): impossibile caricare pixelFont.ttf, impossibile continuare." << std::endl;
                exit(1);
            }
            return font;
        }

        //A yaw=0 il forward della Camera e' -Z (vedi Camera::GetForward): -Z=Nord, +X=Est,
        //+Z=Sud, -X=Ovest. 4 fasce da 90 gradi centrate sui multipli di 90
        static std::string DirectionLabel(float yawDeg){
            float normalized = std::fmod(yawDeg, 360.0f);
            if(normalized < 0.0f) normalized += 360.0f;

            if(normalized >= 315.0f || normalized < 45.0f) return "South";
            if(normalized < 135.0f) return "Ovest";
            if(normalized < 225.0f) return "North";
            return "East";
        }

        void Layout(){
            directionText.setPosition({
                marginLeft,
                marginTop
            });
        }
    };
}

#endif
