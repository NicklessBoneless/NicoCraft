#ifndef MAIN_MENU_HH
#define MAIN_MENU_HH

#include <imgui.h>
#include <iostream>
#include <string>
#include "OptionsPanel.hh"

namespace fcg{
    //Menu principale, riscritto in ImGui immediate-mode (era puro SFML 2D).
    //Draw() va chiamata una volta per frame, tra ImGui_ImplOpenGL3_NewFrame()/
    //ImGui::SFML::Update() e ImGui::Render(): legge l'input e disegna nello stesso passo
    class MainMenu{
    public:
        enum class MenuAction{ None, GenerateWorld, Exit, FovChanged, ResolutionChanged };

    private:
        enum class Screen{ Main, Options };

        static constexpr float baseFontSize = 24.0f;
        ImFont* font = nullptr;

        OptionsPanel optionsPanel;

        Screen currentScreen = Screen::Main;

        int windowWidth = 1920;
        int windowHeight = 1080;

        static constexpr float buttonWidth = 420.0f;
        static constexpr float buttonHeight = 80.0f;
        static constexpr float buttonSpacing = 26.0f;

    public:
        //resourcesDir e' il path relativo alle risorse condivise (es. "../Resources/").
        //initialFov/initialWidth/initialHeight arrivano dal file di preferenze (Settings.hh)
        MainMenu(const std::string& resourcesDir, float initialFov, int initialWidth, int initialHeight) : optionsPanel(LoadFont(resourcesDir), initialFov, initialWidth, initialHeight){
        }

        //Va richiamata all'avvio e ad ogni sf::Event::Resized
        void SetWindowSize(int width, int height){
            windowWidth = width;
            windowHeight = height;
            optionsPanel.SetWindowSize(width, height, height * 0.10f);
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

        //Disegna il menu e ritorna l'azione da applicare all'esterno (creare il mondo,
        //uscire, salvare le preferenze); la navigazione Main <-> Options resta interna
        MenuAction Draw(){
            MenuAction result = MenuAction::None;

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2((float) windowWidth, (float) windowHeight));

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;

            ImGui::Begin("MainMenu", nullptr, flags);
            ImGui::PushFont(font);

            if(currentScreen == Screen::Main){
                result = DrawMainScreen();
            }
            else{
                OptionsPanel::Action action = optionsPanel.Draw();
                switch(action){
                    case OptionsPanel::Action::Back:
                        currentScreen = Screen::Main;
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
            return result;
        }

    private:
        //Chiamato dall'initializer list PRIMA di optionsPanel (font e' dichiarato sopra
        //di essa nella classe): imposta this->font e lo ritorna per passarlo al costruttore
        //di OptionsPanel, stesso trucco gia' usato nella versione SFML originale
        ImFont* LoadFont(const std::string& resourcesDir){
            ImGuiIO& io = ImGui::GetIO();
            std::string fontPath = resourcesDir + "pixelFont.ttf";
            font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), baseFontSize);
            if(!font){
                std::cerr << "Errore (MainMenu): impossibile caricare pixelFont.ttf, impossibile continuare." << std::endl;
                exit(1);
            }
            return font;
        }

        MenuAction DrawMainScreen(){
            MenuAction result = MenuAction::None;

            DrawTitle();

            float centerX = (windowWidth - buttonWidth) * 0.5f;
            float generateY = windowHeight * 0.34f;
            float optionsY = generateY + buttonHeight + buttonSpacing;
            float exitY = optionsY + buttonHeight + buttonSpacing;

            ImGui::SetWindowFontScale(22.0f / baseFontSize);

            ImGui::SetCursorPos(ImVec2(centerX, generateY));
            if(ImGui::Button("Genera Mondo", ImVec2(buttonWidth, buttonHeight))){
                result = MenuAction::GenerateWorld;
            }

            ImGui::SetCursorPos(ImVec2(centerX, optionsY));
            if(ImGui::Button("Opzioni", ImVec2(buttonWidth, buttonHeight))){
                currentScreen = Screen::Options;
            }

            ImGui::SetCursorPos(ImVec2(centerX, exitY));
            if(ImGui::Button("Esci", ImVec2(buttonWidth, buttonHeight))){
                result = MenuAction::Exit;
            }

            ImGui::SetWindowFontScale(1.0f);

            DrawCommands();
            return result;
        }

        void DrawTitle(){
            ImGui::SetWindowFontScale(64.0f / baseFontSize);
            const char* title = "NicoCraft";
            float textWidth = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPos(ImVec2((windowWidth - textWidth) * 0.5f, windowHeight * 0.10f));
            ImGui::TextUnformatted(title);
            ImGui::SetWindowFontScale(1.0f);
        }

        void DrawCommands(){
            ImGui::SetWindowFontScale(18.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2(40.0f, windowHeight - 340.0f));
            ImGui::TextUnformatted(BuildCommandsString().c_str());
            ImGui::SetWindowFontScale(1.0f);
        }

        static std::string BuildCommandsString(){
            return
                "COMANDI:\n"
                "- Movimento : WASD\n"
                "- Camera    : Mouse\n"
                "- Rompi blocco : LMB\n"
                "- Piazza blocco: RMB\n"
                "- Hotbar : Rotella / 1-7 \n"
                "- Sprint : LShift\n"
                "- Salta  : Spazio \n"
                "- Vai giu' (noclip) : LCTRL\n"
                "- Toggle noclip : F\n"
                "- Pausa : ESC\n";
        }
    };
}

#endif