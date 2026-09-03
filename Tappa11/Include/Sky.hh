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

    //Gestisce il ciclo giorno/notte: avanzamento del tempo, colore del cielo,
    //Si usa un fattore di luce globale (usato anche per lo shading dei blocchi in Renderer), e il disegno di Sole, Luna e stelle. 
    class Sky{
    private:
        //// Stato temporale ////
        float elapsedTime = 50.0f; //Metà giornata è sempre PI/daylightSpeed*2
        static constexpr float daylightSpeed = 0.05f; //Un ciclo completo dura circa 6.28/daylightSpeed secondi (20 min)
        static constexpr float minDaylight = 0.30f;   //Luminosita' minima dei blocchi (notte fonda)
        static constexpr float maxDaylight = 1.0f;   //Luminosita' massima dei blocchi (pieno giorno)

        const glm::vec3 daySky = {0.53f, 0.81f, 0.92f};
        const glm::vec3 nightSky = {0.01f, 0.015f, 0.04f};
        const float skyColorMultiplier = 0.9f;
        const float lightMultiplier = 1.00f;

        //// Sole e Luna ////
        static constexpr float skyRadius = 90.0f;  //Distanza di Sole/Luna dalla camera
        static constexpr float celestialHalfSize = 8.0f;  //Meta' lato del quad di Sole/Luna
        GLuint celestialVao = 0, celestialVbo = 0;
        Shaders celestialShader;
        GLint celestialViewLoc = -1, celestialProjLoc = -1, celestialAlphaLoc = -1;
        sf::Texture sunTexture, moonTexture;

        //// Stelle ////
        static constexpr float starRadius = 95.0f; //Deve restare sotto il farPlane (100.0f) della Camera
        static constexpr float starPointSize = 0.50f;
        static constexpr float starThreshold = 0.58f;
        GLuint starsVao = 0, starsVbo = 0, starsEbo = 0;
        Shaders starsShader;
        GLint starsViewLoc = -1, starsProjLoc = -1, starsCamPosLoc = -1;
        GLint starsAlphaLoc = -1, starsRadiusLoc = -1, starsSizeLoc = -1;
        const int starCount = 225;
        GLsizei starIndexCount = 0;
       
       

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
            float customT = std::pow(GetDayNightT(), 1.23f);
            float daylight = glm::mix(minDaylight, maxDaylight * lightMultiplier, customT);

            if(daylight >= maxDaylight) return maxDaylight;
            if(daylight <= minDaylight) return minDaylight;
            return daylight;
        }

        //Colore corrente del cielo, da passare a glClearColor
        glm::vec3 GetSkyColor() const{
            float customT = std::pow(GetDayNightT(), 2.0f);
            glm::vec3 skyColor = glm::mix(nightSky, daySky, customT);
            return skyColor;
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
            std::vector<float> vertexData; //Ora: direzione(3) + corner locale(2) + rotazione(1) = 6 float per vertice, 4 vertici per stella
            vertexData.reserve(starCount* 4 * 6);
            std::vector<uint32_t> indices;
            indices.reserve(starCount * 6);

            const float localCorners[4][2] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };

            for(int i = 0; i < starCount; i++){
                //Qui generiamo la posizione delle stelle SULLA MEZZASFERA superiore
                float theta = ((float) rand() / RAND_MAX) * M_PI * 2;
                float starY = ((float) rand() / RAND_MAX) * 0.99f + 0.005f;
                float rangeForThatStarY = std::sqrt(1.0f - starY*starY); //Teorema di pitagora applicato a una sfera

                //Posizione X e Z randomica con range giusto per quell'altezza
                float starX = rangeForThatStarY * std::cos(theta); 
                float starZ = rangeForThatStarY * std::sin(theta);

                float rotation = ((float) rand() / RAND_MAX) * M_PI * 2; //Rotazione randomica QUAD da 0 e 359.99... gradi

                uint32_t base = (uint32_t)(vertexData.size() / 6);
                for(int c = 0; c < 4; c++){
                    vertexData.insert(vertexData.end(), {
                        starX, starY, starZ,
                        localCorners[c][0], localCorners[c][1],
                        rotation //stessa rotazione sui 4 vertici della stessa stella
                    });
                }
                indices.insert(indices.end(), { base+0, base+1, base+2, base+2, base+3, base+0 });
            }

            starIndexCount = (GLsizei) indices.size();


            glGenVertexArrays(1, &starsVao);
            glBindVertexArray(starsVao);

            //Coordinate delle stelle passate
            glGenBuffers(1, &starsVbo);
            glBindBuffer(GL_ARRAY_BUFFER, starsVbo);
            glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

            //Indici delle stelle
            glGenBuffers(1, &starsEbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, starsEbo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

            //Configurazione del VAO
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
            glEnableVertexAttribArray(2);

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
            starsSizeLoc = glGetUniformLocation(starsShader.program, "starSize");
        }

        //Costruisce il quad (world space) per Sole o Luna e lo carica nel VBO dinamico
        void UpdateCelestialQuad(glm::vec3 center, glm::vec3 right, glm::vec3 up){
            glm::vec3 p0 = center - right * celestialHalfSize - up * celestialHalfSize;
            glm::vec3 p1 = center + right * celestialHalfSize - up * celestialHalfSize;
            glm::vec3 p2 = center + right * celestialHalfSize + up * celestialHalfSize;
            glm::vec3 p3 = center - right * celestialHalfSize + up * celestialHalfSize;

            float vertices[] = { //Posizione, coordivate UV
                p0.x, p0.y, p0.z, 0.0f, 0.0f, 
                p1.x, p1.y, p1.z, 1.0f, 0.0f,
                p2.x, p2.y, p2.z, 1.0f, 1.0f,
                p0.x, p0.y, p0.z, 0.0f, 0.0f,
                p2.x, p2.y, p2.z, 1.0f, 1.0f,
                p3.x, p3.y, p3.z, 0.0f, 1.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, celestialVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); //Va sovrascrivere il buffer, non lo ricrea da 0.
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
            glm::vec3 sunCirclePosition = {glm::cos(angle), glm::sin(angle), 0.0f}; //Posizione sole in un cerchio di r = 1
            glm::vec3 sunCircleTangent = glm::normalize(glm::cross(-sunCirclePosition, rotationAxis)); //Calcolo del vettore tangente al cerchio
            glm::vec3 sunActualPosition = cameraPos + sunCirclePosition * skyRadius;
            UpdateCelestialQuad(sunActualPosition, rotationAxis, sunCircleTangent);
            sf::Texture::bind(&sunTexture);
            glUniform1f(celestialAlphaLoc, 0.0f + t);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            //Luna: sempre all'estremo opposto del Sole (angle + PI), alpha alta di notte
            glm::vec3 moonCirclePosition = {glm::cos(angle + M_PI), glm::sin(angle + M_PI), 0.0f}; //Con cerchio di raggio 1
            glm::vec3 moonCircleTangent = glm::normalize(glm::cross(-moonCirclePosition, rotationAxis));
            glm::vec3 moonActualPosition = cameraPos + moonCirclePosition * skyRadius;
            UpdateCelestialQuad(moonActualPosition, rotationAxis, moonCircleTangent);
            sf::Texture::bind(&moonTexture);
            glUniform1f(celestialAlphaLoc, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(0);
            sf::Texture::bind(nullptr);
        }

        void DrawStars(Camera& camera, float t){       
            float starVisibility = 1.0f-t; //Valore attuale di visibilità delle stelle, da comparare a starThreshold
            starVisibility = starVisibility < starThreshold ? (0.0f) : (starVisibility - starThreshold) / (1.0f - starThreshold);
            if(starVisibility <= 0.0f) return;

            starsShader.use();
            glm::vec3 cameraPos = camera.getPosition();
            glUniformMatrix4fv(starsViewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);
            glUniformMatrix4fv(starsProjLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
            glUniform3f(starsCamPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
            glUniform1f(starsAlphaLoc, starVisibility);

            glUniform1f(starsRadiusLoc, starRadius);
            glUniform1f(starsSizeLoc, starPointSize); //Rinominato concettualmente da "pointSize" a "starSize"

            glBindVertexArray(starsVao);
            glDrawElements(GL_TRIANGLES, starIndexCount, GL_UNSIGNED_INT, 0);
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
            if(starsEbo){ glDeleteBuffers(1, &starsEbo); starsEbo = 0; }
        }
    };
}

#endif
