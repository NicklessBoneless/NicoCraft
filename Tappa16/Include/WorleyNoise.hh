#ifndef WORLEY_NOISE_HH
#define WORLEY_NOISE_HH

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "Noise.hh"

namespace fcg{
    enum class WorleyMode{ F1, F2MinusF1 };

    /*
        Rumore cellulare (Worley 1996): un punto caratteristico per cella, posizionato
        da un hash di (cella, seed). Il valore dipende dalle distanze F1 (punto piu' vicino)
        e F2 (secondo piu' vicino), cercate nelle 3x3 celle attorno al campione (3x3x3 in 3D).
        Tutto e' deterministico: stesso seed, stesso campo, niente stato da memorizzare
    */
    class WorleyNoise : public Noise{
    private:
        uint32_t seed;
        WorleyMode mode;

        //Finalizzatore di tipo murmur: mescola bene i bit di un intero a 32 bit
        static uint32_t Hash(uint32_t value){
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;
            return value;
        }

        //Valore pseudo-casuale in [0, 1) legato alla cella e alla componente (0 = x, 1 = y, 2 = z)
        float CellRandom(int cellX, int cellY, int cellZ, uint32_t component) const{
            uint32_t h = Hash(seed + component * 0x9e3779b9u);
            h = Hash(h + (uint32_t) cellX);
            h = Hash(h + (uint32_t) cellY);
            h = Hash(h + (uint32_t) cellZ);
            return (float) (h >> 8) / 16777216.0f;
        }

        /*
            Porta le distanze in [-1, 1], come richiede l'interfaccia Noise.
            F1: massimo (+1) nei punti caratteristici, quindi cupole attorno a ogni punto.
            F2-F1: minimo (-1) sui confini tra celle, quindi placche separate da fratture.
            La distanza viene troncata a 1 (la cella ha lato 1): e' una scelta provvisoria,
            la normalizzazione vera si decide con le metriche del passo 5
        */
        float Remap(float f1, float f2) const{
            if(mode == WorleyMode::F1) return 1.0f - 2.0f * std::min(f1, 1.0f);
            return 2.0f * std::min(f2 - f1, 1.0f) - 1.0f;
        }

        static void Insert(float distanceSquared, float& f1, float& f2){
            if(distanceSquared < f1){
                f2 = f1;
                f1 = distanceSquared;
            }
            else if(distanceSquared < f2){
                f2 = distanceSquared;
            }
        }

    public:
        WorleyNoise(uint32_t seed, WorleyMode mode = WorleyMode::F1) : seed(seed), mode(mode){}

        float Sample2D(float x, float z) const override{
            int cellX = (int) std::floor(x);
            int cellZ = (int) std::floor(z);

            float f1 = std::numeric_limits<float>::max();
            float f2 = std::numeric_limits<float>::max();

            for(int dz = -1; dz <= 1; dz++){
                for(int dx = -1; dx <= 1; dx++){
                    int neighborX = cellX + dx;
                    int neighborZ = cellZ + dz;
                    float pointX = neighborX + CellRandom(neighborX, 0, neighborZ, 0);
                    float pointZ = neighborZ + CellRandom(neighborX, 0, neighborZ, 2);
                    float distX = pointX - x;
                    float distZ = pointZ - z;
                    Insert(distX * distX + distZ * distZ, f1, f2);
                }
            }
            return Remap(std::sqrt(f1), std::sqrt(f2));
        }

        float Sample3D(float x, float y, float z) const override{
            int cellX = (int) std::floor(x);
            int cellY = (int) std::floor(y);
            int cellZ = (int) std::floor(z);

            float f1 = std::numeric_limits<float>::max();
            float f2 = std::numeric_limits<float>::max();

            for(int dz = -1; dz <= 1; dz++){
                for(int dy = -1; dy <= 1; dy++){
                    for(int dx = -1; dx <= 1; dx++){
                        int neighborX = cellX + dx;
                        int neighborY = cellY + dy;
                        int neighborZ = cellZ + dz;
                        float pointX = neighborX + CellRandom(neighborX, neighborY, neighborZ, 0);
                        float pointY = neighborY + CellRandom(neighborX, neighborY, neighborZ, 1);
                        float pointZ = neighborZ + CellRandom(neighborX, neighborY, neighborZ, 2);
                        float distX = pointX - x;
                        float distY = pointY - y;
                        float distZ = pointZ - z;
                        Insert(distX * distX + distY * distY + distZ * distZ, f1, f2);
                    }
                }
            }
            return Remap(std::sqrt(f1), std::sqrt(f2));
        }
    };
}

#endif
