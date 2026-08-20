#include "Blocks.hh"
#include "Matrices.hh"
#include "Hotshaders.hh"
#include <functional>

namespace Blocks
{
    constexpr int CHUNK_SIZE_X = 16;
    constexpr int CHUNK_SIZE_Y = 64;
    constexpr int CHUNK_SIZE_Z = 16;

    class Chunk{
        private:
            std::vector<BlockType> blocks;

            inline int Index(int x, int y, int z) const
            {
                return x + CHUNK_SIZE_X * (y + CHUNK_SIZE_Y * z);
            }

        public:
            Chunk() : blocks(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, BlockType::AIR) {}

            bool InBounds(int x, int y, int z) const
            {
                return x >= 0 && x < CHUNK_SIZE_X &&
                    y >= 0 && y < CHUNK_SIZE_Y &&
                    z >= 0 && z < CHUNK_SIZE_Z;
            }

            BlockType Get(int x, int y, int z) const
            {
                if(!InBounds(x, y, z))
                    return BlockType::AIR; // fuori dal chunk = aria (bordo visibile)
                return blocks[Index(x, y, z)];
            }

            void Set(int x, int y, int z, BlockType type)
            {
                if(InBounds(x, y, z))
                    blocks[Index(x, y, z)] = type;
            }

            bool IsSolid(int x, int y, int z) const
            {
                return Get(x, y, z) != BlockType::AIR;
            }
    };

    struct Vertex
    {
        float x, y, z;
        float u, v;
        float texture;
    };

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    constexpr int FACE_OFFSETS[6][3] = {
        { 0, 0, 1}, { 0, 0,-1}, {-1, 0, 0}, { 1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
    };

    constexpr float FACE_VERTS[6][4][3] = {
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
        {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
        {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
        {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}
    };

    constexpr float FACE_UV[4][2] = { {0,0}, {1,0}, {1,1}, {0,1} };

    using NeighborSolidFn = std::function<bool(int, int, int)>;

    inline MeshData BuildChunkMesh(const Chunk& chunk, const NeighborSolidFn& isSolidOutside){
        MeshData mesh;
        for(int z = 0; z < CHUNK_SIZE_Z; ++z){
            for(int y = 0; y < CHUNK_SIZE_Y; ++y){
                for(int x = 0; x < CHUNK_SIZE_X; ++x){
                    BlockType type = chunk.Get(x, y, z);
                    if(type == BlockType::AIR){
                        continue;
                    }

                    for(int i = 0; i < 6; i++){
                        BlockFace face = static_cast<BlockFace>(i);
                        int nx = x + FACE_OFFSETS[i][0];
                        int ny = y + FACE_OFFSETS[i][1];
                        int nz = z + FACE_OFFSETS[i][2];

                        //Se il vicino e' dentro lo stesso chunk usiamo il controllo normale,
                        //altrimenti chiediamo alla funzione esterna di controllare nel chunk adiacente
                        bool neighborSolid = chunk.InBounds(nx, ny, nz) ? chunk.IsSolid(nx, ny, nz) : isSolidOutside(nx, ny, nz);

                        if(neighborSolid){
                            continue;
                        }
                        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
                        float layer = static_cast<float>(getTextureIndex(type, face));

                        for(int v = 0; v < 4; ++v){
                            mesh.vertices.push_back({
                                x + FACE_VERTS[i][v][0], y + FACE_VERTS[i][v][1], z + FACE_VERTS[i][v][2],
                                FACE_UV[v][0], FACE_UV[v][1], layer
                            });
                        }
                        mesh.indices.insert(mesh.indices.end(),{ base+0, base+1, base+2, base+2, base+3, base+0 });
                    }
                }
            }
        }
        return mesh;
    }

    class ChunkMesh
    {
    private:
        GLuint vao = 0, vbo = 0, ebo = 0;
        GLsizei indexCount = 0;

    public:
        ~ChunkMesh() { Clean(); }

        void Upload(const MeshData& mesh)
        {
            Clean();
            indexCount = static_cast<GLsizei>(mesh.indices.size());
            if(indexCount == 0) return;

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

            glBindVertexArray(0);
        }

        void Draw() const
        {
            if(indexCount == 0) return;
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        void Clean()
        {
            if(vao){ glDeleteVertexArrays(1, &vao); vao = 0; }
            if(vbo){ glDeleteBuffers(1, &vbo); vbo = 0; }
            if(ebo){ glDeleteBuffers(1, &ebo); ebo = 0; }
            indexCount = 0;
        }
    };
}