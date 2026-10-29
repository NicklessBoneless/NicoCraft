#ifndef TERRAIN_CONFIG_HH
#define TERRAIN_CONFIG_HH

#include <cstdint>
#include <memory>

#include "Noise.hh"
#include "PerlinNoise.hh"


namespace fcg{
    enum class NoiseType{ Perlin = 0 };

    constexpr int noiseTypeCount = 1;
    
    inline const char* NoiseTypeName(NoiseType type){
        switch(type){
            case NoiseType::Perlin: return "Perlin";
            default: return "Perlin";
        }
    }

    inline std::unique_ptr<Noise> CreateNoise(NoiseType type, uint32_t seed){
        switch(type){
            default: return std::make_unique<PerlinNoise>(seed);
        }
    }

    struct CavesConfig{
        bool enabled = true;

        //Sacche (cheese): si scava dove il rumore supera la soglia (piu' bassa = piu' vuoto)
        bool spongeEnabled = false;
        FractalParams spongeFractal = {3, 0.05f, 2.0f, 0.5f};
        float spongeThreshold = 0.35f;
        float verticalScale = 1.0f; //>1 schiaccia le sacche in verticale (sale larghe e basse)
        float depthBias = 0.10f;     //Quanto sale la soglia vicino alla superficie (0 = nessun effetto)

        //Tunnel (spaghetti): si scava dove due campi sono entrambi vicini a zero
        bool wormsEnabled = true;
        FractalParams wormFractal = {4, 0.010f, 2.0f, 0.5f};
        float wormThreshold = 0.04f; //Piu' alta = tunnel piu' larghi
        float wormVerticalScale = 1.2f;

        int minY = 3; //Sotto questa quota non si scava mai
        int surfaceMargin = 0; //Strati intatti sotto la superficie (0 = ingressi visibili)
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
