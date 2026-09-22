#ifndef COMPASS_HH
#define COMPASS_HH

#include <imgui.h>
#include <cmath>
#include <string>

namespace fcg{

    //Overlay ImGui in alto a sinistra con il punto cardinale approssimato verso cui guarda il
    //player, dedotto dallo yaw della Camera. Solo le 4 direzioni principali (N/S/E/O)
    class Compass{
    private:
        std::string direction = "South";
        ImFont* compassFont;
        static constexpr float marginTop = 20.0f;
        static constexpr float marginLeft = 20.0f;

    public:
        Compass(const std::string& res){
            ImGuiIO& io = ImGui::GetIO();
            std::string fontStr = res + "pixelFont.ttf";
            const char* s = fontStr.c_str();
            compassFont = io.Fonts->AddFontFromFileTTF(s, 24.0f);
        }

        //yawDeg viene da Camera::GetYaw(). Va richiamata una volta per frame prima di Draw()
        void Update(float yawDeg){
            direction = DirectionLabel(yawDeg);
        }

        //Va chiamata tra ImGui_ImplOpenGL3_NewFrame()/ImGui::SFML::Update() e ImGui::Render()
        void Draw(){
            ImGui::SetNextWindowPos(ImVec2(marginLeft, marginTop));
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;

            ImGui::Begin("Compass", nullptr, flags);
            ImGui::PushFont(compassFont);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", direction.c_str());
            ImGui::PopFont();
            ImGui::End();
        }

    private:
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
    };
}

#endif