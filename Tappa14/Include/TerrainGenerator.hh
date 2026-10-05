#ifndef TERRAIN_GENERATOR_HH
#define TERRAIN_GENERATOR_HH

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "Chunk.hh"

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

        static constexpr int flatSurface = Blocks::CHUNK_SIZE_Y / 2;
        static constexpr int minTreeDistance = 3; //Distanza minima (in blocchi) tra due tronchi
        static constexpr int treeTrunkHeight = 5;

    public:
        explicit TerrainGenerator(uint32_t seed) : seed(seed){}

        uint32_t GetSeed() const{
            return seed;
        }

        //Quota Y del primo blocco libero sopra il terreno, in coordinate MONDO.
        //Per ora piatta: nei prossimi passi leggera' la heightmap del rumore
        int GetSurfaceHeight(int worldX, int worldZ) const{
            return flatSurface;
        }

        void GenerateChunk(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            FillTerrain(chunk, chunkX, chunkZ);
            GenerateTrees(chunk, chunkX, chunkZ);
        }

    private:
        void FillTerrain(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    int surface = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                    for(int y = 0; y < surface; y++){
                        if(y == surface - 1) chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                        else if(y < surface * 0.90) chunk.Set(x, y, z, Blocks::BlockType::STONE);
                        else chunk.Set(x, y, z, Blocks::BlockType::DIRT);
                    }
                }
            }
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

                    treePositions.push_back({x, z});

                    int groundY = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                    int treeTop = groundY + treeTrunkHeight;

                    for(int y = groundY; y < treeTop; y++){
                        chunk.Set(x, y, z, Blocks::BlockType::LOGWOOD);
                    }

                    for(int y = groundY + 3; y < treeTop + 1; y++){
                        if(y == treeTop) chunk.Set(x, y, z, Blocks::BlockType::LEAVES);
                        chunk.Set(x + 1, y, z + 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x - 1, y, z - 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x - 1, y, z + 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x + 1, y, z - 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x + 1, y, z,     Blocks::BlockType::LEAVES);
                        chunk.Set(x,     y, z + 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x,     y, z - 1, Blocks::BlockType::LEAVES);
                        chunk.Set(x - 1, y, z,     Blocks::BlockType::LEAVES);
                    }
                }
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
