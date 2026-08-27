#ifndef OPTIONS_PANEL_HH
#define OPTIONS_PANEL_HH

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

namespace fcg{

    //Pannello "Opzioni" riutilizzabile: FOV e risoluzione (entrambi coi pulsanti -/+),
    //piu' un tasto "Indietro". E' un componente, non una schermata a se stante: chi lo
    //usa (MainMenu, PauseMenu) decide quando mostrarlo e cosa fare del tasto Indietro.
    //Il cambio di risoluzione NON si applica mai a video: resta solo in memoria (e va
    //salvato su file da chi possiede il pannello) per essere applicato al prossimo avvio
    class OptionsPanel{
    public:
        enum class Action{ None, Back, FovChanged, ResolutionChanged };

    private:
        struct ResolutionPreset{ int width; int height; };

        sf::Text titleText;
        sf::Text fovLabelText;
        sf::Text fovMinusText;
        sf::Text fovValueText;
        sf::Text fovPlusText;
        sf::Text resLabelText;
        sf::Text resPrevText;
        sf::Text resValueText;
        sf::Text resNextText;
        sf::Text resNoteText;
        sf::Text backButtonText;

        sf::RectangleShape fovMinusShape;
        sf::RectangleShape fovPlusShape;
        sf::RectangleShape resPrevShape;
        sf::RectangleShape resNextShape;
        sf::RectangleShape backButtonShape;

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
        int currentResIndex = 2;

        int windowWidth = 1920;
        int windowHeight = 1080;
        float topY = 0.0f; //Offset verticale: chi possiede il pannello decide dove inizia

        static constexpr float buttonWidth = 420.0f;
        static constexpr float buttonHeight = 80.0f;
        static constexpr float smallButtonSize = 56.0f;
        static constexpr float valueBoxWidth = 260.0f;
        static constexpr float rowSpacing = 16.0f;

        const sf::Color buttonIdleColor = sf::Color(55, 55, 70);
        const sf::Color buttonHoverColor = sf::Color(95, 95, 125);

    public:
        //'font' deve restare valido per tutta la vita di questo oggetto: e' il chiamante
        //(MainMenu, PauseMenu) a possederlo, cosi' il font si carica una volta sola
        OptionsPanel(sf::Font& font, float initialFov, int initialWidth, int initialHeight) :
            titleText(font, "OPZIONI", 52),
            fovLabelText(font, "FOV", 20),
            fovMinusText(font, "-", 26),
            fovValueText(font, "", 24),
            fovPlusText(font, "+", 26),
            resLabelText(font, "RISOLUZIONE SCHERMO", 20),
            resPrevText(font, "<", 26),
            resValueText(font, "", 24),
            resNextText(font, ">", 26),
            resNoteText(font, "Si applica al prossimo avvio", 16),
            backButtonText(font, "Indietro", 22)
        {
            titleText.setFillColor(sf::Color::White);
            fovLabelText.setFillColor(sf::Color(210, 210, 210));
            resLabelText.setFillColor(sf::Color(210, 210, 210));
            resNoteText.setFillColor(sf::Color(160, 160, 160));
            fovMinusText.setFillColor(sf::Color::White);
            fovValueText.setFillColor(sf::Color::White);
            fovPlusText.setFillColor(sf::Color::White);
            resPrevText.setFillColor(sf::Color::White);
            resValueText.setFillColor(sf::Color::White);
            resNextText.setFillColor(sf::Color::White);
            backButtonText.setFillColor(sf::Color::White);

            backButtonShape.setSize({buttonWidth, buttonHeight});
            fovMinusShape.setSize({smallButtonSize, smallButtonSize});
            fovPlusShape.setSize({smallButtonSize, smallButtonSize});
            resPrevShape.setSize({smallButtonSize, smallButtonSize});
            resNextShape.setSize({smallButtonSize, smallButtonSize});

            backButtonShape.setFillColor(buttonIdleColor);
            fovMinusShape.setFillColor(buttonIdleColor);
            fovPlusShape.setFillColor(buttonIdleColor);
            resPrevShape.setFillColor(buttonIdleColor);
            resNextShape.setFillColor(buttonIdleColor);

            fov = Clamp(initialFov);
            currentResIndex = FindResolutionIndex(initialWidth, initialHeight);

            Layout();
        }

        //y0 e' l'offset verticale da cui inizia il pannello: permette a MainMenu e
        //PauseMenu di posizionarlo in punti diversi dello schermo
        void SetWindowSize(int width, int height, float y0 = 0.0f){
            windowWidth = width;
            windowHeight = height;
            topY = y0;
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

        void UpdateHover(sf::Vector2i mousePos){
            sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);
            fovMinusShape.setFillColor(fovMinusShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            fovPlusShape.setFillColor(fovPlusShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            resPrevShape.setFillColor(resPrevShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            resNextShape.setFillColor(resNextShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
            backButtonShape.setFillColor(backButtonShape.getGlobalBounds().contains(mouse) ? buttonHoverColor : buttonIdleColor);
        }

        Action HandleClick(sf::Vector2i mousePos){
            sf::Vector2f mouse((float) mousePos.x, (float) mousePos.y);

            if(backButtonShape.getGlobalBounds().contains(mouse)) return Action::Back;
            if(fovMinusShape.getGlobalBounds().contains(mouse)){
                AdjustFov(-fovStep);
                return Action::FovChanged;
            }
            if(fovPlusShape.getGlobalBounds().contains(mouse)){
                AdjustFov(fovStep);
                return Action::FovChanged;
            }
            if(resPrevShape.getGlobalBounds().contains(mouse)){
                CycleResolution(-1);
                return Action::ResolutionChanged;
            }
            if(resNextShape.getGlobalBounds().contains(mouse)){
                CycleResolution(1);
                return Action::ResolutionChanged;
            }
            return Action::None;
        }

        void Draw(sf::RenderWindow& window){
            window.draw(titleText);

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
            window.draw(resNoteText);

            window.draw(backButtonShape);
            window.draw(backButtonText);
        }

    private:
        static float Clamp(float value){
            if(value < minFov) return minFov;
            if(value > maxFov) return maxFov;
            return value;
        }

        int FindResolutionIndex(int width, int height) const{
            for(int i = 0; i < resolutionPresetCount; i++){
                if(resolutionPresets[i].width == width && resolutionPresets[i].height == height) return i;
            }
            return 2; //1920x1080 se la risoluzione salvata non e' uno dei preset noti
        }

        void AdjustFov(float delta){
            fov = Clamp(fov + delta);
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
            CenterHorizontally(titleText, topY);

            float rowWidth = smallButtonSize * 2.0f + valueBoxWidth + rowSpacing * 2.0f;
            float rowStartX = (windowWidth - rowWidth) * 0.5f;

            float fovRowY = topY + 130.0f;
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
            CenterHorizontally(resNoteText, resRowY + smallButtonSize + 12.0f);

            float backY = resRowY + smallButtonSize + 70.0f;
            backButtonShape.setPosition({(windowWidth - buttonWidth) * 0.5f, backY});
            CenterTextOnButton(backButtonText, backButtonShape);

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
