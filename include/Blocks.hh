#ifndef BLOCK_HH
#define BLOCK_HH


#include <SFML/Graphics/Image.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cstdint>

namespace Blocks
{
    enum class BlockType : uint8_t
    {
        Air = 0,
        Dirt,
        Grass,
        Stone,
        Wood
    };

    enum class Face : uint8_t
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
        BlockType type = BlockType::Air;

        bool isSolid() const {
            return type != BlockType::Air;
        }
    };

    // Mappa (TipoBlocco, Faccia) -> Indice del Layer nella Texture Array
    inline uint32_t getTextureIndex(BlockType type, Face face)
    {
        switch (type)
        {
        case BlockType::Grass:
            if (face == Face::Top)    return 1; // Layer 1: Erba (Sopra)
            if (face == Face::Bottom) return 2; // Layer 2: Terra
            return 3;                           // Layer 3: Lato Erba
        
        case BlockType::Dirt:
            return 2; //Texture tutta uguale

        case BlockType::Stone:
            return 4; //Tutta uguale

        case BlockType::Wood:
            if(face == Face::Top || face == Face::Bottom) return 6; //Anelli del Tronco
            return 7;                                               //Corteccia (lati)

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
        void LoadTextures(const std::vector<std::string>& filepaths, int width = 16, int height = 16)
        {
            GLsizei layerCount = static_cast<GLsizei>(filepaths.size());

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

            // Alloca lo spazio GPU per 'layerCount' immagini 2D
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Carica ogni singola immagine e la copia nel rispettivo layer z
            for (GLsizei i = 0; i < layerCount; ++i)
            {
                sf::Image img;
                if (!img.loadFromFile(filepaths[i])) {
                    std::cerr << "Errore nel caricamento della texture: " << filepaths[i] << std::endl;
                    std::cerr << "Eseguo fallback!\n";
                    if(!img.loadFromFile(filepaths[0])) continue;
                }

                // Copia i pixel dell'immagine nel layer 'i'
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
            }

            // Impostazioni di filtraggio stile pixel-art (Minecraft)
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

#endif // BLOCK_HH