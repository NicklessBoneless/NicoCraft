#ifndef TERRAIN_GENERATOR_HH
#define TERRAIN_GENERATOR_HH

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <utility>
#include <vector>
 
#include "Chunk.hh"
#include "Noise.hh"
#include "TerrainConfig.hh"

namespace fcg{
    /*
        Genera il contenuto di un singolo chunk a partire da un seed esplicito.
        Non conosce World, mesh o rendering: riceve un Chunk e lo riempie.
        Ogni chunk usa un generatore casuale derivato da (seed, chunkX, chunkZ):
        il risultato non dipende dall'ordine in cui i chunk vengono generati
    */
    class TerrainGenerator{
    private:
        uint32_t seed;
        std::unique_ptr<Noise> noise;
        FractalParams terrainParameters;
        CavesConfig cavesConfig;
        float heightAmplitude;

        static constexpr int baseHeight = Blocks::CHUNK_SIZE_Y / 2; //Quota media del terreno
        static constexpr int dirtDepth = 5; //Strati di terra sotto l'erba
        static constexpr int minTreeDistance = 3; //Distanza minima (in blocchi) tra due tronchi
        static constexpr int treeTrunkHeight = 5;
        static constexpr int treeClearance = treeTrunkHeight + 2; //Spazio da lasciare sopra il terreno per la chioma
        //Ogni campo ha il suo offset (in blocchi), cosi' sacche e tunnel non si sovrappongono sempre negli stessi punti
        static constexpr float spongeOffsetX = 251.3f;
        static constexpr float spongeOffsetY = 97.1f;
        static constexpr float spongeOffsetZ = 173.9f;

        static constexpr float wormAOffsetX = -181.7f;
        static constexpr float wormAOffsetY = 43.9f;
        static constexpr float wormAOffsetZ = 307.3f;

        static constexpr float wormBOffsetX = 131.7f;
        static constexpr float wormBOffsetY = 57.3f;
        static constexpr float wormBOffsetZ = 211.1f;

    public:
        TerrainGenerator(uint32_t seed, std::unique_ptr<Noise> noise, FractalParams terrainParameters = FractalParams{},CavesConfig cavesConfig = CavesConfig{},float heightAmplitude = 30.0f) : 
        seed(seed), 
        noise(std::move(noise)), 
        terrainParameters(terrainParameters),
        cavesConfig(cavesConfig),
        heightAmplitude(heightAmplitude){}

        uint32_t GetSeed() const{
            return seed;
        }

        /*
            Quota Y del primo blocco libero sopra il terreno, in coordinate MONDO.
            Il rumore e' campionato in coordinate mondo (non locali al chunk):
            cosi' non ci sono cuciture ai bordi tra chunk adiacenti
        */
        int GetSurfaceHeight(int worldX, int worldZ) const{
            float MediumElevationValue = Fractal2D(*noise, (float) worldX, (float) worldZ, terrainParameters); //n = [-1,1]
            int height = baseHeight + (int) std::floor(MediumElevationValue * heightAmplitude);
            return std::clamp(height, 2, Blocks::CHUNK_SIZE_Y - treeClearance);
        }     

        void GenerateChunk(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            //Riempo il mondo di blocchi
            FillTerrain(chunk, chunkX, chunkZ);

            //Creo le caverne
            GenerateCaves(chunk, chunkX, chunkZ); 

            //Aggiungo gli alberi e altro
            GenerateTrees(chunk, chunkX, chunkZ);
        }

    private:
        void FillTerrain(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    int surface = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                    for(int y = 0; y < surface; y++){
                        if(y == surface - 1) chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                        else if(y >= (surface-1)-dirtDepth) chunk.Set(x, y, z, Blocks::BlockType::DIRT);
                        else chunk.Set(x, y, z, Blocks::BlockType::STONE);
                    }
                }
            }
        }

        /*
            Scava le caverne campionando il Perlin 3D in coordinate MONDO (niente cuciture tra chunk).
            Si scava solo tra minY e (superficie - surfaceMargin): la crosta resta sempre intatta
        */
        void GenerateCaves(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            if(!cavesConfig.enabled) return;

            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    int worldX = chunkX * Blocks::CHUNK_SIZE_X + x;
                    int worldZ = chunkZ * Blocks::CHUNK_SIZE_Z + z;

                    int surface = GetSurfaceHeight(worldX, worldZ);
                    int maxY = std::min(surface - cavesConfig.surfaceMargin, Blocks::CHUNK_SIZE_Y);

                    for(int y = cavesConfig.minY; y < maxY; y++){
                        if(IsCave(worldX, y, worldZ, surface)){
                            chunk.Set(x, y, z, Blocks::BlockType::AIR);
                        }
                    }
                }
            }
        }

        bool IsCave(int worldX, int worldY, int worldZ, int surface) const{
            float x = (float) worldX;
            float y = (float) worldY;
            float z = (float) worldZ;

            //Unione: basta una delle due morfologie. Se e' gia' una sacca, i tunnel non si calcolano
            if(cavesConfig.spongeEnabled && IsSpongeCave(x, y, z, surface)) return true;
            if(cavesConfig.wormsEnabled && IsWormCave(x, y, z)) return true;
            return false;
        }

        bool IsSpongeCave(float x, float y, float z, int surface) const{
            float value = Fractal3D(*noise,
                x + spongeOffsetX,
                (y + spongeOffsetY) * cavesConfig.verticalScale,
                z + spongeOffsetZ,
                cavesConfig.spongeFractal);

            /*
                nearSurface vale 0 a minY e 1 in superficie: la soglia sale con depthBias,
                quindi vicino al suolo ci sono meno sacche e in profondita' piu' grandi
            */
            float depthRange = (float) std::max(surface - cavesConfig.minY, 1);
            float nearSurface = std::clamp((y - (float) cavesConfig.minY) / depthRange, 0.0f, 1.0f);
            float threshold = cavesConfig.spongeThreshold + cavesConfig.depthBias * nearSurface;

            return value > threshold;
        }

        /*
            Un solo campo con |n| < soglia darebbe pareti sottili (la superficie n=0), non tunnel.
            Si intersecano DUE campi (stesso Perlin 3D, coordinate traslate): i punti vicini a zero
            in entrambi formano una curva. Il secondo campo si calcola solo se il primo e' gia' sotto soglia
        */
        bool IsWormCave(float x, float y, float z) const{
            float first = Fractal3D(*noise,
                x + wormAOffsetX,
                (y + wormAOffsetY) * cavesConfig.wormVerticalScale,
                z + wormAOffsetZ,
                cavesConfig.wormFractal);
            if(std::abs(first) >= cavesConfig.wormThreshold) return false;

            float second = Fractal3D(*noise,
                x + wormBOffsetX,
                (y + wormBOffsetY) * cavesConfig.wormVerticalScale,
                z + wormBOffsetZ,
                cavesConfig.wormFractal);
            return std::abs(second) < cavesConfig.wormThreshold;
        } 

        void GenerateTrees(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            /*
                seed_seq mescola seed e coordinate del chunk: stesso seed e stesso chunk
                danno sempre gli stessi alberi, indipendentemente dall'ordine di generazione
            */
            std::seed_seq seq{seed, static_cast<uint32_t>(chunkX), static_cast<uint32_t>(chunkZ)};
            std::mt19937 rng(seq);

            std::vector<std::pair<int,int>> treePositions; //Tronchi gia' piazzati in questo chunk (x,z)

            //x/z partono da 1 e si fermano a CHUNK_SIZE-2: cosi' x-1, x+1, z-1, z+1 restano sempre in bounds
            for(int x = 1; x < Blocks::CHUNK_SIZE_X - 1; x++){
                for(int z = 1; z < Blocks::CHUNK_SIZE_Z - 1; z++){
                    //Probabilita' 1%. Uso % su rng() invece di uniform_int_distribution
                    //perche' quest'ultima non e' riproducibile tra standard library diverse
                    if(rng() % 100 != 0) continue;
                    if(IsTooCloseToOtherTree(x, z, treePositions)) continue;
                    
                    int groundY = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                    if(!chunk.IsSolid(x,groundY-1,z)) continue;
                    
                    treePositions.push_back({x, z});
                    int treeTop = groundY + treeTrunkHeight;

                    for(int y = groundY; y < treeTop; y++){
                        chunk.Set(x, y, z, Blocks::BlockType::LOGWOOD);
                    }

                    for(int y = groundY + 3; y < treeTop + 1; y++){
                        for(int dx = -1; dx <= 1; dx++){
                            for(int dz = -1; dz <= 1; dz++){
                                if(dx == 0 && dz == 0 && y != treeTop) continue; //Al centro c'e' il tronco
                                PlaceLeaf(chunk, x + dx, y, z + dz);
                            }
                        }
                    }
                }
            }
        }

        //Piazza una foglia solo se la cella è libera: su terreno irregolare non deve scavare le colline vicine
        static void PlaceLeaf(Blocks::Chunk& chunk, int x, int y, int z){
            if(chunk.Get(x, y, z) == Blocks::BlockType::AIR){
                chunk.Set(x, y, z, Blocks::BlockType::LEAVES);
            }
        }

        //Controlla se (x,z) e' entro minTreeDistance da un tronco gia' piazzato
        static bool IsTooCloseToOtherTree(int x, int z, const std::vector<std::pair<int,int>>& treePositions){
            int minDistanceSquared = minTreeDistance * minTreeDistance;
            for(const auto& pos : treePositions){
                int dx = x - pos.first;
                int dz = z - pos.second;
                if(dx * dx + dz * dz < minDistanceSquared) return true;
            }
            return false;
        }
    };
}

#endif
