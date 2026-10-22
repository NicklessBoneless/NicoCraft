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
        
        //Permutazioni, permutations 
        std::array<uint8_t, 512> perm;

        //Funzione 6x^{5}-15x^{4}+10x^{3}
        static float Fade(float x){ 
            float x3 = x*x*x;
            float x4 = x3*x;
            float x5 = x4*x;
            return (6.0f*x5) - (15.0f*x4) + (10.0f*x3);
        }

        /*
            Interpolazione Lineare 
            Dati due valori a e b e un peso x tra 0 e 1, 
            restituisce un valore “a metà strada” tra i due, in proporzione a x.
        */
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
            int direction = hash & 15;

            //Scegliamo quale componente è da scartare (x/y/z),
            //dipendentemente dal gradiente scelto
            float firstComponent;
            if(direction < 8) firstComponent = x;
            else firstComponent = y;
            
            float secondComponent;
            if(direction < 4) secondComponent = y;
            else if(direction == 12 || direction == 14) secondComponent = x;
            else secondComponent = z;

            //Che operazione bisogna effettuare su i due componenti? x + z o -x + z , etc.
            if((direction & 1) != 0){
                firstComponent = -firstComponent;
            }
            if((direction & 2) != 0){
                secondComponent = -secondComponent;
            }

            //Prodotto scalare tra x,y,z, però dato che 
            return firstComponent + secondComponent;
        }

    public: 
        explicit PerlinNoise(uint32_t seed){ 
            //Inizializza la tabella delle permutazioni
            for(uint16_t i = 0; i < (perm.max_size() >> 1) ; i++){
                perm[i] = (uint8_t) i;
            }
            
            //Mescoliamo le permutazioni in base al seed scelto!
            std::mt19937 rng(seed);
            for(int i = (perm.max_size() >> 1)-1 ; i > 0 ; i--){
                int j = (int) (rng() % (uint32_t) (i + 1));
                std::swap(perm[i], perm[j]);
            }

            //Tabella duplicata: evita il wrap degli indici nel calcolo dei corner
            for(uint16_t i = 0; i < (perm.max_size() >> 1) ; i++){
                perm[256 + i] = perm[i];
            }
        }

        float Sample2D(float x, float z) const override{
            float floorX = std::floor(x);
            float floorZ = std::floor(z);
            int xIndexUnitCell = ((int) floorX) & 255; //Equivalente a % 256, serve perché % da valori negativi
            int zIndexUnitCell = ((int) floorZ) & 255; //Equivalente a % 256
            float xLocalCell = x - floorX; //[0,1) 
            float zLocalCell = z - floorZ; //[0,1)

            //Coordinate lisce, per rendere le transizioni tra i blocchi più morbide
            float uSmoothedWeight = Fade(xLocalCell);
            float wSmoothedWeight = Fade(zLocalCell);

            //Generiamo l'hash per ogni angolo del quadrato, che poi useremo per scegliere la direzione del vettore
            int bottomLeft  = perm[perm[xIndexUnitCell] + zIndexUnitCell];         //aa
            int bottomRight = perm[perm[xIndexUnitCell + 1] + zIndexUnitCell];     //ba
            int topLeft     = perm[perm[xIndexUnitCell] + zIndexUnitCell + 1];     //ab
            int topRight    = perm[perm[xIndexUnitCell + 1] + zIndexUnitCell + 1]; //bb

            //Gradient2D effettua il prodotto scalare 
            float dotProductBottomLeft = Gradient2D(bottomLeft, xLocalCell, zLocalCell);
            float dotProductBottomRight = Gradient2D(bottomRight, xLocalCell - 1.0f, zLocalCell);
            
            //Rumore bordo inferiore cella, metà sotto
            //uSmoothedWeight fa da PESO tra [dotProductBottomLeft, dotProductBottomRight]
            float bottomHalfBlend = Lerp(uSmoothedWeight, dotProductBottomLeft, dotProductBottomRight);

            //Prodotto scalare
            float dotProductTopLeft = Gradient2D(topLeft, xLocalCell, zLocalCell - 1.0f);
            float dotProductTopRight = Gradient2D(topRight, xLocalCell - 1.0f, zLocalCell - 1.0f);

            //Rumore bordo superiore cella, metà sopra
            float topHalfBlend = Lerp(uSmoothedWeight, dotProductTopLeft,dotProductTopRight);

            //[-1,1] Tramite l'uso di clamp, tra [bottomHalfBlend, topHalfBlend]
            return std::clamp(Lerp(wSmoothedWeight, bottomHalfBlend, topHalfBlend), -1.0f, 1.0f); 
        }

        float Sample3D(float x, float y, float z) const override{
            //Arrotondiamo verso il basso con floor, 
            //con cast ad (int) avremmo arrotondato verso lo zero!
            float floorX = std::floor(x);
            float floorY = std::floor(y);
            float floorZ = std::floor(z);

            //Valore che sarà usato nella tabella di permutazione per ciascuna coordinata
            int xIndexUnitCell = ((int) floorX) & 255; //Equivalente a % 256, serve perché % da valori negativi
            int yIndexUnitCell = ((int) floorY) & 255; //Equivalente a % 256
            int zIndexUnitCell = ((int) floorZ) & 255; //Equivalente a % 256
            
            //Calcoliamo le coordinate locali di P relative al cubo x,y,z
            float xLocalCell = x - floorX; //[0,1)
            float yLocalCell = y - floorY; //[0,1)
            float zLocalCell = z - floorZ; //[0,1)

            //Pesi, per rendere le transizioni tra i blocchi più morbide
            float uSmoothedWeight = Fade(xLocalCell);
            float vSmoothedWeight = Fade(yLocalCell);
            float wSmoothedWeight = Fade(zLocalCell);

            //Hash: Si concatenano le permutazioni asse x asse (x, poi y, infine z)

            //x , y
            int hashX_Left_Y  = perm[xIndexUnitCell] + yIndexUnitCell;       //a
            //x+1, y
            int hashX_Right_Y = perm[xIndexUnitCell + 1] + yIndexUnitCell;   //b
            
            //Hash degli 8 angoli del cubo: "back" = faccia a z, "front" = faccia a z + 1
            // x, y, z
            int backBottomLeft   = perm[perm[hashX_Left_Y] + zIndexUnitCell];
            // x+1, y, z
            int backBottomRight  = perm[perm[hashX_Right_Y] + zIndexUnitCell];
            // x, y+1, z
            int backTopLeft      = perm[perm[hashX_Left_Y + 1] + zIndexUnitCell];
            // x+1, y+1, z
            int backTopRight     = perm[perm[hashX_Right_Y + 1] + zIndexUnitCell];
            // x, y, z+1
            int frontBottomLeft  = perm[perm[hashX_Left_Y] + zIndexUnitCell + 1];
            // x+1, y, z+1
            int frontBottomRight = perm[perm[hashX_Right_Y] + zIndexUnitCell + 1];
            // x, y+1, z+1
            int frontTopLeft     = perm[perm[hashX_Left_Y + 1] + zIndexUnitCell + 1];
            // x+1, y+1, z+1
            int frontTopRight    = perm[perm[hashX_Right_Y + 1] + zIndexUnitCell + 1];

            //Gradient3D effettua il prodotto scalare, back (z)
            float dotProductBackBottomLeft  = Gradient3D(backBottomLeft, xLocalCell, yLocalCell, zLocalCell);
            float dotProductBackBottomRight = Gradient3D(backBottomRight, xLocalCell - 1.0f, yLocalCell, zLocalCell);
            float dotProductBackTopLeft     = Gradient3D(backTopLeft, xLocalCell, yLocalCell - 1.0f, zLocalCell);
            float dotProductBackTopRight    = Gradient3D(backTopRight, xLocalCell - 1.0f, yLocalCell - 1.0f, zLocalCell);

            //Prodotto scalare, front (z + 1)
            float dotProductFrontBottomLeft  = Gradient3D(frontBottomLeft, xLocalCell, yLocalCell, zLocalCell - 1.0f);
            float dotProductFrontBottomRight = Gradient3D(frontBottomRight, xLocalCell - 1.0f, yLocalCell, zLocalCell - 1.0f);
            float dotProductFrontTopLeft     = Gradient3D(frontTopLeft, xLocalCell, yLocalCell - 1.0f, zLocalCell - 1.0f);
            float dotProductFrontTopRight    = Gradient3D(frontTopRight, xLocalCell - 1.0f, yLocalCell - 1.0f, zLocalCell - 1.0f);

            //Faccia back: 
            //Prima si interpola su x (uSmoothedWeight), poi su y (vSmoothedWeight)
            float backBottomBlend = Lerp(uSmoothedWeight, dotProductBackBottomLeft, dotProductBackBottomRight);
            float backTopBlend    = Lerp(uSmoothedWeight, dotProductBackTopLeft, dotProductBackTopRight);
            float backHalfBlend   = Lerp(vSmoothedWeight, backBottomBlend, backTopBlend);

            //Faccia front
            float frontBottomBlend = Lerp(uSmoothedWeight, dotProductFrontBottomLeft, dotProductFrontBottomRight);
            float frontTopBlend    = Lerp(uSmoothedWeight, dotProductFrontTopLeft, dotProductFrontTopRight);
            float frontHalfBlend   = Lerp(vSmoothedWeight, frontBottomBlend, frontTopBlend);

            //Infine si interpola tra le due facce lungo z (wSmoothedWeight)
            //[-1,1] grazie a clamp
            return std::clamp(Lerp(wSmoothedWeight, backHalfBlend, frontHalfBlend), -1.0f, 1.0f);
        }
    };
}

#endif