#ifndef SETTINGS_HH
#define SETTINGS_HH

#include <fstream>
#include <string>
#include <iostream>

namespace fcg{

    //Preferenze persistite su file tra un avvio e l'altro. La risoluzione NON viene mai
    //applicata a video mentre il programma e' in esecuzione (eviterebbe di dover
    //ricreare finestra e contesto OpenGL): viene solo salvata qui e riletta all'avvio
    //successivo, cosi' la finestra nasce gia' alla dimensione scelta
    struct Settings{
        int width = 1920;
        int height = 1080;
        float fov = 90.0f;
    };

    //Se il file non esiste ancora (prima esecuzione) ritorna i valori di default
    inline Settings LoadSettings(const std::string& path){
        Settings settings;

        std::ifstream file(path);
        if(!file.is_open()) return settings;

        std::string line;
        while(std::getline(file, line)){
            size_t separator = line.find('=');
            if(separator == std::string::npos) continue;

            std::string key = line.substr(0, separator);
            std::string value = line.substr(separator + 1);

            if(key == "width") settings.width = std::stoi(value);
            else if(key == "height") settings.height = std::stoi(value);
            else if(key == "fov") settings.fov = std::stof(value);
        }

        return settings;
    }

    inline void SaveSettings(const std::string& path, const Settings& settings){
        std::ofstream file(path);
        if(!file.is_open()){
            std::cerr << "Errore (Settings): impossibile salvare le preferenze in " << path << std::endl;
            return;
        }

        file << "width=" << settings.width << "\n";
        file << "height=" << settings.height << "\n";
        file << "fov=" << settings.fov << "\n";
    }
}

#endif
