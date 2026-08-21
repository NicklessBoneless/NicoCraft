#ifndef WORLD_HH
#define WORLD_HH

#include <vector>
#include <cmath>
#include <limits>
#include <cstdlib>

#include "Blocks.hh"
#include "Chunk.hh"
#include "IWorld.hh"
#include "Player.hh"

namespace fcg
{
    constexpr int WORLDSIZECHUNKSX = 4;
    constexpr int WORLDSIZECHUNKSZ = 4;

    //Un chunk posizionato nella griglia mondo (coordinate in unita' di chunk, non di blocco)
    struct ChunkInstance{
        Blocks::Chunk chunk;
        Blocks::ChunkMesh mesh;
        int chunkX;
        int chunkZ;
    };

    //Se colpisce un blocco esso ritorna le coordinate MONDO del blocco
    struct RaycastHit{
        bool hit = false;
        int blockX = 0;
        int blockY = 0;
        int blockZ = 0;
        int hitFace = -1; //Faccia colpita del blocco (indice in Blocks::FACE_OFFSETS), -1 se hit=false
    };

    //Simulazione del mondo voxel: chunk, editing (break/place), raycast, rebuild mesh su modifica.
    //Non conosce shader, uniform o draw call: quello e' compito di Renderer (vedi Renderer.hh)
    class World : public fcg::IWorld{
    public:
        std::vector<ChunkInstance> chunks;

    public:
        World(){
            chunks.reserve(WORLDSIZECHUNKSX * WORLDSIZECHUNKSZ);
            GenerateAllChunks();
            BuildAllMeshes();
        }

        const std::vector<ChunkInstance>& GetChunks() const{
            return chunks;
        }

        //Lancia un raggio da 'origin' lungo 'direction' (non serve normalizzata) e restituisce
        //il primo blocco solido incontrato entro 'maxDistance', con voxel traversal DDA
        RaycastHit RaycastBlock(glm::vec3 origin, glm::vec3 direction, float maxDistance){
            RaycastHit result;

            int x = (int) std::floor(origin.x);
            int y = (int) std::floor(origin.y);
            int z = (int) std::floor(origin.z);

            int stepX = direction.x > 0.0f ? 1 : -1;
            int stepY = direction.y > 0.0f ? 1 : -1;
            int stepZ = direction.z > 0.0f ? 1 : -1;

            float tDeltaX = direction.x != 0.0f ? std::abs(1.0f / direction.x) : std::numeric_limits<float>::infinity();
            float tDeltaY = direction.y != 0.0f ? std::abs(1.0f / direction.y) : std::numeric_limits<float>::infinity();
            float tDeltaZ = direction.z != 0.0f ? std::abs(1.0f / direction.z) : std::numeric_limits<float>::infinity();

            float tMaxX = NextBoundary(origin.x, x, direction.x, stepX);
            float tMaxY = NextBoundary(origin.y, y, direction.y, stepY);
            float tMaxZ = NextBoundary(origin.z, z, direction.z, stepZ);

            int enteredFace = -1; //Faccia da cui si e' entrati nella cella corrente

            float traveled = 0.0f;
            while(traveled < maxDistance){
                if(IsSolidAtWorld(x, y, z)){
                    result.hit = true;
                    result.blockX = x;
                    result.blockY = y;
                    result.blockZ = z;
                    result.hitFace = enteredFace;
                    return result;
                }

                if(tMaxX < tMaxY && tMaxX < tMaxZ){
                    x += stepX;
                    traveled = tMaxX;
                    tMaxX += tDeltaX;
                    enteredFace = stepX > 0 ? 2 : 3; //Entrando in +X si tocca la faccia Left(2) del blocco; in -X la Right(3)
                }
                else if(tMaxY < tMaxZ){
                    y += stepY;
                    traveled = tMaxY;
                    tMaxY += tDeltaY;
                    enteredFace = stepY > 0 ? 5 : 4; //+Y -> Bottom(5), -Y -> Top(4)
                }
                else{
                    z += stepZ;
                    traveled = tMaxZ;
                    tMaxZ += tDeltaZ;
                    enteredFace = stepZ > 0 ? 1 : 0; //+Z -> Back(1), -Z -> Front(0)
                }
            }

            return result;
        }

        //Rompe (rimuove) il blocco alle coordinate MONDO indicate, aggiornando la mesh
        //del chunk modificato e, se necessario, quella dei chunk adiacenti
        void BreakBlockWorld(int worldX, int worldY, int worldZ){
            if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
                return;
            }

            int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
            int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

            ChunkInstance* instance = GetChunkInstanceAt(chunkX, chunkZ);
            if(!instance){
                return;
            }

            int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
            int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

            instance->chunk.Set(localX, worldY, localZ, Blocks::BlockType::AIR);
            RebuildChunkMesh(*instance);

            //Se il blocco era sul bordo del chunk, il vicino potrebbe aver nascosto
            //(o mostrato) una faccia in base al vecchio stato: va rigenerato anche lui
            if(localX == 0) RebuildNeighborIfExists(chunkX - 1, chunkZ);
            if(localX == Blocks::CHUNK_SIZE_X - 1) RebuildNeighborIfExists(chunkX + 1, chunkZ);
            if(localZ == 0) RebuildNeighborIfExists(chunkX, chunkZ - 1);
            if(localZ == Blocks::CHUNK_SIZE_Z - 1) RebuildNeighborIfExists(chunkX, chunkZ + 1);
        }

        //Piazza un blocco alle coordinate MONDO indicate, aggiornando la mesh
        //del chunk modificato e, se necessario, quella dei chunk adiacenti
        void PlaceBlockWorld(int worldX, int worldY, int worldZ, Blocks::BlockType type){
            if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
                return;
            }

            int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
            int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

            ChunkInstance* instance = GetChunkInstanceAt(chunkX, chunkZ);
            if(!instance){
                return;
            }

            int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
            int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

            //Sicurezza: non sovrascrivere un blocco gia' solido (non dovrebbe succedere, placeX/Y/Z e' gia' aria)
            if(instance->chunk.IsSolid(localX, worldY, localZ)){
                return;
            }

            instance->chunk.Set(localX, worldY, localZ, type);
            RebuildChunkMesh(*instance);

            if(localX == 0) RebuildNeighborIfExists(chunkX - 1, chunkZ);
            if(localX == Blocks::CHUNK_SIZE_X - 1) RebuildNeighborIfExists(chunkX + 1, chunkZ);
            if(localZ == 0) RebuildNeighborIfExists(chunkX, chunkZ - 1);
            if(localZ == Blocks::CHUNK_SIZE_Z - 1) RebuildNeighborIfExists(chunkX, chunkZ + 1);
        }

        //Traduce l'input in coda sul Player (break/place) in modifiche effettive al mondo.
        //E' logica di dominio (manipola i blocchi), non orchestrazione: per questo vive qui
        //e non in NicoCraft.cc
        //Traduce l'input in coda sul Player (break/place) in modifiche effettive al mondo.
        void ProcessBlockInteractions(Player& player, RaycastHit& target, Blocks::BlockType selectedBlockType){
            bool wantBreak = player.ConsumeBreakBlock();
            bool wantPlace = player.ConsumePlaceBlock();

            if(wantBreak && target.hit){
                BreakBlockWorld(target.blockX, target.blockY, target.blockZ);
                target.hit = false;
                return;
            }
            if(wantPlace && target.hit && target.hitFace >= 0){
                const auto& offset = Blocks::FACE_OFFSETS[target.hitFace];
                int placeX = target.blockX + offset[0];
                int placeY = target.blockY + offset[1];
                int placeZ = target.blockZ + offset[2];

                if(!player.IsPlayerOccupyingBlock(placeX, placeY, placeZ)){
                    PlaceBlockWorld(placeX, placeY, placeZ, selectedBlockType);
                }
            }
        }

        //Controlla se e' solido il blocco a coordinate MONDO, individuando da solo il chunk giusto.
        //Override di fcg::IWorld: e' cosi' che PlayerPhysics interroga il mondo senza conoscere World
        bool IsSolidAtWorld(int worldX, int worldY, int worldZ) override{
            if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
                return false;
            }

            int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
            int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

            Blocks::Chunk* chunk = GetChunkAt(chunkX, chunkZ);
            if(!chunk){
                return false;
            }

            int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
            int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

            return chunk->IsSolid(localX, worldY, localZ);
        }

    private:
        //Riempie un chunk con il terreno
        static void fillExistingChunks(Blocks::Chunk& chunk){
            const int SURFACE = Blocks::CHUNK_SIZE_Y * 0.5;
            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int y = 0; y < SURFACE; y++){
                    for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                        if(y == SURFACE - 1){
                            chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                        }
                        else if(y < SURFACE * 0.90) chunk.Set(x, y, z, Blocks::BlockType::STONE);
                        else chunk.Set(x, y, z, Blocks::BlockType::DIRT);

                        if(y == SURFACE - 1 && rand() % 100 >= 99){
                            chunk.Set(x, y + 1, z, Blocks::BlockType::WOOD);
                            chunk.Set(x, y + 2, z, Blocks::BlockType::LEAVES);
                        }
                    }
                }
            }
        }

        void GenerateAllChunks(){
            for(int chunkZ = 0; chunkZ < WORLDSIZECHUNKSZ; chunkZ++){
                for(int chunkX = 0; chunkX < WORLDSIZECHUNKSX; chunkX++){
                    chunks.emplace_back();
                    ChunkInstance& instance = chunks.back();
                    instance.chunkX = chunkX;
                    instance.chunkZ = chunkZ;
                    fillExistingChunks(instance.chunk);
                }
            }
        }

        void BuildAllMeshes(){
            for(ChunkInstance& instance : chunks){
                RebuildChunkMesh(instance);
            }
        }

        void RebuildNeighborIfExists(int chunkX, int chunkZ){
            ChunkInstance* neighbor = GetChunkInstanceAt(chunkX, chunkZ);
            if(neighbor){
                RebuildChunkMesh(*neighbor);
            }
        }

        //Cerca il chunk alle coordinate di griglia indicate, nullptr se non esiste
        Blocks::Chunk* GetChunkAt(int chunkX, int chunkZ){
            for(auto& instance : chunks){
                if(instance.chunkX == chunkX && instance.chunkZ == chunkZ){
                    return &instance.chunk;
                }
            }
            return nullptr;
        }

        //Cerca la ChunkInstance completa (chunk + mesh) alle coordinate di griglia indicate
        ChunkInstance* GetChunkInstanceAt(int chunkX, int chunkZ){
            for(auto& instance : chunks){
                if(instance.chunkX == chunkX && instance.chunkZ == chunkZ){
                    return &instance;
                }
            }
            return nullptr;
        }

        //Rigenera la mesh di una ChunkInstance leggendo lo stato attuale del chunk
        void RebuildChunkMesh(ChunkInstance& instance){
            auto isTransparentOutside = [this, &instance](int localX, int localY, int localZ){
                return IsTransparentWorld(instance.chunkX, instance.chunkZ, localX, localY, localZ);
            };
            auto meshData = Blocks::BuildChunkMesh(instance.chunk, isTransparentOutside);
            instance.mesh.Upload(meshData);
        }

        //Controlla se e' solido un blocco a coordinate locali (anche fuori dai bordi 0..15)
        //rispetto al chunk (chunkX, chunkZ), guardando nel chunk vicino se necessario
        bool IsTransparentWorld(int chunkX, int chunkZ, int localX, int localY, int localZ){
            //Y non e' suddiviso in chunk: se esce sopra/sotto e' semplicemente aria
            if(localY < 0 || localY >= Blocks::CHUNK_SIZE_Y){
                return false;
            }

            int neighborChunkX = chunkX + (localX < 0 ? -1 : (localX >= Blocks::CHUNK_SIZE_X ? 1 : 0));
            int neighborChunkZ = chunkZ + (localZ < 0 ? -1 : (localZ >= Blocks::CHUNK_SIZE_Z ? 1 : 0));

            Blocks::Chunk* neighbor = GetChunkAt(neighborChunkX, neighborChunkZ);
            if(!neighbor){
                return false; //Bordo del mondo: nessun chunk vicino, quindi aria
            }

            int wrappedX = ((localX % Blocks::CHUNK_SIZE_X) + Blocks::CHUNK_SIZE_X) % Blocks::CHUNK_SIZE_X;
            int wrappedZ = ((localZ % Blocks::CHUNK_SIZE_Z) + Blocks::CHUNK_SIZE_Z) % Blocks::CHUNK_SIZE_Z;

            return neighbor->IsTransparent(wrappedX, localY, wrappedZ);
        }

        //Divisione intera "verso il basso" (floor): serve perche' l'operatore % di C++
        //tronca verso zero, non verso -infinito, e con coordinate negative darebbe risultati sbagliati
        static int FloorDiv(int a, int b){
            int d = a / b;
            int r = a % b;
            return (r != 0 && ((r < 0) != (b < 0))) ? d - 1 : d;
        }

        //Distanza (in unita' di t lungo il raggio) dal punto 'originComp' al prossimo confine di voxel
        static float NextBoundary(float originComp, int voxelComp, float dirComp, int step){
            if(dirComp == 0.0f) return std::numeric_limits<float>::infinity();
            float boundary = step > 0 ? (float)(voxelComp + 1) : (float) voxelComp;
            return (boundary - originComp) / dirComp;
        }
    };
}

#endif
