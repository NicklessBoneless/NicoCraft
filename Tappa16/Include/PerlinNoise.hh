#ifndef PERLIN_NOISE_HH
#define PERLIN_NOISE_HH

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include "Noise.hh"

namespace fcg{
    /*  
    Improved Perlin Noise (Perlin 2002) con tabella di permutazione generata dal seed.
    Il rimescolamento e' un Fisher-Yates fatto a mano con mt19937: std::shuffle non e'
    riproducibile tra standard library diverse, e per il confronto serve lo stesso mondo ovunque 
    */
    class PerlinNoise : public Noise{
    private:
        //Permutazioni 
        std::array<uint8_t, 512> permutation;

        //Funzione 6x^{5}-15x^{4}+10x^{3}
        static float Fade(float x){ 
            float x3 = x*x*x;
            float x4 = x3*x;
            float x5 = x4*x;
            return (6.0f*x5) - (15.0f*x4) + (10.0f*x3);
        }

        static float Lerp(float x, float a, float b){
            return a + x * (b - a);
        }

        //8 gradienti nel piano: 4 diagonali e 4 assiali
        static float Gradient2D(int hash, float x, float z){
            switch(hash & 7){
                case 0: return  x + z;
                case 1: return -x + z;
                case 2: return  x - z;
                case 3: return -x - z;
                case 4: return  x;
                case 5: return -x;
                case 6: return  z;
                default: return -z;
            }
        }

        //12 gradienti verso i punti medi degli spigoli del cubo (piu' 4 ripetuti)
        static float Gradient3D(int hash, float x, float y, float z){
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : ((h == 12 || h == 14) ? x : z);
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }

    public: 
        explicit PerlinNoise(uint32_t seed){ //Inizializza la tabella delle permutazioni
            for(int i = 0; i < 256; i++){
                permutation[i] = (uint8_t) i;
            }

            std::mt19937 rng(seed);
            for(int i = 255; i > 0; i--){
                int j = (int) (rng() % (uint32_t) (i + 1));
                std::swap(permutation[i], permutation[j]);
            }

            //Tabella duplicata: evita il wrap degli indici nel calcolo dei corner
            for(int i = 0; i < 256; i++){
                permutation[256 + i] = permutation[i];
            }
        }

        float Sample2D(float x, float z) const override{
            float floorX = std::floor(x);
            float floorZ = std::floor(z);
            int xIndexUnitCell = ((int) floorX) & 255; //Equivalente a % 256, serve perché % da valori negativi
            int zIndexUnitCell = ((int) floorZ) & 255; //Equivalente a % 256
            float xLocalCell = x - floorX; //[0,1) 
            float zLocalCell = z - floorZ; //[0,1)

            float u = Fade(xLocalCell);
            float w = Fade(zLocalCell);

            int aa = permutation[permutation[xIndexUnitCell] + zIndexUnitCell];
            int ab = permutation[permutation[xIndexUnitCell] + zIndexUnitCell + 1];
            int ba = permutation[permutation[xIndexUnitCell + 1] + zIndexUnitCell];
            int bb = permutation[permutation[xIndexUnitCell + 1] + zIndexUnitCell + 1];

            float x1 = Lerp(u, Gradient2D(aa, xLocalCell, zLocalCell),        Gradient2D(ba, xLocalCell - 1.0f, zLocalCell));
            float x2 = Lerp(u, Gradient2D(ab, xLocalCell, zLocalCell - 1.0f), Gradient2D(bb, xLocalCell - 1.0f, zLocalCell - 1.0f));

            return std::clamp(Lerp(w, x1, x2), -1.0f, 1.0f);
        }

        float Sample3D(float x, float y, float z) const override{
            float floorX = std::floor(x);
            float floorY = std::floor(y);
            float floorZ = std::floor(z);
            int xIndexUnitCell = ((int) floorX) & 255;
            int yi = ((int) floorY) & 255;
            int zIndexUnitCell = ((int) floorZ) & 255;
            float xLocalCell = x - floorX;
            float yf = y - floorY;
            float zLocalCell = z - floorZ;

            float u = Fade(xLocalCell);
            float v = Fade(yf);
            float w = Fade(zLocalCell);

            int a  = permutation[xIndexUnitCell] + yi;
            int aa = permutation[a] + zIndexUnitCell;
            int ab = permutation[a + 1] + zIndexUnitCell;
            int b  = permutation[xIndexUnitCell + 1] + yi;
            int ba = permutation[b] + zIndexUnitCell;
            int bb = permutation[b + 1] + zIndexUnitCell;

            float y1 = Lerp(v,
                Lerp(u, Gradient3D(permutation[aa], xLocalCell, yf, zLocalCell),        Gradient3D(permutation[ba], xLocalCell - 1.0f, yf, zLocalCell)),
                Lerp(u, Gradient3D(permutation[ab], xLocalCell, yf - 1.0f, zLocalCell), Gradient3D(permutation[bb], xLocalCell - 1.0f, yf - 1.0f, zLocalCell)));
            float y2 = Lerp(v,
                Lerp(u, Gradient3D(permutation[aa + 1], xLocalCell, yf, zLocalCell - 1.0f),        Gradient3D(permutation[ba + 1], xLocalCell - 1.0f, yf, zLocalCell - 1.0f)),
                Lerp(u, Gradient3D(permutation[ab + 1], xLocalCell, yf - 1.0f, zLocalCell - 1.0f), Gradient3D(permutation[bb + 1], xLocalCell - 1.0f, yf - 1.0f, zLocalCell - 1.0f)));

            return std::clamp(Lerp(w, y1, y2), -1.0f, 1.0f);
        }
    };
}

#endif