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
            default:
                return std::make_unique<PerlinNoise>(seed);
        }
    }

    //Tutto cio' che serve per riprodurre esattamente lo stesso mondo
    struct TerrainConfig{
        NoiseType noiseType = NoiseType::Perlin;
        uint32_t seed = 8008;
        FractalParams fractal;
    };
}

#endif
