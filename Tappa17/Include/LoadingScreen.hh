#ifndef LOADING_SCREEN_HH
#define LOADING_SCREEN_HH

#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_impl_opengl3.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Window/Window.hpp>
#include <iostream>
#include <string>

namespace fcg{

    //Carica il logo in una texture OpenGL "usa e getta" per ImGui::Image. Ritorna 0 se il file manca
    inline GLuint LoadLoadingLogoTexture(const std::string& resourcesDir, int& outWidth, int& outHeight){
        sf::Image image;
        if(!image.loadFromFile(resourcesDir + "loadingScreen.png")){
            std::cerr << "Attenzione (LoadingScreen): loadingScreen.png non trovato, mostro solo il testo." << std::endl;
            return 0;
        }

        sf::Vector2u size = image.getSize();
        outWidth = (int) size.x;
        outHeight = (int) size.y;

        GLuint textureID = 0;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) size.x, (GLsizei) size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

    //Disegna UNA schermata di caricamento e la presenta subito a video (window.display()).
    //Gestisce da sola il ciclo NewFrame/Render, dato che viene chiamata fuori dal loop principale
    inline void DrawLoadingScreen(sf::RenderWindow& window, const std::string& resourcesDir){
        int windowWidth = (int) window.getSize().x;
        int windowHeight = (int) window.getSize().y;

        int logoWidthPx = 0, logoHeightPx = 0;
        GLuint logoTexture = LoadLoadingLogoTexture(resourcesDir, logoWidthPx, logoHeightPx);

        glViewport(0, 0, windowWidth, windowHeight);
        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::SFML::Update(window, sf::seconds(1.0f / 60.0f)); //Dt fittizio: schermata singola, non serve un vero delta

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float) windowWidth, (float) windowHeight));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs;

        ImGui::Begin("LoadingScreen", nullptr, flags);

        if(logoTexture != 0){
            //Il logo occupa al massimo un quarto dell'altezza della finestra, proporzioni mantenute
            float maxLogoHeight = windowHeight * 0.25f;
            float scale = maxLogoHeight / (float) logoHeightPx;
            float logoWidth = logoWidthPx * scale;
            float logoHeight = logoHeightPx * scale;

            ImGui::SetCursorPos(ImVec2((windowWidth - logoWidth) * 0.5f, (windowHeight - logoHeight) * 0.5f));
            ImGui::Image((ImTextureID)(intptr_t) logoTexture, ImVec2(logoWidth, logoHeight));
        }

        const char* loadingText = "Generazione del terreno in corso...";
        float textWidth = ImGui::CalcTextSize(loadingText).x;
        ImGui::SetCursorPos(ImVec2((windowWidth - textWidth) * 0.5f, windowHeight * 0.75f));
        ImGui::TextUnformatted(loadingText);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.display();

        if(logoTexture != 0){
            glDeleteTextures(1, &logoTexture);
        }
    }
}

#endif