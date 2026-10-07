#ifndef TERRAIN_CONFIG_HH
#define TERRAIN_CONFIG_HH

#include <cstdint>
#include <memory>

#include "Noise.hh"
#include "PerlinNoise.hh"
#include "WorleyNoise.hh"

namespace fcg{
    enum class NoiseType{ Perlin = 0, WorleyF1, WorleyF2MinusF1 };

    constexpr int noiseTypeCount = 3;

    inline const char* NoiseTypeName(NoiseType type){
        switch(type){
            case NoiseType::Perlin: return "Perlin";
            case NoiseType::WorleyF1: return "Worley F1";
            default: return "Worley F2-F1";
        }
    }

    inline std::unique_ptr<Noise> CreateNoise(NoiseType type, uint32_t seed){
        switch(type){
            case NoiseType::WorleyF1:
                return std::make_unique<WorleyNoise>(seed, WorleyMode::F1);
            case NoiseType::WorleyF2MinusF1:
                return std::make_unique<WorleyNoise>(seed, WorleyMode::F2MinusF1);
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
