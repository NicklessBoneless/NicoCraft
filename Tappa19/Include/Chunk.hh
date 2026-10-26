#ifndef CHUNK_HH
#define CHUNK_HH

#include "Blocks.hh"
#include "Matrices.hh"
#include "Hotshaders.hh"
#include <functional>
#include <algorithm>
#include <cstdint>
#include <array>

namespace Blocks
{
    constexpr int CHUNK_SIZE_X = 16;
    constexpr int CHUNK_SIZE_Y = 256;
    constexpr int CHUNK_SIZE_Z = 16;
    constexpr int SECTION_SIZE = 16;
    constexpr int SECTIONS_PER_CHUNK = CHUNK_SIZE_Y / SECTION_SIZE;

    class Chunk{
        private:
            std::vector<BlockType> blocks;

            inline int Index(int x, int y, int z) const{
                return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
            }

        public:
            Chunk() : blocks(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, BlockType::AIR){}

            bool InBounds(int x, int y, int z) const{
                return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z;
            }

            BlockType Get(int x, int y, int z) const{
                if(!InBounds(x, y, z))
                    return BlockType::AIR; //Se fuori dal chunk == aria (allora bordo visibile)
                return blocks[Index(x, y, z)];
            }

            void Set(int x, int y, int z, BlockType type){
                if(InBounds(x, y, z))
                    blocks[Index(x, y, z)] = type;
            }

            bool IsSolid(int x, int y, int z) const{
                return Get(x, y, z) != BlockType::AIR;
            }

            bool IsTransparent(int x, int y, int z) const{
                return isBlockTransparent(Get(x, y, z));
            }
    };

    struct Vertex{
        float x, y, z;
        float u, v;
        float texture;
        float brightness;
    };

    struct MeshData{
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    constexpr int FACE_OFFSETS[6][3] = {
        { 0, 0, 1}, { 0, 0,-1}, {-1, 0, 0}, { 1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
    };

    constexpr float FACE_VERTICES[6][4][3] = {
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
        {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}
    };

    constexpr float FACE_UV[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };
    //Front, Back, Left, Right, Top, Bottom
    constexpr float FACE_BRIGHTNESS[6] = { 0.75f, 0.75f, 0.75f, 0.75f, 1.f, 0.55f };

    using NeighborSolidFn = std::function<bool(int, int, int)>;

    inline MeshData BuildChunkMesh(const Chunk& chunk, const NeighborSolidFn& isNeighborTransparent, int sectionHeightY){
        MeshData mesh;
        int startingHeightY = sectionHeightY * SECTION_SIZE;
        int RoofHeightY = startingHeightY + SECTION_SIZE;
        for(int z = 0; z < CHUNK_SIZE_Z ; ++z){
            for(int y = startingHeightY ; y < RoofHeightY; ++y){
                for(int x = 0 ; x < CHUNK_SIZE_X; ++x){
                    BlockType type = chunk.Get(x, y, z);
                    if(type == BlockType::AIR){
                        continue;
                    }

                    for(int i = 0; i < 6; i++){
                        BlockFace face = static_cast<BlockFace>(i);
                        int nx = x + FACE_OFFSETS[i][0];
                        int ny = y + FACE_OFFSETS[i][1];
                        int nz = z + FACE_OFFSETS[i][2];

                        bool IsNeighborTransparent = false;
                        if(chunk.InBounds(nx, ny, nz)) //Blocco dentro i confini del chunk?
                            IsNeighborTransparent = chunk.IsTransparent(nx, ny, nz);
                        else //Out of bounds, blocco fuori dai confini del chunk?
                            IsNeighborTransparent = isNeighborTransparent(nx, ny, nz);

                        if(!IsNeighborTransparent){
                            continue;
                        }
                        
                        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
                        float texture = static_cast<float>(getTextureIndex(type, face));
                        float brightness = FACE_BRIGHTNESS[i];

                        for(int v = 0; v < 4; ++v){
                            mesh.vertices.push_back({
                                x + FACE_VERTICES[i][v][0], 
                                y + FACE_VERTICES[i][v][1], 
                                z + FACE_VERTICES[i][v][2],
                                FACE_UV[v][0], //
                                FACE_UV[v][1], 
                                texture,
                                brightness
                            });
                        }
                        mesh.indices.insert(mesh.indices.end(),{ base+0, base+1, base+2, base+2, base+3, base+0 });
                    }
                }
            }
        }
        return mesh;
    }

    //Indice (0..14) della coppia non ordinata di facce (a, b), con a != b
    constexpr int FacePairIndex(int a, int b){
        if(a > b){
            int temp = a;
            a = b;
            b = temp;
        }
        return a * (11 - a) / 2 + (b - a - 1);
    }

    //True se, dentro la sezione, l'aria collega la faccia a con la faccia b
    constexpr bool AreFacesConnected(uint16_t connectivity, int a, int b){
        if(a == b) return false;
        return ((connectivity >> FacePairIndex(a, b)) & 1) != 0;
    }

    /*
        Calcola la maschera di connettivita' (15 bit) di una sezione 16x16x16.
        Flood fill sulle celle attraversabili (aria, foglie, vetro): per ogni regione connessa
        si guardano le facce della sezione che tocca, e tutte le coppie di quelle facce
        risultano collegate. Facce = BlockFace: Front=0, Back=1, Left=2, Right=3, Top=4, Bottom=5
    */
    inline uint16_t ComputeSectionConnectivity(const Chunk& chunk, int sectionY){
        constexpr int cellCount = CHUNK_SIZE_X * SECTION_SIZE * CHUNK_SIZE_Z;
        const int startY = sectionY * SECTION_SIZE;

        std::array<bool, cellCount> visited{};
        std::array<int, cellCount> stack;
        uint16_t connectivity = 0;

        auto isPassable = [&](int cell){
            int x = cell % CHUNK_SIZE_X;
            int localY = (cell / CHUNK_SIZE_X) % SECTION_SIZE;
            int z = cell / (CHUNK_SIZE_X * SECTION_SIZE);
            return chunk.IsTransparent(x, startY + localY, z);
        };

        for(int start = 0; start < cellCount; start++){
            if(visited[start]) continue;
            visited[start] = true;
            if(!isPassable(start)) continue;

            uint8_t touchedFaces = 0;
            int top = 0;
            stack[top++] = start;

            while(top > 0){
                int cell = stack[--top];
                int x = cell % CHUNK_SIZE_X;
                int localY = (cell / CHUNK_SIZE_X) % SECTION_SIZE;
                int z = cell / (CHUNK_SIZE_X * SECTION_SIZE);

                if(z == CHUNK_SIZE_Z - 1) touchedFaces |= 1 << (int) BlockFace::Front;
                if(z == 0)                touchedFaces |= 1 << (int) BlockFace::Back;
                if(x == 0)                touchedFaces |= 1 << (int) BlockFace::Left;
                if(x == CHUNK_SIZE_X - 1) touchedFaces |= 1 << (int) BlockFace::Right;
                if(localY == SECTION_SIZE - 1) touchedFaces |= 1 << (int) BlockFace::Top;
                if(localY == 0)                touchedFaces |= 1 << (int) BlockFace::Bottom;

                for(int face = 0; face < 6; face++){
                    int nx = x + FACE_OFFSETS[face][0];
                    int ny = localY + FACE_OFFSETS[face][1];
                    int nz = z + FACE_OFFSETS[face][2];
                    if(nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny >= SECTION_SIZE || nz < 0 || nz >= CHUNK_SIZE_Z) continue;

                    int neighbor = nx + CHUNK_SIZE_X * (ny + SECTION_SIZE * nz);
                    if(visited[neighbor]) continue;
                    visited[neighbor] = true; //Segnata anche se solida: non la ricontrollo
                    if(isPassable(neighbor)) stack[top++] = neighbor;
                }
            }

            for(int a = 0; a < 6; a++){
                if(!(touchedFaces & (1 << a))) continue;
                for(int b = a + 1; b < 6; b++){
                    if(touchedFaces & (1 << b)) connectivity |= (uint16_t) (1 << FacePairIndex(a, b));
                }
            }

            if(connectivity == 0x7FFF) return connectivity; //Gia' tutte collegate
        }
        return connectivity;
    }    

    class ChunkMesh
    {
    private:
        GLuint vao = 0, vbo = 0, ebo = 0;
        GLsizei indexCount = 0;
        GLsizei vertexCount = 0;
        float minY = 0.0f,maxY = 0.0f;

    public:
        ~ChunkMesh() { Clean(); }

        void Upload(const MeshData& mesh){
            Clean();
            indexCount = static_cast<GLsizei>(mesh.indices.size());
            if(indexCount == 0) return;

            vertexCount = static_cast<GLsizei>(mesh.vertices.size());
            minY = mesh.vertices[0].y;
            maxY = minY;
            for(const Vertex& vertex : mesh.vertices){
                minY = std::min(minY, vertex.y);
                maxY = std::max(maxY, vertex.y);
            }

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, u));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texture));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, brightness));
            glEnableVertexAttribArray(3);

            glBindVertexArray(0);
        }

        void Draw() const{
            if(indexCount == 0) return;
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        bool IsEmpty() const{
            return indexCount == 0;
        }

        GLsizei GetVertexCount() const{
            return vertexCount;
        }

        GLsizei GetTriangleCount() const{
            return indexCount / 3;
        }

        float GetMinY() const{
            return minY;
        }

        float GetMaxY() const{
            return maxY;
        }

        void Clean(){
            if(vao){ glDeleteVertexArrays(1, &vao); vao = 0; }
            if(vbo){ glDeleteBuffers(1, &vbo); vbo = 0; }
            if(ebo){ glDeleteBuffers(1, &ebo); ebo = 0; }
            indexCount = 0;
            vertexCount = 0;
            minY = 0.0f;
            maxY = 0.0f;
        }
    };
}

#endif
