#ifndef BLOCK_OUTLINE_HH
#define BLOCK_OUTLINE_HH

#include "Hotshaders.hh"
#include "Matrices.hh"

namespace fcg{
    //Disegna un bordo bianco sui lati del blocco puntato dal giocatore
    class BlockOutline{
    private:
        GLuint vao = 0, vbo = 0, ebo = 0;
        GLint pvmLoc = -1;
        Shaders shader;
        
        //Margine per far "galleggiare" leggermente il contorno fuori dal blocco
        //ed evitare z-fighting con le facce della mesh
        static constexpr float offsetLine = 0.001f;
        static constexpr float lineThickness = 4.0f;
        static constexpr float polygonOffset = -10.0f;

        void Cleanup(){
            if(vao){ 
                glDeleteVertexArrays(1, &vao); 
                vao = 0;
            }
            if(vbo){ 
                glDeleteBuffers(1, &vbo); 
                vbo = 0;
            }
            if(ebo){ 
                glDeleteBuffers(1, &ebo); 
                ebo = 0;
            }
        }

    public:
        BlockOutline(const std::string vertexFile, const std::string fragmentFile) : shader(vertexFile, fragmentFile){
            pvmLoc = glGetUniformLocation(shader.program, "pvm");
            BuildOutline();
        }

        ~BlockOutline(){
            Cleanup();
        }

        void BuildOutline(){
            const float minBound = -offsetLine; // -0.001f (limite inferiore su X, Y o Z)
            const float maxBound = 1.0f + offsetLine; //  1.001f (limite superiore su X, Y o Z)

            const float vertices[] = {
                //  X         Y         Z
                minBound, minBound, minBound, // Vertice 0
                maxBound, minBound, minBound, // Vertice 1
                maxBound, maxBound, minBound, // Vertice 2
                minBound, maxBound, minBound, // Vertice 3

                minBound, minBound, maxBound, // Vertice 4
                maxBound, minBound, maxBound, // Vertice 5
                maxBound, maxBound, maxBound, // Vertice 6
                minBound, maxBound, maxBound  // Vertice 7
            };

            //12 spigoli del cubo, come coppie di indici per GL_LINES
            uint32_t indices[] = {
                0,1, 1,2, 2,3, 3,0, //Spigoli faccia dietro
                4,5, 5,6, 6,7, 7,4, //Spigoli faccia davanti
                0,4, 1,5, 2,6, 3,7  //Spigoli che collegano le due facce
            };

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }

        //Disegna il contorno posizionato sul blocco (blockX, blockY, blockZ), coordinate mondo
        void Draw(int blockX, int blockY, int blockZ, const glm::mat4& view, const glm::mat4& projection){
            glm::mat4 model = fcg::translation((float) blockX, (float) blockY, (float) blockZ); //Traslazione da 0,0,0
            glm::mat4 vm = view * model; //Rotazione e spostamento attorno alla videocamera 
            glm::mat4 pvm = projection * vm; //Proiezione dello spazio 3D in 2D
           
            shader.use();

            //Passiamo al vertex shader (block_outline.vert) la matrice di trasformazione
            //UN unica matrice, così da far effettuare meno calcoli alla GPU
            glUniformMatrix4fv(pvmLoc,1,GL_FALSE,&pvm[0][0]);

            glLineWidth(lineThickness); //Su alcuni driver (es. macOS core profile) resta 1.0, non e' bloccante
            glDepthFunc(GL_LEQUAL);

            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(polygonOffset,polygonOffset);

            glBindVertexArray(vao);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDisable(GL_POLYGON_OFFSET_LINE);
            glDepthFunc(GL_LESS);
        }
    };
}

#endif
