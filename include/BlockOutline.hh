#ifndef BLOCK_OUTLINE_HH
#define BLOCK_OUTLINE_HH

#include "hotshaders.hh"
#include "matrices.hh"

namespace fcg{
    //Disegna un contorno (wireframe) attorno a un blocco 1x1x1, per evidenziare
    //il blocco attualmente puntato dal giocatore
    class BlockOutline{
    private:
        GLuint vao = 0, vbo = 0, ebo = 0;
        Shaders shader;
        GLint modelLoc, viewLoc, projLoc;

        //Margine per far "galleggiare" leggermente il contorno fuori dal blocco
        //ed evitare z-fighting con le facce della mesh
        static constexpr float offsetLine = 0.001f;
        static constexpr float lineThickness = 3.0f;

    public:
        BlockOutline(const std::string vertexFile, const std::string fragmentFile) : shader(vertexFile, fragmentFile){
            modelLoc = glGetUniformLocation(shader.program, "model");
            viewLoc = glGetUniformLocation(shader.program, "view");
            projLoc = glGetUniformLocation(shader.program, "projection");
            Build();
        }

        ~BlockOutline(){
            if(vao){ glDeleteVertexArrays(1, &vao); }
            if(vbo){ glDeleteBuffers(1, &vbo); }
            if(ebo){ glDeleteBuffers(1, &ebo); }
        }

        void Build(){
            float m = -offsetLine;
            float M = 1.0f + offsetLine;

            //8 vertici del cubo (leggermente ingrandito di 'margin')
            float vertices[] = {
                m, m, m,   M, m, m,   M, M, m,   m, M, m, //Faccia z = m
                m, m, M,   M, m, M,   M, M, M,   m, M, M  //Faccia z = M
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
            glm::mat4 model = fcg::translation((float) blockX, (float) blockY, (float) blockZ);

            glLineWidth(lineThickness); //Su alcuni driver (es. macOS core profile) resta 1.0, non e' bloccante

            shader.use();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

            glBindVertexArray(vao);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    };
}

#endif
