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
        bool spongeEnabled = true;
        FractalParams spongeFractal = {4, 0.018f, 2.0f, 0.45f}; //oct.,freq.,lacun.,persist.
        float spongeThreshold = 0.28f;
        float verticalScale = 1.0f; //>1 schiaccia le sacche in verticale (sale larghe e basse)
        float depthBias = 0.09f;     //Quanto sale la soglia vicino alla superficie (0 = nessun effetto)

        //Tunnel (spaghetti): si scava dove due campi sono entrambi vicini a zero
        bool wormsEnabled = !true;
        FractalParams wormFractal = {4, 0.012f, 2.0f, 0.5f};
        float wormThreshold = 0.033f; //Piu' alta = tunnel piu' larghi
        float wormVerticalScale = 1.29f;

        int minCeilingHeight = 2;
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
