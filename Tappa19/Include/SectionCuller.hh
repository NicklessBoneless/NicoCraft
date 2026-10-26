#ifndef SECTION_CULLER_HH
#define SECTION_CULLER_HH

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Camera.hh"
#include "Chunk.hh"
#include "Frustum.hh"
#include "World.hh"

namespace fcg{
    /*
        Occlusion culling a grafo di visibilita' (Tommaso Checchi, "cave culling").
        Ogni sezione 16x16x16 e' un nodo con una maschera di connettivita' tra le sue 6 facce.
        Si parte dalla sezione della camera e si propaga ai vicini con una BFS:
        - da una sezione si esce da una faccia solo se l'aria la collega a quella d'ingresso
        - non si torna mai indietro rispetto a una direzione gia' percorsa
        - il vicino deve stare nel frustum e dentro la distanza di rendering
        Le sezioni non raggiunte (caverne sotto il suolo, dietro colline) non vengono disegnate
    */
    class SectionCuller{
    private:
        static constexpr int sectionsPerChunk = Blocks::SECTIONS_PER_CHUNK;
        static constexpr int totalSections = WORLDSIZECHUNKSX * WORLDSIZECHUNKSZ * sectionsPerChunk;
        static constexpr uint8_t noFace = 6; //Ingresso "nessuno": e' la sezione della camera
        static constexpr int alwaysVisibleRadius = 0; //Sezioni attorno alla camera sempre disegnate

        struct QueueEntry{
            int node;
            uint8_t entryFace;  //Faccia della sezione da cui siamo entrati
            uint8_t directions; //Bitmask delle direzioni (BlockFace) percorse finora
        };

        std::vector<uint8_t> visited = std::vector<uint8_t>(totalSections, 0);
        std::vector<QueueEntry> queue;

    public:
        SectionCuller(){
            queue.reserve(totalSections);
        }

        //Riempie visibleNodes con gli indici dei nodi raggiunti dalla BFS
        void Compute(const World& world, const Camera& camera, const Frustum& frustum, bool useFrustum, float renderDistance, std::vector<int>& visibleNodes){
            visibleNodes.clear();
            const std::vector<ChunkInstance>& chunks = world.GetChunks();
            glm::vec3 cameraPos = camera.getPosition();

            int startChunkX = (int) std::floor(cameraPos.x / Blocks::CHUNK_SIZE_X);
            int startChunkZ = (int) std::floor(cameraPos.z / Blocks::CHUNK_SIZE_Z);
            int startSectionY = std::clamp((int) std::floor(cameraPos.y / Blocks::SECTION_SIZE), 0, sectionsPerChunk - 1);

            //Camera fuori dal mondo (noclip): la BFS non ha un punto di partenza valido
            if(startChunkX < 0 || startChunkX >= WORLDSIZECHUNKSX || startChunkZ < 0 || startChunkZ >= WORLDSIZECHUNKSZ){
                CollectAll(camera, frustum, useFrustum, renderDistance, visibleNodes);
                return;
            }

            std::fill(visited.begin(), visited.end(), 0);
            queue.clear();

            int startNode = NodeIndex(startChunkX, startSectionY, startChunkZ);
            visited[startNode] = 1;
            queue.push_back({startNode, noFace, 0});
            visibleNodes.push_back(startNode);

            for(size_t head = 0; head < queue.size(); head++){
                QueueEntry current = queue[head];

                int sectionY = current.node % sectionsPerChunk;
                int chunkIndex = current.node / sectionsPerChunk;
                int chunkX = chunkIndex % WORLDSIZECHUNKSX;
                int chunkZ = chunkIndex / WORLDSIZECHUNKSX;
                uint16_t connectivity = chunks[chunkIndex].connectivity[sectionY];

                for(int face = 0; face < 6; face++){
                    int opposite = face ^ 1; //Le facce sono a coppie: (0,1) (2,3) (4,5)

                    //Non si torna indietro rispetto a una direzione gia' percorsa
                    if(current.directions & (1 << opposite)) continue;

                    //Dalla faccia d'ingresso si esce solo se l'aria collega le due facce
                    if(current.entryFace != noFace && !Blocks::AreFacesConnected(connectivity, current.entryFace, face)) continue;

                    int neighborChunkX = chunkX + Blocks::FACE_OFFSETS[face][0];
                    int neighborSectionY = sectionY + Blocks::FACE_OFFSETS[face][1];
                    int neighborChunkZ = chunkZ + Blocks::FACE_OFFSETS[face][2];
                    if(!InWorld(neighborChunkX, neighborSectionY, neighborChunkZ)) continue;

                    int neighbor = NodeIndex(neighborChunkX, neighborSectionY, neighborChunkZ);
                    if(visited[neighbor]) continue;
                    if(!IsSectionVisible(neighborChunkX, neighborSectionY, neighborChunkZ, cameraPos, frustum, useFrustum, renderDistance)) continue;

                    visited[neighbor] = 1;
                    queue.push_back({neighbor, (uint8_t) opposite, (uint8_t) (current.directions | (1 << face))});
                    visibleNodes.push_back(neighbor);
                }
            }

            //Rete di sicurezza: le sezioni attorno alla camera si disegnano sempre
            for(int dx = -alwaysVisibleRadius; dx <= alwaysVisibleRadius; dx++){
                for(int dy = -alwaysVisibleRadius; dy <= alwaysVisibleRadius; dy++){
                    for(int dz = -alwaysVisibleRadius; dz <= alwaysVisibleRadius; dz++){
                        int x = startChunkX + dx;
                        int y = startSectionY + dy;
                        int z = startChunkZ + dz;
                        if(!InWorld(x, y, z)) continue;

                        int node = NodeIndex(x, y, z);
                        if(visited[node]) continue;
                        visited[node] = 1;
                        visibleNodes.push_back(node);
                    }
                }
            }
        }

        //Senza occlusione: tutte le sezioni, filtrate solo da frustum e distanza (per il confronto con F5)
        void CollectAll(const Camera& camera, const Frustum& frustum, bool useFrustum, float renderDistance, std::vector<int>& visibleNodes){
            visibleNodes.clear();
            glm::vec3 cameraPos = camera.getPosition();

            for(int chunkZ = 0; chunkZ < WORLDSIZECHUNKSZ; chunkZ++){
                for(int chunkX = 0; chunkX < WORLDSIZECHUNKSX; chunkX++){
                    for(int sectionY = 0; sectionY < sectionsPerChunk; sectionY++){
                        if(IsSectionVisible(chunkX, sectionY, chunkZ, cameraPos, frustum, useFrustum, renderDistance)){
                            visibleNodes.push_back(NodeIndex(chunkX, sectionY, chunkZ));
                        }
                    }
                }
            }
        }

    private:
        static int NodeIndex(int chunkX, int sectionY, int chunkZ){
            return (chunkZ * WORLDSIZECHUNKSX + chunkX) * sectionsPerChunk + sectionY;
        }

        static bool InWorld(int chunkX, int sectionY, int chunkZ){
            return chunkX >= 0 && chunkX < WORLDSIZECHUNKSX
                && sectionY >= 0 && sectionY < sectionsPerChunk
                && chunkZ >= 0 && chunkZ < WORLDSIZECHUNKSZ;
        }

        static bool IsSectionVisible(int chunkX, int sectionY, int chunkZ, const glm::vec3& cameraPos, const Frustum& frustum, bool useFrustum, float renderDistance){
            glm::vec3 minCorner((float) (chunkX * Blocks::CHUNK_SIZE_X), (float) (sectionY * Blocks::SECTION_SIZE), (float) (chunkZ * Blocks::CHUNK_SIZE_Z));
            glm::vec3 maxCorner = minCorner + glm::vec3((float) Blocks::CHUNK_SIZE_X, (float) Blocks::SECTION_SIZE, (float) Blocks::CHUNK_SIZE_Z);

            //Distanza dal punto piu' vicino del box alla camera
            glm::vec3 closest = glm::clamp(cameraPos, minCorner, maxCorner);
            glm::vec3 difference = closest - cameraPos;
            if(glm::dot(difference, difference) > renderDistance * renderDistance) return false;

            return !useFrustum || frustum.IsBoxVisible(minCorner, maxCorner);
        }
    };
}

#endif