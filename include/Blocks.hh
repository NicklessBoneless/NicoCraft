#ifndef BLOCK_HH
#define BLOCK_HH

#include <SFML/Graphics/Image.hpp>
#include <glm/vec3.hpp>
#include "basicLib.hh"

namespace Blocks{
    enum class BlockType : uint8_t
    {
        AIR = 0,
        DIRT,
        GRASS,
        STONE,
        WOOD,
        LEAVES
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

    struct Block
    {
        BlockType type = BlockType::AIR;

        bool isSolid() const{
            return type != BlockType::AIR;
        }
    };

    // Mappa (TipoBlocco, Faccia) -> Indice della texture nel Texture Array
    inline uint32_t getTextureIndex(BlockType type, BlockFace face)
    {
        switch(type)
        {
        case BlockType::GRASS:
            if(face == BlockFace::Top)    return 1; // Index 1: Erba (Sopra)
            if(face == BlockFace::Bottom) return 2; // Index 2: Terra
            return 3;                           // Index 3: Lato Erba
        
        case BlockType::DIRT:
            return 2; //Texture tutta uguale

        case BlockType::STONE:
            return 4; //Tutta uguale

        case BlockType::WOOD:
            if(face == BlockFace::Top || face == BlockFace::Bottom) return 5; //Anelli del Tronco
            return 6;                                               //Corteccia (lati)

        case BlockType::LEAVES:
            return 7;
            
        default:
            return 0; 
        }
    }

    //Gestione delle Texture Array OpenGL [GL_TEXTURE_2D_ARRAY]
    class TextureArray{
    private:
        GLuint textureID = 0;

    public:
        TextureArray() = default;
        ~TextureArray() { Clean(); }

        //Carica una lista di percorsi immagini e le impila nella Texture Array
        void LoadTextures(const std::vector<std::string>& filepaths, int width = 32, int height = 32){
            GLsizei textureCount = static_cast<GLsizei>(filepaths.size());

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

            // Alloca lo spazio GPU per 'layerCount' immagini 2D
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, textureCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Carica ogni singola immagine e la copia nel rispettivo layer z
            for(GLsizei i = 0; i < textureCount; i++){
                sf::Image img;
                if(!img.loadFromFile(filepaths[i])){
                    std::cerr << "Errore nel caricamento della texture: " << filepaths[i] << std::endl;
                    std::cerr << "Eseguo fallback!\n";
                    if(!img.loadFromFile(filepaths[0])){
                      std::cerr << "Fallback fallito :-( !\n";
                      continue;
                    }
                }

                //Necessario per non prendere le texture rovesciate
                img.flipVertically();

                // Copia i pixel dell'immagine nel layer 'i'
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
            }

            //Filtraggio stile pixel-art (Minecraft)
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
            if(textureID != 0){
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
        }
    };
}

#endif