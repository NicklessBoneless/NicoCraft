#ifndef MAIN_MENU_HH
#define MAIN_MENU_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <string>

namespace fcg{

    //Menu principale: schermata Main (titolo, Genera Mondo, Opzioni, Esci) e
    //schermata Options (FOV e risoluzione, entrambi regolabili coi pulsanti -/+).
    //Disegnato in puro SFML 2D, nessuna dipendenza da OpenGL/3D: chi chiama Draw()
    //deve avvolgerla in window.pushGLStates()/popGLStates() se nel frame lo stato
    //OpenGL (depth test ecc.) e' gia' stato toccato, esattamente come si fa per Hotbar
    class MainMenu{
    public:
        //Azioni che la MainMenu non puo' applicare da sola (serve la finestra vera e
        //propria, o la Camera): il chiamante le legge dal valore di ritorno di HandleClick()
        enum class MenuAction{ None, GenerateWorld, Exit, FovChanged, ResolutionChanged };

    private:
        enum class Screen{ Main, Options };

        struct ResolutionPreset{ int width; int height; };

        sf::Font font; //Dichiarato PRIMA dei sf::Text: i membri si inizializzano nell'ordine
                       //di dichiarazione (non della initializer list), quindi quando i
                       //sf::Text vengono costruiti 'font' e' gia' pronto

        //// Schermata Main ////
        sf::Text titleText;
        sf::Text generateButtonText;
        sf::Text optionsButtonText;
        sf::Text exitButtonText;
        sf::Text commandsText;

        //// Schermata Options ////
        sf::Text optionsTitleText;
        sf::Text fovLabelText;
        sf::Text fovMinusText;
        sf::Text fovValueText;
        sf::Text fovPlusText;
        sf::Text resLabelText;
        sf::Text resPrevText;
        sf::Text resValueText;
        sf::Text resNextText;
        sf::Text backButtonText;

        sf::RectangleShape generateButtonShape;
        sf::RectangleShape optionsButtonShape;
        sf::RectangleShape exitButtonShape;

        sf::RectangleShape fovMinusShape;
        sf::RectangleShape fovPlusShape;
        sf::RectangleShape resPrevShape;
        sf::RectangleShape resNextShape;
        sf::RectangleShape backButtonShape;

        Screen currentScreen = Screen::Main;

        float fov = 90.0f;
        static constexpr float minFov = 60.0f;
        static constexpr float maxFov = 110.0f;
        static constexpr float fovStep = 5.0f;

        static constexpr ResolutionPreset resolutionPresets[] = {
            {1280, 720},
            {1600, 900},
            {1920, 1080},
            {2560, 1440}
        };
        static constexpr int resolutionPresetCount = 4;
        int currentResIndex = 2; //1920x1080, coerente col default di Setup

        int windowWidth = 1920;
        int windowHeight = 1080;

        static constexpr float buttonWidth = 420.0f;
        static constexpr float buttonHeight = 80.0f;
        static constexpr float buttonSpacing = 26.0f;

        static constexpr float smallButtonSize = 56.0f;
        static constexpr float valueBoxWidth = 260.0f;
        static constexpr float rowSpacing = 16.0f;

        const sf::Color buttonIdleColor = sf::Color(55, 55, 70);
        const sf::Color buttonHoverColor = sf::Color(95, 95, 125);

    public:
        //resourcesDir e' il path relativo alle risorse condivise (es. "../Resources/"),
        //stesso path usato per le texture dei blocchi
        MainMenu(const std::string& resourcesDir) :
            titleText(LoadFont(resourcesDir), "NicoCraft", 64),
            generateButtonText(font, "Genera Mondo", 22),
            optionsButtonText(font, "Opzioni", 22),
            exitButtonText(font, "Esci", 22),
            commandsText(font, BuildCommandsString(), 18),
            optionsTitleText(font, "OPZIONI", 52),
            fovLabelText(font, "FOV", 20),
            fovMinusText(font, "-", 26),
            fovValueText(font, "", 24),
            fovPlusText(font, "+", 26),
            resLabelText(font, "RISOLUZIONE SCHERMO", 20),
            resPrevText(font, "<", 26),
            resValueText(font, "", 24),
            resNextText(font, ">", 26),
            backButtonText(font, "Indietro", 22)
        {
            titleText.setFillColor(sf::Color::White);

            generateButtonText.setFillColor(sf::Color::White);
            optionsButtonText.setFillColor(sf::Color::White);
            exitButtonText.setFillColor(sf::Color::White);

            commandsText.setFillColor(sf::Color(210, 210, 210));
            commandsText.setLineSpacing(1.3f);

            optionsTitleText.setFillColor(sf::Color::White);
            fovLabelText.setFillColor(sf::Color(210, 210, 210));
            resLabelText.setFillColor(sf::Color(210, 210, 210));
            fovMinusText.setFillColor(sf::Color::White);
            fovValueText.setFillColor(sf::Color::White);
            fovPlusText.setFillColor(sf::Color::White);
            resPrevText.setFillColor(sf::Color::White);
            resValueText.setFillColor(sf::Color::White);
            resNextText.setFillColor(sf::Color::White);
            backButtonText.setFillColor(sf::Color::White);

            generateButtonShape.setSize({buttonWidth, buttonHeight});
            optionsButtonShape.setSize({buttonWidth, buttonHeight});
            exitButtonShape.setSize({buttonWidth, buttonHeight});
            backButtonShape.setSize({buttonWidth, buttonHeight});
            fovMinusShape.setSize({smallButtonSize, smallButtonSize});
            fovPlusShape.setSize({smallButtonSize, smallButtonSize});
            resPrevShape.setSize({smallButtonSize, smallButtonSize});
            resNextShape.setSize({smallButtonSize, smallButtonSize});

            generateButtonShape.setFillColor(buttonIdleColor);
            optionsButtonShape.setFillColor(buttonIdleColor);
            exitButtonShape.setFillColor(buttonIdleColor);
            backButtonShape.setFillColor(buttonIdleColor);
            fovMinusShape.setFillColor(buttonIdleColor);
            fovPlusShape.setFillColor(buttonIdleColor);
            resPrevShape.setFillColor(buttonIdleColor);
            resNextShape.setFillColor(buttonIdleColor);

            Layout();
        }

        //Va richiamata all'avvio e ad ogni sf::Event::Resized
        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
            Layout();
        }

        float GetFov() const{
            return fov;
        }

        int GetResolutionWidth() const{
            return resolutionPresets[currentResIndex].width;
        }

        int GetResolutionHeight() const{
            return resolutionPresets[currentResIndex].height;
        }

        //Aggiorna il colore dei tasti della schermata attiva in base al mouse (feedback hover)
        void UpdateHover(sf::Vector2i mousePos){
            sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);

            if(currentScreen == Screen::Main){
                generateButtonShape.setFillColor(generateButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                optionsButtonShape.setFillColor(optionsButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                exitButtonShape.setFillColor(exitButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            }
            else{
                fovMinusShape.setFillColor(fovMinusShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                fovPlusShape.setFillColor(fovPlusShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                resPrevShape.setFillColor(resPrevShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                resNextShape.setFillColor(resNextShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
                backButtonShape.setFillColor(backButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            }
        }

        //Da chiamare quando arriva un click sinistro. Gestisce da sola la navigazione
        //tra le due schermate (Main <-> Options) e i valori di FOV/risoluzione; ritorna
        //solo le azioni che il chiamante deve applicare all'esterno (finestra, Camera)
        MenuAction HandleClick(sf::Vector2i mousePos){
            sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);

            if(currentScreen == Screen::Main){
                if(generateButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::GenerateWorld;
                if(optionsButtonShape.getGlobalBounds().contains(mouse)){
                    currentScreen = Screen::Options;
                    return MenuAction::None;
                }
                if(exitButtonShape.getGlobalBounds().contains(mouse)) return MenuAction::Exit;
                return MenuAction::None;
            }

            //Screen::Options
            if(backButtonShape.getGlobalBounds().contains(mouse)){
                currentScreen = Screen::Main;
                return MenuAction::None;
            }
            if(fovMinusShape.getGlobalBounds().contains(mouse)){
                AdjustFov(-fovStep);
                return MenuAction::FovChanged;
            }
            if(fovPlusShape.getGlobalBounds().contains(mouse)){
                AdjustFov(fovStep);
                return MenuAction::FovChanged;
            }
            if(resPrevShape.getGlobalBounds().contains(mouse)){
                CycleResolution(-1);
                return MenuAction::ResolutionChanged;
            }
            if(resNextShape.getGlobalBounds().contains(mouse)){
                CycleResolution(1);
                return MenuAction::ResolutionChanged;
            }
            return MenuAction::None;
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
                window.draw(optionsTitleText);

                window.draw(fovLabelText);
                window.draw(fovMinusShape);
                window.draw(fovMinusText);
                window.draw(fovValueText);
                window.draw(fovPlusShape);
                window.draw(fovPlusText);

                window.draw(resLabelText);
                window.draw(resPrevShape);
                window.draw(resPrevText);
                window.draw(resValueText);
                window.draw(resNextShape);
                window.draw(resNextText);

                window.draw(backButtonShape);
                window.draw(backButtonText);
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
                "Esc - esci";
        }

        void AdjustFov(float delta){
            fov += delta;
            if(fov < minFov) fov = minFov;
            if(fov > maxFov) fov = maxFov;
            UpdateFovValueText();
        }

        void CycleResolution(int direction){
            currentResIndex = ((currentResIndex + direction) % resolutionPresetCount + resolutionPresetCount) % resolutionPresetCount;
            UpdateResValueText();
        }

        void UpdateFovValueText(){
            fovValueText.setString(std::to_string((int) fov));
            CenterTextInBounds(fovValueText, fovMinusShape.getPosition().x + smallButtonSize + rowSpacing, fovMinusShape.getPosition().y, valueBoxWidth, smallButtonSize);
        }

        void UpdateResValueText(){
            resValueText.setString(std::to_string(GetResolutionWidth()) + " x " + std::to_string(GetResolutionHeight()));
            CenterTextInBounds(resValueText, resPrevShape.getPosition().x + smallButtonSize + rowSpacing, resPrevShape.getPosition().y, valueBoxWidth, smallButtonSize);
        }

        void Layout(){
            //// Schermata Main ////
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

            //// Schermata Options ////
            CenterHorizontally(optionsTitleText, windowHeight * 0.10f);

            float rowWidth = smallButtonSize * 2.0f + valueBoxWidth + rowSpacing * 2.0f;
            float rowStartX = (windowWidth - rowWidth) * 0.5f;

            float fovRowY = windowHeight * 0.36f;
            fovMinusShape.setPosition({rowStartX, fovRowY});
            fovPlusShape.setPosition({rowStartX + smallButtonSize + rowSpacing + valueBoxWidth + rowSpacing, fovRowY});
            CenterHorizontally(fovLabelText, fovRowY - 40.0f);
            CenterTextOnButton(fovMinusText, fovMinusShape);
            CenterTextOnButton(fovPlusText, fovPlusShape);

            float resRowY = fovRowY + smallButtonSize + 80.0f;
            resPrevShape.setPosition({rowStartX, resRowY});
            resNextShape.setPosition({rowStartX + smallButtonSize + rowSpacing + valueBoxWidth + rowSpacing, resRowY});
            CenterHorizontally(resLabelText, resRowY - 40.0f);
            CenterTextOnButton(resPrevText, resPrevShape);
            CenterTextOnButton(resNextText, resNextShape);

            float backY = resRowY + smallButtonSize + 60.0f;
            backButtonShape.setPosition({(windowWidth - buttonWidth) * 0.5f, backY});
            CenterTextOnButton(backButtonText, backButtonShape);

            //Ricentra i valori correnti (dipendono dalla larghezza finestra)
            UpdateFovValueText();
            UpdateResValueText();
        }

        void CenterHorizontally(sf::Text& text, float y){
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition({(windowWidth - bounds.size.x) * 0.5f - bounds.position.x, y});
        }

        static void CenterTextOnButton(sf::Text& text, const sf::RectangleShape& button){
            CenterTextInBounds(text, button.getPosition().x, button.getPosition().y, button.getSize().x, button.getSize().y);
        }

        static void CenterTextInBounds(sf::Text& text, float x, float y, float width, float height){
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition({
                x + (width - bounds.size.x) * 0.5f - bounds.position.x,
                y + (height - bounds.size.y) * 0.5f - bounds.position.y
            });
        }
    };
}

#endif
