#ifndef MAIN_MENU_HH
#define MAIN_MENU_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <string>
#include "OptionsPanel.hh"

namespace fcg{

    //Menu principale: schermata Main (titolo, Genera Mondo, Opzioni, Esci) e schermata
    //Options (delegata al componente condiviso OptionsPanel, lo stesso usato dalla pausa).
    //Disegnato in puro SFML 2D, nessuna dipendenza da OpenGL/3D: chi chiama Draw() deve
    //avvolgerla in window.pushGLStates()/popGLStates() se nel frame lo stato OpenGL
    //(depth test ecc.) e' gia' stato toccato, esattamente come si fa per Hotbar
    class MainMenu{
    public:
        enum class MenuAction{ None, GenerateWorld, Exit, FovChanged, ResolutionChanged };

    private:
        enum class Screen{ Main, Options };

        sf::Font font; //Dichiarato PRIMA dei sf::Text: i membri si inizializzano nell'ordine
                       //di dichiarazione (non della initializer list), quindi quando i
                       //sf::Text (e OptionsPanel, che referenzia 'font') vengono costruiti
                       //'font' e' gia' pronto

        sf::Text titleText;
        sf::Text generateButtonText;
        sf::Text optionsButtonText;
        sf::Text exitButtonText;
        sf::Text commandsText;

        sf::RectangleShape generateButtonShape;
        sf::RectangleShape optionsButtonShape;
        sf::RectangleShape exitButtonShape;

        OptionsPanel optionsPanel;

        Screen currentScreen = Screen::Main;

        int windowWidth = 1920;
        int windowHeight = 1080;

        static constexpr float buttonWidth = 420.0f;
        static constexpr float buttonHeight = 80.0f;
        static constexpr float buttonSpacing = 26.0f;

        const sf::Color buttonIdleColor = sf::Color(55, 55, 70);
        const sf::Color buttonHoverColor = sf::Color(95, 95, 125);

    public:
        //resourcesDir e' il path relativo alle risorse condivise (es. "../Resources/").
        //initialFov/initialWidth/initialHeight arrivano dal file di preferenze (Settings.hh)
        MainMenu(const std::string& resourcesDir, float initialFov, int initialWidth, int initialHeight) :
            titleText(LoadFont(resourcesDir), "NicoCraft", 64),
            generateButtonText(font, "Genera Mondo", 22),
            optionsButtonText(font, "Opzioni", 22),
            exitButtonText(font, "Esci", 22),
            commandsText(font, BuildCommandsString(), 18),
            optionsPanel(font, initialFov, initialWidth, initialHeight)
        {
            titleText.setFillColor(sf::Color::White);
            generateButtonText.setFillColor(sf::Color::White);
            optionsButtonText.setFillColor(sf::Color::White);
            exitButtonText.setFillColor(sf::Color::White);

            commandsText.setFillColor(sf::Color(210, 210, 210));
            commandsText.setLineSpacing(1.3f);

            generateButtonShape.setSize({buttonWidth, buttonHeight});
            optionsButtonShape.setSize({buttonWidth, buttonHeight});
            exitButtonShape.setSize({buttonWidth, buttonHeight});
            generateButtonShape.setFillColor(buttonIdleColor);
            optionsButtonShape.setFillColor(buttonIdleColor);
            exitButtonShape.setFillColor(buttonIdleColor);

            Layout();
        }

        //Va richiamata all'avvio e ad ogni sf::Event::Resized
        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
            Layout();
        }

        float GetFov() const{
            return optionsPanel.GetFov();
        }

        int GetResolutionWidth() const{
            return optionsPanel.GetResolutionWidth();
        }

        int GetResolutionHeight() const{
            return optionsPanel.GetResolutionHeight();
        }

        void UpdateHover(sf::Vector2i mousePos){
            if(currentScreen == Screen::Main){
                sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);
                generateButtonShape.setFillColor(generateButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                optionsButtonShape.setFillColor(optionsButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                exitButtonShape.setFillColor(exitButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            }
            else{
                optionsPanel.UpdateHover(mousePos);
            }
        }

        //Da chiamare quando arriva un click sinistro. Gestisce da sola la navigazione
        //Main <-> Options; ritorna solo le azioni che il chiamante deve applicare
        //all'esterno (creare il mondo, uscire, salvare le preferenze)
        MenuAction HandleClick(sf::Vector2i mousePos){
            if(currentScreen == Screen::Main){
                sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);
                if(generateButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::GenerateWorld;
                if(optionsButtonShape.getGlobalBounds().contains(mouse)){
                    currentScreen = Screen::Options;
                    return MenuAction::None;
                }
                if(exitButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::Exit;
                return MenuAction::None;
            }

            OptionsPanel::Action action = optionsPanel.HandleClick(mousePos);
            switch(action){
                case OptionsPanel::Action::Back:
                    currentScreen = Screen::Main;
                    return MenuAction::None;
                case OptionsPanel::Action::FovChanged:
                    return MenuAction::FovChanged;
                case OptionsPanel::Action::ResolutionChanged:
                    return MenuAction::ResolutionChanged;
                default:
                    return MenuAction::None;
            }
        }

        void Draw(sf::RenderWindow& window){
            if(currentScreen == Screen::Main){
                window.draw(titleText);

                window.draw(generateButtonShape);
                window.draw(generateButtonText);

                window.draw(optionsButtonShape);
                window.draw(optionsButtonText);

                window.draw(exitButtonShape);
                window.draw(exitButtonText);

                window.draw(commandsText);
            }
            else{
                optionsPanel.Draw(window);
            }
        }

    private:
        sf::Font& LoadFont(const std::string& resourcesDir){
            if(!font.openFromFile(resourcesDir + "pixelFont.ttf")){
                std::cerr << "Errore (MainMenu): impossibile caricare pixelFont.ttf, impossibile continuare." << std::endl;
                exit(1);
            }
            return font;
        }

        static std::string BuildCommandsString(){
            return
                "COMANDI\n"
                "W A S D - muovi\n"
                "Mouse - guarda\n"
                "LShift - sprint\n"
                "Spazio - salta / vola su (noclip)\n"
                "LCtrl - vola giu' (noclip)\n"
                "F - attiva/disattiva noclip\n"
                "Click sinistro - rompi blocco\n"
                "Click destro - piazza blocco\n"
                "1-7 / rotellina - seleziona blocco\n"
                "Esc - pausa";
        }

        void Layout(){
            CenterHorizontally(titleText, windowHeight * 0.10f);

            float centerX = (windowWidth - buttonWidth) * 0.5f;
            float generateY = windowHeight * 0.34f;
            float optionsY = generateY + buttonHeight + buttonSpacing;
            float exitY = optionsY + buttonHeight + buttonSpacing;

            generateButtonShape.setPosition({centerX, generateY});
            optionsButtonShape.setPosition({centerX, optionsY});
            exitButtonShape.setPosition({centerX, exitY});

            CenterTextOnButton(generateButtonText, generateButtonShape);
            CenterTextOnButton(optionsButtonText, optionsButtonShape);
            CenterTextOnButton(exitButtonText, exitButtonShape);

            commandsText.setPosition({40.0f, windowHeight - 300.0f});

            optionsPanel.SetWindowSize(windowWidth, windowHeight, windowHeight * 0.10f);
        }

        void CenterHorizontally(sf::Text& text, float y){
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition({(windowWidth - bounds.size.x) * 0.5f - bounds.position.x, y});
        }

        static void CenterTextOnButton(sf::Text& text, const sf::RectangleShape& button){
            sf::FloatRect bounds = text.getLocalBounds();
            sf::Vector2f buttonPos = button.getPosition();
            sf::Vector2f buttonSize = button.getSize();
            text.setPosition({
                buttonPos.x + (buttonSize.x - bounds.size.x) * 0.5f - bounds.position.x,
                buttonPos.y + (buttonSize.y - bounds.size.y) * 0.5f - bounds.position.y
            });
        }
    };
}

#endif
