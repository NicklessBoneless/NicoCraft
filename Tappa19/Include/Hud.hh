#ifndef HUD_HH
#define HUD_HH

#include <imgui.h>
#include <cmath>
#include <string>
#include "RenderStats.hh"

namespace fcg{
    /*
        Overlay ImGui con le informazioni di gioco sempre a schermo:
        - in alto a sinistra il punto cardinale approssimato verso cui guarda il player (dallo yaw della Camera)
        - in alto a destra gli FPS correnti, mediati su refreshInterval secondi
    */
    class Hud{
    private:
        ImFont* hudFont;

        //Compass
        std::string direction = "South";

        //FPS
        float accumulatedTime = 0.0f;
        int frameCount = 0;
        int currentFps = 0;
        bool showStats = false;

        static constexpr float refreshInterval = 0.5f;
        static constexpr float marginTop = 20.0f;
        static constexpr float marginLeft = 20.0f;
        static constexpr float marginRight = 20.0f;
        static constexpr float fontSize = 24.0f;

    public:
        Hud(const std::string& res){
            ImGuiIO& io = ImGui::GetIO();
            std::string fontStr = res + "pixelFont.ttf";
            hudFont = io.Fonts->AddFontFromFileTTF(fontStr.c_str(), fontSize);
        }

        //yawDeg viene da Camera::GetYaw(). Va richiamata una volta per frame prima di Draw()
        void Update(float yawDeg, float deltaTime){
            direction = DirectionLabel(yawDeg);

            accumulatedTime += deltaTime;
            frameCount++;

            if(accumulatedTime >= refreshInterval){
                currentFps = (int) std::round(frameCount / accumulatedTime);
                accumulatedTime = 0.0f;
                frameCount = 0;
            }
        }

        //Va chiamata tra ImGui::SFML::Update() e ImGui::Render()
        void Draw(){
            DrawCompass();
            DrawFps();
        }

        void ToggleStats(){
            showStats = !showStats;
        }

        //Va chiamata dopo Draw(), sempre tra ImGui::SFML::Update() e ImGui::Render()
        void DrawStats(const RenderStats& stats, bool frustumEnabled,bool occlusionEnabled){
            if(!showStats) return;

            ImGui::SetNextWindowPos(ImVec2(marginLeft, marginTop + 40.0f));
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::Begin("RenderStats", nullptr, OverlayFlags());
            ImGui::PushFont(hudFont);
            ImGui::SetWindowFontScale(18.0f / fontSize);

            ImVec4 white(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::TextColored(white, "Frustum Culling: %s", frustumEnabled ? "ON" : "OFF");
            ImGui::TextColored(white, "Occlusion Culling: %s", occlusionEnabled ? "ON" : "OFF");
            ImGui::TextColored(white, "Sezioni: %d / %d", stats.sectionsDrawn, stats.sectionsTotal);
            ImGui::TextColored(white, "Vertici: %lld / %lld", stats.verticesDrawn, stats.verticesTotal);
            ImGui::TextColored(white, "Triangoli: %lld / %lld", stats.trianglesDrawn, stats.trianglesTotal);

            ImGui::PopFont();
            ImGui::End();
        }

    private:
        static ImGuiWindowFlags OverlayFlags(){
            return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;
        }

        void DrawCompass(){
            ImGui::SetNextWindowPos(ImVec2(marginLeft, marginTop));
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::Begin("Compass", nullptr, OverlayFlags());
            ImGui::PushFont(hudFont);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", direction.c_str());
            ImGui::PopFont();
            ImGui::End();
        }

        void DrawFps(){
            ImGuiIO& io = ImGui::GetIO();

            //Pivot (1,0): l'angolo in alto a destra della finestra coincide col punto indicato,
            //quindi il testo resta ancorato al bordo destro qualunque sia la sua larghezza
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - marginRight, marginTop), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::Begin("FpsCounter", nullptr, OverlayFlags());
            ImGui::PushFont(hudFont);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d FPS", currentFps);
            ImGui::PopFont();
            ImGui::End();
        }

        //A yaw=0 il forward della Camera e' -Z (vedi Camera::GetForward): -Z=Nord, +X=Est,
        //+Z=Sud, -X=Ovest. 4 settori da 90 gradi
        static std::string DirectionLabel(float yawDeg){
            float normalized = std::fmod(yawDeg, 360.0f);
            if(normalized < 0.0f) normalized += 360.0f;

            if(normalized >= 315.0f || normalized < 45.0f) return "South";
            if(normalized < 135.0f) return "Ovest";
            if(normalized < 225.0f) return "North";
            return "East";
        }
    };
}

#endif
