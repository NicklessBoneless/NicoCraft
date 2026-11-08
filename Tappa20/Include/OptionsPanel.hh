#ifndef OPTIONS_PANEL_HH
#define OPTIONS_PANEL_HH

#include <imgui.h>
#include <string>

namespace fcg{
    //Pannello Opzioni (FOV + risoluzione), riscritto in ImGui immediate-mode.
    //Nessuno stato SFML: Draw() legge l'input e disegna nello stesso frame
    class OptionsPanel{
    public:
        enum class Action{ None, Back, FovChanged, ResolutionChanged };

    private:
        struct ResolutionPreset{ int width; int height; };

        ImFont* font; //Font base (24px), caricato e posseduto dal chiamante (MainMenu/PauseMenu)

        float fov = 90.0f;
        static constexpr float minFov = 60.0f;
        static constexpr float maxFov = 110.0f;
        static constexpr float fovStep = 5.0f;
        static constexpr float baseFontSize = 24.0f;

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

    public:
        OptionsPanel(ImFont* font, float initialFov, int initialWidth, int initialHeight) : font(font){
            fov = Clamp(initialFov);
            currentResIndex = FindResolutionIndex(initialWidth, initialHeight);
        }

        //y0 e' l'offset verticale da cui inizia il pannello
        void SetWindowSize(int width, int height, float y0 = 0.0f){
            windowWidth = width;
            windowHeight = height;
            topY = y0;
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

        //Va chiamata dentro il blocco ImGui::Begin/End del chiamante, una volta per frame
        Action Draw(){
            Action result = Action::None;

            ImGui::PushFont(font);

            DrawTitle();
            if(DrawFovRow()) result = Action::FovChanged;
            if(DrawResolutionRow()) result = Action::ResolutionChanged;
            if(DrawBackButton()) result = Action::Back;

            ImGui::PopFont();
            return result;
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
        }

        void CycleResolution(int direction){
            currentResIndex = ((currentResIndex + direction) % resolutionPresetCount + resolutionPresetCount) % resolutionPresetCount;
        }

        void DrawTitle(){
            ImGui::SetWindowFontScale(52.0f / baseFontSize);
            const char* title = "OPZIONI";
            float textWidth = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPos(ImVec2((windowWidth - textWidth) * 0.5f, topY));
            ImGui::TextUnformatted(title);
            ImGui::SetWindowFontScale(1.0f);
        }

        bool DrawFovRow(){
            bool changed = false;
            float rowWidth = smallButtonSize * 2.0f + valueBoxWidth + rowSpacing * 2.0f;
            float rowStartX = (windowWidth - rowWidth) * 0.5f;
            float fovRowY = topY + 130.0f;

            ImGui::SetWindowFontScale(20.0f / baseFontSize);
            const char* label = "FOV";
            ImGui::SetCursorPos(ImVec2((windowWidth - ImGui::CalcTextSize(label).x) * 0.5f, fovRowY - 40.0f));
            ImGui::TextUnformatted(label);

            ImGui::SetWindowFontScale(26.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2(rowStartX, fovRowY));
            if(ImGui::Button("-", ImVec2(smallButtonSize, smallButtonSize))){
                AdjustFov(-fovStep);
                changed = true;
            }

            ImGui::SetWindowFontScale(1.0f);
            std::string fovValue = std::to_string((int) fov);
            float valueBoxX = rowStartX + smallButtonSize + rowSpacing;
            ImVec2 textSize = ImGui::CalcTextSize(fovValue.c_str());
            ImGui::SetCursorPos(ImVec2(valueBoxX + (valueBoxWidth - textSize.x) * 0.5f, fovRowY + (smallButtonSize - textSize.y) * 0.5f));
            ImGui::TextUnformatted(fovValue.c_str());

            ImGui::SetWindowFontScale(26.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2(valueBoxX + valueBoxWidth + rowSpacing, fovRowY));
            if(ImGui::Button("+", ImVec2(smallButtonSize, smallButtonSize))){
                AdjustFov(fovStep);
                changed = true;
            }

            ImGui::SetWindowFontScale(1.0f);
            return changed;
        }

        bool DrawResolutionRow(){
            bool changed = false;
            float rowWidth = smallButtonSize * 2.0f + valueBoxWidth + rowSpacing * 2.0f;
            float rowStartX = (windowWidth - rowWidth) * 0.5f;
            float fovRowY = topY + 130.0f;
            float resRowY = fovRowY + smallButtonSize + 80.0f;

            ImGui::SetWindowFontScale(20.0f / baseFontSize);
            const char* label = "RISOLUZIONE SCHERMO";
            ImGui::SetCursorPos(ImVec2((windowWidth - ImGui::CalcTextSize(label).x) * 0.5f, resRowY - 40.0f));
            ImGui::TextUnformatted(label);

            ImGui::SetWindowFontScale(26.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2(rowStartX, resRowY));
            if(ImGui::Button("<", ImVec2(smallButtonSize, smallButtonSize))){
                CycleResolution(-1);
                changed = true;
            }

            ImGui::SetWindowFontScale(1.0f);
            std::string resValue = std::to_string(GetResolutionWidth()) + " x " + std::to_string(GetResolutionHeight());
            float valueBoxX = rowStartX + smallButtonSize + rowSpacing;
            ImVec2 textSize = ImGui::CalcTextSize(resValue.c_str());
            ImGui::SetCursorPos(ImVec2(valueBoxX + (valueBoxWidth - textSize.x) * 0.5f, resRowY + (smallButtonSize - textSize.y) * 0.5f));
            ImGui::TextUnformatted(resValue.c_str());

            ImGui::SetWindowFontScale(26.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2(valueBoxX + valueBoxWidth + rowSpacing, resRowY));
            if(ImGui::Button(">", ImVec2(smallButtonSize, smallButtonSize))){
                CycleResolution(1);
                changed = true;
            }

            ImGui::SetWindowFontScale(16.0f / baseFontSize);
            const char* note = "Si applica al prossimo avvio";
            ImGui::SetCursorPos(ImVec2((windowWidth - ImGui::CalcTextSize(note).x) * 0.5f, resRowY + smallButtonSize + 12.0f));
            ImGui::TextUnformatted(note);

            ImGui::SetWindowFontScale(1.0f);
            return changed;
        }

        bool DrawBackButton(){
            float fovRowY = topY + 130.0f;
            float resRowY = fovRowY + smallButtonSize + 80.0f;
            float backY = resRowY + smallButtonSize + 70.0f;

            ImGui::SetWindowFontScale(22.0f / baseFontSize);
            ImGui::SetCursorPos(ImVec2((windowWidth - buttonWidth) * 0.5f, backY));
            bool clicked = ImGui::Button("Indietro", ImVec2(buttonWidth, buttonHeight));
            ImGui::SetWindowFontScale(1.0f);
            return clicked;
        }
    };
}

#endif