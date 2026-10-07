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
        std::array<uint8_t, 512> permutation;

        static float Fade(float t){
            return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        }

        static float Lerp(float t, float a, float b){
            return a + t * (b - a);
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
        explicit PerlinNoise(uint32_t seed){
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
            int xi = ((int) floorX) & 255;
            int zi = ((int) floorZ) & 255;
            float xf = x - floorX;
            float zf = z - floorZ;

            float u = Fade(xf);
            float w = Fade(zf);

            int aa = permutation[permutation[xi] + zi];
            int ab = permutation[permutation[xi] + zi + 1];
            int ba = permutation[permutation[xi + 1] + zi];
            int bb = permutation[permutation[xi + 1] + zi + 1];

            float x1 = Lerp(u, Gradient2D(aa, xf, zf),        Gradient2D(ba, xf - 1.0f, zf));
            float x2 = Lerp(u, Gradient2D(ab, xf, zf - 1.0f), Gradient2D(bb, xf - 1.0f, zf - 1.0f));

            return std::clamp(Lerp(w, x1, x2), -1.0f, 1.0f);
        }

        float Sample3D(float x, float y, float z) const override{
            float floorX = std::floor(x);
            float floorY = std::floor(y);
            float floorZ = std::floor(z);
            int xi = ((int) floorX) & 255;
            int yi = ((int) floorY) & 255;
            int zi = ((int) floorZ) & 255;
            float xf = x - floorX;
            float yf = y - floorY;
            float zf = z - floorZ;

            float u = Fade(xf);
            float v = Fade(yf);
            float w = Fade(zf);

            int a  = permutation[xi] + yi;
            int aa = permutation[a] + zi;
            int ab = permutation[a + 1] + zi;
            int b  = permutation[xi + 1] + yi;
            int ba = permutation[b] + zi;
            int bb = permutation[b + 1] + zi;

            float y1 = Lerp(v,
                Lerp(u, Gradient3D(permutation[aa], xf, yf, zf),        Gradient3D(permutation[ba], xf - 1.0f, yf, zf)),
                Lerp(u, Gradient3D(permutation[ab], xf, yf - 1.0f, zf), Gradient3D(permutation[bb], xf - 1.0f, yf - 1.0f, zf)));
            float y2 = Lerp(v,
                Lerp(u, Gradient3D(permutation[aa + 1], xf, yf, zf - 1.0f),        Gradient3D(permutation[ba + 1], xf - 1.0f, yf, zf - 1.0f)),
                Lerp(u, Gradient3D(permutation[ab + 1], xf, yf - 1.0f, zf - 1.0f), Gradient3D(permutation[bb + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f)));

            return std::clamp(Lerp(w, y1, y2), -1.0f, 1.0f);
        }
    };
}

#endif