#ifndef PAUSE_MENU_HH
#define PAUSE_MENU_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <string>
#include "OptionsPanel.hh"

//Modificati i pulsanti delle impostazioni

namespace fcg{
    //Overlay di pausa (ESC durante il gioco): scurisce lo schermo con un riquadro
    //semi-trasparente e mostra Ritorna al gioco / Opzioni / (Menu Principale, Esci dal
    //gioco sulla stessa riga, piu' piccoli). Il chiamante NON deve leggere alcun input
    //di gioco mentre questo overlay e' attivo: e' lui a garantirlo semplicemente non
    //chiamando le funzioni di update del gioco mentre lo stato e' Paused
    class PauseMenu{
    public:
        enum class MenuAction{ None, Resume, BackToMainMenu, QuitGame, FovChanged, ResolutionChanged };

    private:
        enum class Screen{ Pause, Options };

        sf::Font font; //Dichiarato PRIMA dei sf::Text, stesso motivo di MainMenu

        sf::RectangleShape dimBackground; //Scurisce il mondo "congelato" dietro l'overlay

        sf::Text titleText;
        sf::Text resumeButtonText;
        sf::Text optionsButtonText;
        sf::Text mainMenuButtonText;
        sf::Text quitButtonText;

        sf::RectangleShape resumeButtonShape;
        sf::RectangleShape optionsButtonShape;
        sf::RectangleShape mainMenuButtonShape;
        sf::RectangleShape quitButtonShape;

        OptionsPanel optionsPanel;

        Screen currentScreen = Screen::Pause;

        int windowWidth = 1920;
        int windowHeight = 1080;

        static constexpr float bigButtonWidth = 576.0f;
        static constexpr float bigButtonHeight = 64.0f;
        static constexpr float smallButtonWidth = 280.0f;
        static constexpr float smallButtonHeight = 64.0f;
        static constexpr float rowGap = 26.0f;
        static constexpr float smallRowGap = 16.0f;

        const sf::Color buttonIdleColor = sf::Color(55, 55, 70);
        const sf::Color buttonHoverColor = sf::Color(95, 95, 125);
        const sf::Color quitButtonIdleColor = sf::Color(90, 45, 45);
        const sf::Color quitButtonHoverColor = sf::Color(140, 60, 60);

    public:
        PauseMenu(const std::string& resourcesDir, float initialFov, int initialWidth, int initialHeight) :
            titleText(LoadFont(resourcesDir), "PAUSA", 56),
            resumeButtonText(font, "Ritorna al gioco", 22),
            optionsButtonText(font, "Opzioni", 22),
            mainMenuButtonText(font, "Menu Principale", 18),
            quitButtonText(font, "Esci dal gioco", 18),
            optionsPanel(font, initialFov, initialWidth, initialHeight)
        {
            dimBackground.setFillColor(sf::Color(0, 0, 0, 170));

            titleText.setFillColor(sf::Color::White);
            resumeButtonText.setFillColor(sf::Color::White);
            optionsButtonText.setFillColor(sf::Color::White);
            mainMenuButtonText.setFillColor(sf::Color::White);
            quitButtonText.setFillColor(sf::Color::White);

            resumeButtonShape.setSize({bigButtonWidth, bigButtonHeight});
            optionsButtonShape.setSize({bigButtonWidth, bigButtonHeight});
            mainMenuButtonShape.setSize({smallButtonWidth, smallButtonHeight});
            quitButtonShape.setSize({smallButtonWidth, smallButtonHeight});

            resumeButtonShape.setFillColor(buttonIdleColor);
            optionsButtonShape.setFillColor(buttonIdleColor);
            mainMenuButtonShape.setFillColor(buttonIdleColor);
            quitButtonShape.setFillColor(quitButtonIdleColor);

            Layout();
        }

        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
            Layout();
        }

        //Va richiamata ogni volta che si apre l'overlay (ESC): cosi' riparte sempre dalla
        //schermata "Pausa" e non resta bloccata su Opzioni da una sessione precedente
        void Reset(){
            currentScreen = Screen::Pause;
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
            if(currentScreen == Screen::Pause){
                sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);
                resumeButtonShape.setFillColor(resumeButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                optionsButtonShape.setFillColor(optionsButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                mainMenuButtonShape.setFillColor(mainMenuButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                quitButtonShape.setFillColor(quitButtonShape.getGlobalBounds().contains(mouse) ? quitButtonHoverColor : quitButtonIdleColor);
            }
            else{
                optionsPanel.UpdateHover(mousePos);
            }
        }

        MenuAction HandleClick(sf::Vector2i mousePos){
            if(currentScreen == Screen::Pause){
                sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);
                if(resumeButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::Resume;
                if(optionsButtonShape.getGlobalBounds().contains(mouse)){
                    currentScreen = Screen::Options;
                    return MenuAction::None;
                }
                if(mainMenuButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::BackToMainMenu;
                if(quitButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::QuitGame;
                return MenuAction::None;
            }

            OptionsPanel::Action action = optionsPanel.HandleClick(mousePos);
            switch(action){
                case OptionsPanel::Action::Back:
                    currentScreen = Screen::Pause;
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
            window.draw(dimBackground);

            if(currentScreen == Screen::Pause){
                window.draw(titleText);

                window.draw(resumeButtonShape);
                window.draw(resumeButtonText);

                window.draw(optionsButtonShape);
                window.draw(optionsButtonText);

                window.draw(mainMenuButtonShape);
                window.draw(mainMenuButtonText);

                window.draw(quitButtonShape);
                window.draw(quitButtonText);
            }
            else{
                optionsPanel.Draw(window);
            }
        }

    private:
        sf::Font& LoadFont(const std::string& resourcesDir){
            if(!font.openFromFile(resourcesDir + "pixelFont.ttf")){
                std::cerr << "Errore (PauseMenu): impossibile caricare pixelFont.ttf, impossibile continuare." << std::endl;
                exit(1);
            }
            return font;
        }

        void Layout(){
            dimBackground.setSize({(float) windowWidth, (float) windowHeight});
            dimBackground.setPosition({0.0f, 0.0f});

            CenterHorizontally(titleText, windowHeight * 0.22f);

            float centerX = (windowWidth - bigButtonWidth) * 0.5f;
            float resumeY = windowHeight * 0.40f;
            float optionsY = resumeY + bigButtonHeight + rowGap;
            float smallRowY = optionsY + bigButtonHeight + rowGap;

            resumeButtonShape.setPosition({centerX, resumeY});
            optionsButtonShape.setPosition({centerX, optionsY});

            CenterTextOnButton(resumeButtonText, resumeButtonShape);
            CenterTextOnButton(optionsButtonText, optionsButtonShape);

            float smallRowWidth = smallButtonWidth * 2.0f + smallRowGap;
            float smallRowStartX = (windowWidth - smallRowWidth) * 0.5f;

            mainMenuButtonShape.setPosition({smallRowStartX, smallRowY});
            quitButtonShape.setPosition({smallRowStartX + smallButtonWidth + smallRowGap, smallRowY});

            CenterTextOnButton(mainMenuButtonText, mainMenuButtonShape);
            CenterTextOnButton(quitButtonText, quitButtonShape);

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
