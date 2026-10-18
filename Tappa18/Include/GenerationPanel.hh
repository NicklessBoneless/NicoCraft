#ifndef GENERATION_PANEL_HH
#define GENERATION_PANEL_HH

#include <imgui.h>
#include "TerrainConfig.hh"

namespace fcg{
    //Pannello ImGui per scegliere algoritmo, seed e parametri fBm e rigenerare il mondo
    class GenerationPanel{
    public:
        enum class Action{ None, Regenerate };

    private:
        TerrainConfig config;
        double lastGenerationMs = -1.0;

    public:
        explicit GenerationPanel(const TerrainConfig& initialConfig) : config(initialConfig){}

        const TerrainConfig& GetConfig() const{
            return config;
        }

        void SetLastGenerationMs(double milliseconds){
            lastGenerationMs = milliseconds;
        }

        //Va chiamata tra ImGui::SFML::Update() e ImGui::Render()
        Action Draw(){
            Action result = Action::None;

            //Il menu di pausa occupa tutta la finestra: senza il focus forzato potrebbe coprire il pannello
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Generazione terreno", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            int type = (int) config.noiseType;
            if(ImGui::BeginCombo("Algoritmo", NoiseTypeName(config.noiseType))){
                for(int i = 0; i < noiseTypeCount; i++){
                    bool selected = (i == type);
                    if(ImGui::Selectable(NoiseTypeName((NoiseType) i), selected)){
                        config.noiseType = (NoiseType) i;
                    }
                }
                ImGui::EndCombo();
            }

            int seed = (int) config.seed;
            if(ImGui::InputInt("Seed", &seed)){
                config.seed = seed < 0 ? 0u : (uint32_t) seed;
            }

            ImGui::SliderInt("Octaves", &config.fractal.octaves, 1, 8);
            ImGui::SliderFloat("HeightAmplitude", &config.heightAmplitude, 1.f, 100.f, "%.2f");
            ImGui::SliderFloat("Frequency", &config.fractal.frequency, 0.005f, 0.1f, "%.3f");
            ImGui::SliderFloat("Lacunarity", &config.fractal.frequencyMultiplier, 1.5f, 3.5f, "%.2f");
            ImGui::SliderFloat("Persistence", &config.fractal.amplitudeMultiplier, 0.1f, 0.9f, "%.2f");

            ImGui::Separator();
            ImGui::Checkbox("Caverne", &config.caves.enabled);

            if(config.caves.enabled){
                if(ImGui::BeginCombo("Tipo caverna", CaveTypeName(config.caves.type))){
                    for(int i = 0; i < caveTypeCount; i++){
                        bool selected = (i == (int) config.caves.type);
                        if(ImGui::Selectable(CaveTypeName((CaveType) i), selected)){
                            config.caves.type = (CaveType) i;
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SliderInt("Cave octaves", &config.caves.fractal.octaves, 1, 6);
                ImGui::SliderFloat("Cave frequency", &config.caves.fractal.frequency, 0.01f, 0.15f, "%.3f");
                ImGui::SliderFloat("Cave lacunarity", &config.caves.fractal.frequencyMultiplier, 1.5f, 3.5f, "%.2f");
                ImGui::SliderFloat("Cave persistence", &config.caves.fractal.amplitudeMultiplier, 0.1f, 0.9f, "%.2f");

                if(config.caves.type == CaveType::Sponge){
                    ImGui::SliderFloat("Soglia spugna", &config.caves.spongeThreshold, 0.05f, 0.7f, "%.2f");
                }else{
                    ImGui::SliderFloat("Spessore tunnel", &config.caves.wormThreshold, 0.01f, 0.30f, "%.3f");
                }

                ImGui::SliderInt("Margine superficie", &config.caves.surfaceMargin, 0, 12);
            }

            if(ImGui::Button("Rigenera")){
                result = Action::Regenerate;
            }

            if(lastGenerationMs >= 0.0){
                ImGui::Text("Ultima generazione: %.0f ms", lastGenerationMs);
            }

            ImGui::End();
            return result;
        }
    };
}

#endif
