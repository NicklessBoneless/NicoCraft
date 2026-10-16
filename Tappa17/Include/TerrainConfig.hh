#ifndef TERRAIN_CONFIG_HH
#define TERRAIN_CONFIG_HH

#include <cstdint>
#include <memory>

#include "Noise.hh"
#include "PerlinNoise.hh"


namespace fcg{
    enum class NoiseType{ Perlin = 0 };
    enum class CaveType{ Sponge = 0, Worms = 1};

    constexpr int noiseTypeCount = 1;
    constexpr int caveTypeCount = 2;
    
    inline const char* NoiseTypeName(NoiseType type){
        switch(type){
            case NoiseType::Perlin: return "Perlin";
            default: return "Perlin";
        }
    }

    inline const char* CaveTypeName(CaveType type){
        switch(type){
            case CaveType::Sponge: return "Sponge";
            case CaveType::Worms: return "Vorm";
            default: return "Sponge";
        }
    }

    inline std::unique_ptr<Noise> CreateNoise(NoiseType type, uint32_t seed){
        switch(type){
            default: return std::make_unique<PerlinNoise>(seed);
        }
    }

    struct CavesConfig{
        bool enabled = true;
        CaveType type = CaveType::Sponge;
        FractalParams fractal = {3, 0.05f, 2.0f, 0.5f};

        //Sponge: si scava dove il rumore supera la soglia (piu' bassa = piu' vuoto)
        float spongeThreshold = 0.30f;

        //Vorm: si scava dove |rumore| e' sotto la soglia (piu' alta = tunnel piu' larghi)
        float wormThreshold = 0.08f;

        int minY = 3; //Sotto questa quota non si scava mai
        int surfaceMargin = 0; //Strati intatti sotto la superficie, cosi' le caverne non bucano il terreno
    };

    //Dati usati per la generazione del mondo, rende i mondi replicabili
    struct TerrainConfig{
        NoiseType noiseType = NoiseType::Perlin;
        CavesConfig caves;
        uint32_t seed = 8008;
        FractalParams fractal;
        float heightAmplitude = 30.0f;
    };


}

#endif
