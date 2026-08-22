#ifndef SKY_HH
#define SKY_HH

#include <SFML/Graphics/Texture.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "Hotshaders.hh"
#include "Camera.hh"

namespace fcg{

    //Gestisce il ciclo giorno/notte: avanzamento del tempo, colore del cielo, fattore di
    //luce globale (usato anche per lo shading dei blocchi in Renderer), e il disegno di
    //Sole, Luna e stelle. E' la fonte unica di verita' per lo stato temporale del cielo
    class Sky{
    private:
        //// Stato temporale ////
        float elapsedTime = 0.0f;
        static constexpr float daylightSpeed = 0.3f; //Un ciclo completo dura circa 2*PI/daylightSpeed secondi (~63s)
        static constexpr float minDaylight = 0.3f;   //Luminosita' minima dei blocchi (notte fonda)
        static constexpr float maxDaylight = 1.0f;   //Luminosita' massima dei blocchi (pieno giorno)

        const glm::vec3 daySky = {0.53f, 0.81f, 0.92f};
        const glm::vec3 nightSky = {0.01f, 0.015f, 0.04f};

        //// Sole e Luna ////
        static constexpr float skyRadius = 90.0f;  //Distanza di Sole/Luna dalla camera
        static constexpr float celestialHalfSize = 8.0f;  //Meta' lato del quad di Sole/Luna
        GLuint celestialVao = 0, celestialVbo = 0;
        Shaders celestialShader;
        GLint celestialViewLoc = -1, celestialProjLoc = -1, celestialAlphaLoc = -1;
        sf::Texture sunTexture, moonTexture;

        //// Stelle ////
        static constexpr float starRadius = 90.0f; //Deve restare sotto il farPlane (100.0f) della Camera
        static constexpr float starPointSize = 8.0f;
        GLuint starsVao = 0, starsVbo = 0;
        Shaders starsShader;
        GLint starsViewLoc = -1, starsProjLoc = -1, starsCamPosLoc = -1;
        GLint starsAlphaLoc = -1, starsRadiusLoc = -1, starsPointSizeLoc = -1;
        int starCount = 0;

       

    public:
        Sky(const std::string& resourcesDir, const std::string& shaderDir) :
            celestialShader(shaderDir + "shader_sky.vert", shaderDir + "shader_sky.frag"),
            starsShader(shaderDir + "shader_stars.vert", shaderDir + "shader_stars.frag")
        {
            LoadTextures(resourcesDir);
            BuildCelestialQuad();
            BuildStars();
            Locations();
        }

        ~Sky(){
            Cleanup();
        }

        //Va chiamato una volta per frame, prima di Draw()
        void Update(float deltaTime){
            elapsedTime += deltaTime;
        }

        //t oscilla tra 0 (notte piena) e 1 (giorno pieno)
        float GetDayNightT() const{
            float angle = elapsedTime * daylightSpeed;
            return (glm::sin(angle) + 1.0f) * 0.5f;
        }

        //Fattore di luce globale da applicare ai blocchi del mondo (range [minDaylight, maxDaylight])
        float GetDaylightFactor() const{
            return minDaylight + (maxDaylight - minDaylight) * GetDayNightT();
        }

        //Colore corrente del cielo, da passare a glClearColor
        glm::vec3 GetSkyColor() const{
            return nightSky + (daySky - nightSky) * GetDayNightT();
        }

        //Disegna Sole, Luna e stelle. Va chiamato dopo il clear, prima del mondo
        void Draw(Camera& camera){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_CULL_FACE);

            glDepthMask(GL_FALSE);

            float t = GetDayNightT();
            DrawStars(camera, t);
            DrawSunMoon(camera, t);
            

            // RIABILITA la scrittura sul Depth Buffer
            glDepthMask(GL_TRUE);

            glEnable(GL_CULL_FACE);
            glDisable(GL_BLEND);
        }

    private:
        void LoadTextures(const std::string& resourcesDir){
            if(!sunTexture.loadFromFile(resourcesDir + "sun.png")){
                std::cerr << "Errore nel caricamento di sun.png" << std::endl;
                if(!sunTexture.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                } 
            }
            if(!moonTexture.loadFromFile(resourcesDir + "moon.png")){
                std::cerr << "Errore nel caricamento di moon.png" << std::endl;
                if(!moonTexture.loadFromFile(resourcesDir + "missingTextureBlock.png")){
                    std::cerr << "Errore nel caricamento texture di fallback :-(" << std::endl;
                }
            }
        }

        void BuildCelestialQuad(){
            //6 vertici (2 triangoli): posizione xyz + texcoord uv, ricalcolati ogni frame via glBufferSubData
            glGenVertexArrays(1, &celestialVao);
            glBindVertexArray(celestialVao);

            glGenBuffers(1, &celestialVbo);
            glBindBuffer(GL_ARRAY_BUFFER, celestialVbo);
            glBufferData(GL_ARRAY_BUFFER, 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }

        //Genera 'count' direzioni casuali sull'emisfero superiore (le stelle non servono sotto l'orizzonte)
        void BuildStars(){
            const int count = 250;
            starCount = count;
            std::vector<float> directions;
            directions.reserve(count * 3);

            for(int i = 0; i < count; i++){
                float theta = ((float) rand() / RAND_MAX) * 6.2831853f;     //Angolo attorno all'asse Y, 0..2*PI
                float height = ((float) rand() / RAND_MAX) * 0.9f + 0.05f;  //Solo emisfero superiore, evita l'orizzonte esatto
                float radius = std::sqrt(1.0f - height * height);

                directions.push_back(radius * std::cos(theta)); //x
                directions.push_back(height);                    //y
                directions.push_back(radius * std::sin(theta)); //z
            }

            glGenVertexArrays(1, &starsVao);
            glBindVertexArray(starsVao);

            glGenBuffers(1, &starsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, starsVbo);
            glBufferData(GL_ARRAY_BUFFER, directions.size() * sizeof(float), directions.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }

        void Locations(){
            celestialShader.use();
            celestialViewLoc  = glGetUniformLocation(celestialShader.program, "view");
            celestialProjLoc  = glGetUniformLocation(celestialShader.program, "projection");
            celestialAlphaLoc = glGetUniformLocation(celestialShader.program, "alpha");
            glUniform1i(glGetUniformLocation(celestialShader.program, "celestialTexture"), 0);

            starsShader.use();
            starsViewLoc      = glGetUniformLocation(starsShader.program, "view");
            starsProjLoc      = glGetUniformLocation(starsShader.program, "projection");
            starsCamPosLoc    = glGetUniformLocation(starsShader.program, "cameraPos");
            starsAlphaLoc     = glGetUniformLocation(starsShader.program, "alpha");
            starsRadiusLoc    = glGetUniformLocation(starsShader.program, "starRadius");
            starsPointSizeLoc = glGetUniformLocation(starsShader.program, "pointSize");
        }

        //Costruisce il quad (world space) per Sole o Luna e lo carica nel VBO dinamico
        void UpdateCelestialQuad(glm::vec3 center, glm::vec3 right, glm::vec3 up){
            glm::vec3 p0 = center - right * celestialHalfSize - up * celestialHalfSize;
            glm::vec3 p1 = center + right * celestialHalfSize - up * celestialHalfSize;
            glm::vec3 p2 = center + right * celestialHalfSize + up * celestialHalfSize;
            glm::vec3 p3 = center - right * celestialHalfSize + up * celestialHalfSize;

            float vertices[] = {
                p0.x, p0.y, p0.z, 0.0f, 0.0f,
                p1.x, p1.y, p1.z, 1.0f, 0.0f,
                p2.x, p2.y, p2.z, 1.0f, 1.0f,
                p0.x, p0.y, p0.z, 0.0f, 0.0f,
                p2.x, p2.y, p2.z, 1.0f, 1.0f,
                p3.x, p3.y, p3.z, 0.0f, 1.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, celestialVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        }

        void DrawSunMoon(Camera& camera, float t){
            float angle = elapsedTime * daylightSpeed;
            glm::vec3 cameraPos = camera.getPosition();

            //Sole e Luna si muovono sull'arco nel piano X-Y (asse di rotazione fisso = Z),
            //sempre agli estremi opposti. 'rotationAxis' e' l'asse stesso: non e' mai
            //parallelo alla normale del quad (che sta nel piano X-Y), quindi 'up' non degenera mai
            glm::vec3 rotationAxis = {0.0f, 0.0f, 1.0f};

            celestialShader.use();
            glUniformMatrix4fv(celestialViewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);
            glUniformMatrix4fv(celestialProjLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
            glBindVertexArray(celestialVao);

            //Sole: alpha alta di giorno, sparisce di notte
            glm::vec3 sunDir = {glm::cos(angle), glm::sin(angle), 0.0f};
            glm::vec3 sunUp = glm::normalize(glm::cross(-sunDir, rotationAxis));
            UpdateCelestialQuad(cameraPos + sunDir * skyRadius, rotationAxis, sunUp);
            sf::Texture::bind(&sunTexture);
            glUniform1f(celestialAlphaLoc, t);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            //Luna: sempre all'estremo opposto del Sole (angle + PI), alpha alta di notte
            glm::vec3 moonDir = {glm::cos(angle + 3.14159265f), glm::sin(angle + 3.14159265f), 0.0f};
            glm::vec3 moonUp = glm::normalize(glm::cross(-moonDir, rotationAxis));
            UpdateCelestialQuad(cameraPos + moonDir * skyRadius, rotationAxis, moonUp);
            sf::Texture::bind(&moonTexture);
            glUniform1f(celestialAlphaLoc, 1.0f - t);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(0);
            sf::Texture::bind(nullptr);
        }

        void DrawStars(Camera& camera, float t){
            //Soglia piu' bassa = le stelle iniziano a comparire prima nel crepuscolo,
            //non solo a notte fonda. 0.2f invece di 0.5f: compaiono gia' quando il cielo
            //e' scuro al 20% invece di aspettare il 50%
            const float starThreshold = 0.6f;
            float starVisibility = 1.0f-t;
            starVisibility = starVisibility < starThreshold ? 0.0f : (starVisibility - starThreshold) / (1.0f - starThreshold);
            if(starVisibility <= 0.0f) return;

            starsShader.use();
            glm::vec3 cameraPos = camera.getPosition();
            glUniformMatrix4fv(starsViewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);
            glUniformMatrix4fv(starsProjLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
            glUniform3f(starsCamPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
            glUniform1f(starsAlphaLoc, starVisibility);
            glUniform1f(starsRadiusLoc, starRadius);
            glUniform1f(starsPointSizeLoc, starPointSize);

            glBindVertexArray(starsVao);
            glDrawArrays(GL_POINTS, 0, starCount);
            glBindVertexArray(0);

            GLenum err = glGetError();
            if(err != GL_NO_ERROR){
                std::cerr << "GL error dopo disegno stelle: " << err << std::endl;
            }
        }

        void Cleanup(){
            if(celestialVao){ glDeleteVertexArrays(1, &celestialVao); celestialVao = 0; }
            if(celestialVbo){ glDeleteBuffers(1, &celestialVbo); celestialVbo = 0; }
            if(starsVao){ glDeleteVertexArrays(1, &starsVao); starsVao = 0; }
            if(starsVbo){ glDeleteBuffers(1, &starsVbo); starsVbo = 0; }
        }
    };
}

#endif
