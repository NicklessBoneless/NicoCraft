#ifndef BLOCK_HH
#define BLOCK_HH

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <SFML/Graphics/Image.hpp>
#include <glm/vec3.hpp>

namespace Blocks{
    enum class BlockType : uint8_t
    {
        AIR = 0,
        DIRT,
        GRASS,
        STONE,
        PLANK,
        LOGWOOD,
        LEAVES,
        GLASS,
        LOGWOODNORTHZ,
        LOGWOODEASTX
    };

    enum class BlockFace : uint8_t
    {
        Front = 0, // +Z
        Back,      // -Z
        Left,      // -X
        Right,     // +X
        Top,       // +Y
        Bottom     // -Y
    };

    // Identificatori chiari per ogni layer nell'array di texture
    enum class TextureIndex : uint32_t
    {
        MISSING = 0,
        DIRT,
        GRASS_TOP,
        GRASS_SIDE,
        STONE,
        PLANK,
        LOG_TOP,
        LOG_SIDE,
        LOG_SIDEFLIP,
        LOG_SIDELEFTROT,
        LOG_SIDERIGHTROT,
        LEAVES,
        GLASS,
    };

    struct Block
    {
        BlockType type = BlockType::AIR;

        bool isSolid() const {
            return type != BlockType::AIR;
        }
    };

    inline bool isBlockTransparent(BlockType type){
        switch(type){
            case BlockType::AIR:
            case BlockType::LEAVES:
            case BlockType::GLASS:
                return true;
            default:
                return false;
        }
    }

    // Mappa (TipoBlocco, Faccia) -> Indice della texture nel Texture Array
    inline uint32_t getTextureIndex(BlockType type, BlockFace face)
    {
        switch(type) {
        case BlockType::GRASS:
            if(face == BlockFace::Bottom) return static_cast<uint32_t>(TextureIndex::DIRT);
            if(face == BlockFace::Top)    return static_cast<uint32_t>(TextureIndex::GRASS_TOP);
            return static_cast<uint32_t>(TextureIndex::GRASS_SIDE);
        
        case BlockType::DIRT:
            return static_cast<uint32_t>(TextureIndex::DIRT);

        case BlockType::STONE:
            return static_cast<uint32_t>(TextureIndex::STONE);

        case BlockType::PLANK:
            return static_cast<uint32_t>(TextureIndex::PLANK);
    
        case BlockType::LOGWOOD:
            if(face == BlockFace::Top || face == BlockFace::Bottom) 
                return static_cast<uint32_t>(TextureIndex::LOG_TOP);
            return static_cast<uint32_t>(TextureIndex::LOG_SIDE);

        case BlockType::LOGWOODEASTX:
            if(face == BlockFace::Right || face == BlockFace::Left)
                return static_cast<uint32_t>(TextureIndex::LOG_TOP);
            if(face == BlockFace::Top || face == BlockFace::Bottom || face == BlockFace::Front)
                return static_cast<uint32_t>(TextureIndex::LOG_SIDERIGHTROT); 
            return static_cast<uint32_t>(TextureIndex::LOG_SIDELEFTROT);

        case BlockType::LOGWOODNORTHZ:
            if(face == BlockFace::Front || face == BlockFace::Back)
                return static_cast<uint32_t>(TextureIndex::LOG_TOP);
            if(face == BlockFace::Top)
                return static_cast<uint32_t>(TextureIndex::LOG_SIDE); 
            if(face == BlockFace::Bottom)
                return static_cast<uint32_t>(TextureIndex::LOG_SIDEFLIP); 
            if(face == BlockFace::Right)
                 return static_cast<uint32_t>(TextureIndex::LOG_SIDERIGHTROT); //Left
            return static_cast<uint32_t>(TextureIndex::LOG_SIDELEFTROT);
            
        case BlockType::LEAVES:
            return static_cast<uint32_t>(TextureIndex::LEAVES);

        case BlockType::GLASS:
            return static_cast<uint32_t>(TextureIndex::GLASS);
            
        default:
            return static_cast<uint32_t>(TextureIndex::MISSING); 
        }
    }

    // Gestione delle Texture Array OpenGL [GL_TEXTURE_2D_ARRAY]
    class TextureArray {
    private:
        GLuint textureID = 0;

        // L'ordine in questo vector corrisponde agli indici definiti in TextureIndex
        const std::vector<std::string> fileNames = {
            "missingTextureBlock.png", // Index 0 -> MISSING
            "dirt.png",                // Index 1 -> DIRT
            "grassTop.png",            // Index 2 -> GRASS_TOP
            "grassSide.png",           // Index 3 -> GRASS_SIDE
            "stone.png",               // Index 4 -> STONE
            "woodplank.png",           // Index 5 -> PLANK
            "logTop.png",              // Index 6 -> LOG_TOP
            "logSide.png",             // Index 7 -> LOG_SIDE
            "logSideFlip.png",         // Index 8 -> LOG_SIDEFLIP
            "logSideLeftRot.png",      // Index 9 -> LOG_ROT
            "logSideRightRot.png",     // Index 10 -> LOG_ROT
            "leaves.png",              // Index 11 -> LEAVES
            "glass.png"                // Index 12 -> GLASS
        };

        const int textureSize = 32;

    public:
        TextureArray() = default;
        ~TextureArray() { Clean(); }

        // Carica una lista di percorsi immagini e le impila nella Texture Array
        void LoadTextures(const std::string& res) {
            GLsizei numberOfTextures = static_cast<GLsizei>(fileNames.size());

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

            // Alloca lo spazio GPU per 'textureCount' immagini 2D
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, textureSize, textureSize, numberOfTextures, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Carica ogni singola immagine e la copia nel rispettivo layer z
            for(GLsizei textureIndex = 0; textureIndex < numberOfTextures; textureIndex++) {
                sf::Image img;
                if(!img.loadFromFile(res + fileNames[textureIndex])) {
                    std::cerr << "Errore nel caricamento della texture: " << res + fileNames[textureIndex] << std::endl;
                    std::cerr << "Eseguo fallback!\n";
                    if(!img.loadFromFile(res + fileNames[0])) {
                        std::cerr << "Fallback fallito :-( !\n";
                        continue;
                    }
                }

                // Necessario per non avere le texture capovolte
                img.flipVertically();

                //Copia la texture nella posizione textureIndex dell'array di OpenGL
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, textureIndex, textureSize, textureSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());

                GLenum err = glGetError();
                if(err != GL_NO_ERROR) {
                    std::cerr << "GL error dopo texture " << textureIndex << ": " << err << std::endl;
                }
            }

            // Filtraggio stile pixel-art (Minecraft)
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }

        void Bind(GLuint unit = 0) const
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
        }

        void Clean()
        {
            if(textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
        }
    };
}

#endif