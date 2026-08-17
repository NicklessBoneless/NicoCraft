#ifndef CROSSHAIR_HH
#define CROSSHAIR_HH

#include "hotshaders.hh"

namespace fcg{
    class Crosshair{
    private:
        GLuint vao = 0, vbo = 0, ebo = 0;
        Shaders shader;
        GLint aspectLoc;

        //Dimensioni del mirino in coordinate NDC, prima della correzione di aspect ratio
        static constexpr float halfLength = 0.018f;
        static constexpr float halfThickness = 0.0025f;

    public:
        Crosshair(const std::string vertexFile, const std::string fragmentFile) : shader(vertexFile, fragmentFile){
            aspectLoc = glGetUniformLocation(shader.program, "aspectRatio");
            Build();
        }

        ~Crosshair(){
            if(vao){ glDeleteVertexArrays(1, &vao); }
            if(vbo){ glDeleteBuffers(1, &vbo); }
            if(ebo){ glDeleteBuffers(1, &ebo); }
        }

        void Build(){
            //Barra orizzontale + barra verticale: due rettangoli di 4 vertici (x, y) ciascuno
            float vertices[] = {
                //Barra orizzontale
                -halfLength, -halfThickness,
                 halfLength, -halfThickness,
                 halfLength,  halfThickness,
                -halfLength,  halfThickness,
                //Barra verticale
                -halfThickness, -halfLength,
                 halfThickness, -halfLength,
                 halfThickness,  halfLength,
                -halfThickness,  halfLength
            };

            uint32_t indices[] = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4
            };

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

          
            glBindVertexArray(0);
            
        }

        void Draw(float aspectRatio){
            //std::cerr << "aspectLoc=" << aspectLoc << " aspectRatio=" << aspectRatio << std::endl;
            //Disabilita il depth test per disegnare sempre sopra alla scena 3D
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            shader.use();
            glUniform1f(aspectLoc, aspectRatio);

            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }
    };
}

#endif