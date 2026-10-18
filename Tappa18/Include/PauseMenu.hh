#ifndef PAUSE_MENU_HH
#define PAUSE_MENU_HH

#include <imgui.h>
#include <iostream>
#include <string>
#include "OptionsPanel.hh"

namespace fcg{
    //Overlay di pausa, riscritto in ImGui immediate-mode (era puro SFML 2D).
    //Draw() va chiamata una volta per frame, tra ImGui_ImplOpenGL3_NewFrame()/
    //ImGui::SFML::Update() e ImGui::Render(): legge l'input e disegna nello stesso passo
    class PauseMenu{
    public:
        enum class MenuAction{ None, Resume, BackToMainMenu, QuitGame, FovChanged, ResolutionChanged };

    private:
        enum class Screen{ Pause, Options };

        static constexpr float baseFontSize = 24.0f;
        ImFont* font = nullptr; //Dichiarato PRIMA di optionsPanel, stesso trucco di MainMenu

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

    public:
        PauseMenu(const std::string& resourcesDir, float initialFov, int initialWidth, int initialHeight) : optionsPanel(LoadFont(resourcesDir), initialFov, initialWidth, initialHeight){
        }

        //Va richiamata all'avvio e ad ogni sf::Event::Resized
        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
            optionsPanel.SetWindowSize(width, height, height * 0.10f);
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

        //Disegna l'overlay e ritorna l'azione da applicare all'esterno (riprendere,
        //tornare al menu, uscire, salvare le preferenze); la navigazione Pause <-> Options resta interna
        MenuAction Draw(){
            MenuAction result = MenuAction::None;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.667f)); //Scurisce il mondo "congelato" dietro
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2((float) windowWidth, (float) windowHeight));

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

            ImGui::Begin("PauseMenu", nullptr, flags);
            ImGui::PushFont(font);

            if(currentScreen == Screen::Pause){
                result = DrawPauseScreen();
            }
            else{
                OptionsPanel::Action action = optionsPanel.Draw();
                switch(action){
                    case OptionsPanel::Action::Back:
                        currentScreen = Screen::Pause;
                        break;
                    case OptionsPanel::Action::FovChanged:
                        result = MenuAction::FovChanged;
                        break;
                    case OptionsPanel::Action::ResolutionChanged:
                        result = MenuAction::ResolutionChanged;
                        break;
                    default:
                        break;
                }
            }

            ImGui::PopFont();
            ImGui::End();
            ImGui::PopStyleColor();
            return result;
        }

    private:
        ImFont* LoadFont(const std::string& resourcesDir){
            ImGuiIO& io = ImGui::GetIO();
            std::string fontPath = resourcesDir + "pixelFont.ttf";
            font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), baseFontSize);
            if(!font){
                std::cerr << "Errore (PauseMenu): impossibile caricare pixelFont.ttf, impossibile continuare." << std::endl;
                exit(1);
            }
            return font;
        }

        MenuAction DrawPauseScreen(){
            MenuAction result = MenuAction::None;

            DrawTitle();

            float centerX = (windowWidth - bigButtonWidth) * 0.5f;
            float resumeY = windowHeight * 0.40f;
            float optionsY = resumeY + bigButtonHeight + rowGap;
            float smallRowY = optionsY + bigButtonHeight + rowGap;

            ImGui::SetWindowFontScale(22.0f / baseFontSize);

            ImGui::SetCursorPos(ImVec2(centerX, resumeY));
            if(ImGui::Button("Ritorna al gioco", ImVec2(bigButtonWidth, bigButtonHeight))){
                result = MenuAction::Resume;
            }

            ImGui::SetCursorPos(ImVec2(centerX, optionsY));
            if(ImGui::Button("Opzioni", ImVec2(bigButtonWidth, bigButtonHeight))){
                currentScreen = Screen::Options;
            }

            ImGui::SetWindowFontScale(1.0f);

            float smallRowWidth = smallButtonWidth * 2.0f + smallRowGap;
            float smallRowStartX = (windowWidth - smallRowWidth) * 0.5f;

            ImGui::SetWindowFontScale(18.0f / baseFontSize);

            ImGui::SetCursorPos(ImVec2(smallRowStartX, smallRowY));
            if(ImGui::Button("Menu Principale", ImVec2(smallButtonWidth, smallButtonHeight))){
                result = MenuAction::BackToMainMenu;
            }

            //Colori distintivi (rosso) per il pulsante di uscita, come nella versione SFML
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.353f, 0.176f, 0.176f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.549f, 0.235f, 0.235f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.549f, 0.235f, 0.235f, 1.0f));

            ImGui::SetCursorPos(ImVec2(smallRowStartX + smallButtonWidth + smallRowGap, smallRowY));
            if(ImGui::Button("Esci dal gioco", ImVec2(smallButtonWidth, smallButtonHeight))){
                result = MenuAction::QuitGame;
            }

            ImGui::PopStyleColor(3);
            ImGui::SetWindowFontScale(1.0f);

            return result;
        }

        void DrawTitle(){
            ImGui::SetWindowFontScale(56.0f / baseFontSize);
            const char* title = "PAUSA";
            float textWidth = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPos(ImVec2((windowWidth - textWidth) * 0.5f, windowHeight * 0.22f));
            ImGui::TextUnformatted(title);
            ImGui::SetWindowFontScale(1.0f);
        }
    };
}

#endif