#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <cmath>
#include <limits>

#include "Blocks.hh" //Messo per primo per dipendenze
#include "Chunk.hh"
#include "Crosshair.hh"
#include "BlockOutline.hh"

const std::string dir = "../Tappa04/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa04";
const int TEXTUREPIXELSIZE = 32;
const int WORLDSIZECHUNKSX = 4;
const int WORLDSIZECHUNKSZ = 4;
const float REACH_DISTANCE = 6.0f; //Distanza massima di selezione del blocco

/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup{
public:
    //Width x Height 
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::Window* window;

    Setup(){
        sf::ContextSettings settings; //SFML Options
        settings.depthBits = 32;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Attribute::Core;
        settings.majorVersion = 4;
        settings.minorVersion = 1;

        window = new sf::Window(
                                 sf::VideoMode({window_width, window_height}),
                                 winTitle, //Title
                                 sf::Style::Default,
                                 sf::State::Windowed, //Window Type
                                 settings
                                 );
        window->setVerticalSyncEnabled(true);

        if(!window->setActive(true)){
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit(1);
        }

        int version = gladLoadGL(sf::Context::getFunction);
        if(!version){
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit(1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

        //Colore della skybox di OpenGL
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    }

    ~Setup(){
        delete window;
    }
};

////////////////////
// Camera         //
////////////////////

class Camera{
public:
    glm::mat4 viewMatrix;
    glm::mat4 viewProjMatrix;
    glm::mat4 projMatrix;
    
private:
    float fovDegrees = 70.0f;
    float aspectRatio = 1.0f;

    glm::vec3 cameraPos = {40.0f, 40.0f, 40.0f};
    float yawDeg = -45.0f;
    float pitchDeg = 25.0f;

    bool sprinting = false;
    

    const float mouseSensitivity = 0.15f;
    float moveSpeed = 2.0f;

    // Ad ogni tasto è correllata una direzione di movimento in coordinate della camera.
    // Ogni riga corrisponde ad un input di movimento
    struct MovementBindings{
        sf::Keyboard::Key key;
        glm::vec3 direction; // in coordinate locali: x=right, y=up, z=forward
    };

    const MovementBindings moveBindings[6] = {
        { sf::Keyboard::Key::W,        { 0.0f, 0.0f,  1.0f} },
        { sf::Keyboard::Key::S,        { 0.0f, 0.0f, -1.0f} },
        { sf::Keyboard::Key::D,        { 1.0f, 0.0f,  0.0f} },
        { sf::Keyboard::Key::A,        {-1.0f, 0.0f,  0.0f} },
        { sf::Keyboard::Key::Space,    { 0.0f, 1.0f,  0.0f} },
        { sf::Keyboard::Key::LControl, { 0.0f,-1.0f,  0.0f} },
    };


public:
    Camera(){
        SetWindowSize(Setup::window_width, Setup::window_height);
        ViewProjection();
    }

    void SetWindowSize(int width, int height){
        aspectRatio = ((float) width) / (float) height;
        ViewProjection();
    }

    void Look(float deltaX, float deltaY){
        yawDeg += deltaX * mouseSensitivity;
        pitchDeg += deltaY * mouseSensitivity;
        pitchDeg = pitchDeg > 89.0f ? 89.0f : pitchDeg;
        pitchDeg = pitchDeg < -89.0f ? -89.0f : pitchDeg;

        ViewProjection();
    }

    float GetAspectRatio() const{
        return aspectRatio;
    }

    glm::vec3 GetPosition() const{ //Const => Metodo di sola lettura
        return cameraPos;
    }

    glm::vec3 GetForward() const{
        float yawRad = glm::radians(yawDeg);
        float pitchRad = glm::radians(pitchDeg);

        //Ricavata invertendo la stessa rotazione Rx*Ry usata in ViewProjection()
        return glm::vec3(
            glm::sin(yawRad) * glm::cos(pitchRad),
            -glm::sin(pitchRad),
            -glm::cos(yawRad) * glm::cos(pitchRad)
        );
    }

    void Move(float deltaTime){
        float yawRad = glm::radians(yawDeg);

        glm::vec3 forward = { glm::sin(yawRad), 0.0f, -glm::cos(yawRad) };
        glm::vec3 right   = { glm::cos(yawRad), 0.0f,  glm::sin(yawRad) };
        glm::vec3 up      = { 0.0f, 1.0f, 0.0f };

        glm::vec3 moveDirection = {0.0f, 0.0f, 0.0f};

        for(const auto& keyBinding : moveBindings){
            if(sf::Keyboard::isKeyPressed(keyBinding.key)){
                moveDirection += keyBinding.direction.x * right + keyBinding.direction.y * up + keyBinding.direction.z * forward;
            }
        }

        if(glm::length(moveDirection) < 0.0001f)
            return;

        moveDirection = glm::normalize(moveDirection);
        cameraPos += moveDirection * moveSpeed * deltaTime;

        ViewProjection();
    }

    void startSprint(){
        if(!sprinting){
            moveSpeed = 6.0f;
            fovDegrees += 0.5;
            sprinting = true;
            ViewProjection();
            return;
        }
    }

    void stopSprint(){
        if(sprinting){
            sprinting = false;
            moveSpeed = 2.0f;
            fovDegrees -= 0.5;
            ViewProjection();
        }
    }

    glm::mat4 ViewProjection(){
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        glm::mat4 ry = fcg::rotation_y(yawDeg);
        glm::mat4 rx = fcg::rotation_x(pitchDeg);
        glm::mat4 t  = fcg::translation(-cameraPos.x, -cameraPos.y, -cameraPos.z);

        viewMatrix = rx * ry * t; 

        float perspectiveA = (farPlane + nearPlane) / (nearPlane - farPlane); //
        float perspectiveB = 2.0f * farPlane * nearPlane / (nearPlane - farPlane); //

        float focalDistance = 1.0f / glm::tan(glm::radians(fovDegrees / 2.0f));

        //Salvala direttamente in 'projMatrix' della classe
        projMatrix = glm::mat4(
            focalDistance,  0.0,                     0.0,          0.0,
            0.0,            focalDistance * aspectRatio, 0.0,       0.0,
            0.0,            0.0,                     perspectiveA, -1.0,
            0.0,            0.0,                     perspectiveB,  0.0
        ); 

        return projMatrix * viewMatrix;
    }
};

////////////////////
// Chunk Instance //
////////////////////

//Un chunk posizionato nella griglia mondo (coordinate in unita' di chunk, non di blocco)
struct ChunkInstance{
    Blocks::Chunk chunk;
    Blocks::ChunkMesh mesh;
    int chunkX;
    int chunkZ;
};

//Se colpisce un blocco esso ritorna le coordinate MONDO del blocco 
struct RaycastHit{
    bool hit = false;
    int blockX = 0;
    int blockY = 0;
    int blockZ = 0;
};

//Riempie un chunk con lo stesso terreno di prova usato finora (erba sopra, terra sotto)
void fillExistingChunks(Blocks::Chunk& chunk){
    const int SURFACE = Blocks::CHUNK_SIZE_Y*0.5;
    for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
        for(int y = 0; y < SURFACE; y++){
            for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                if(y == SURFACE-1){
                    chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                }
                else if(y < SURFACE * 0.90) chunk.Set(x, y, z, Blocks::BlockType::STONE);
                else chunk.Set(x, y, z, Blocks::BlockType::DIRT);

                if(y == SURFACE-1 && rand() % 100 >= 99){
                    chunk.Set(x, y+1, z, Blocks::BlockType::WOOD);
                    chunk.Set(x, y+2, z, Blocks::BlockType::LEAVES);
                }
                else if(rand() % 100 >= 66){
                    chunk.Set(x, y+1, z, Blocks::BlockType::AIR);
                }
            }
        }
    }
    
}

////////////////////
//     Scene      //
////////////////////

class Scene{
public:
    Camera camera;
    std::vector<ChunkInstance> chunks;
    Blocks::TextureArray textureArray;

private:
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;

    

public:
    Scene(fcg::Shaders& shaders){
        chunks.reserve(WORLDSIZECHUNKSX * WORLDSIZECHUNKSZ);

        //Fase 1: genera tutti i chunk (senza ancora costruire le mesh)
        for(int chunkZ = 0; chunkZ < WORLDSIZECHUNKSZ; chunkZ++){
            for(int chunkX = 0; chunkX < WORLDSIZECHUNKSX; chunkX++){
                chunks.emplace_back();
                ChunkInstance& instance = chunks.back();
                instance.chunkX = chunkX;
                instance.chunkZ = chunkZ;
                fillExistingChunks(instance.chunk);
            }
        }

        //Fase 2: ora che tutti i chunk esistono, costruisci le mesh
        //potendo controllare correttamente i blocchi dei chunk vicini
        for(auto& instance : chunks){
            auto isSolidOutside = [this, &instance](int localX, int localY, int localZ){
                return IsSolidWorld(instance.chunkX, instance.chunkZ, localX, localY, localZ);
            };
            auto meshData = Blocks::BuildChunkMesh(instance.chunk, isSolidOutside);
            instance.mesh.Upload(meshData);
        }

        std::vector<std::string> texturePaths = {
            res + "MissingTextureBlock.png",
            res + "grassTop.png",
            res + "dirt.png",
            res + "grassSide.png",
            res + "stone.png",
            res + "logTop.png",
            res + "logSide.png",
            res + "leaves.png" 
        };

        textureArray.LoadTextures(texturePaths, TEXTUREPIXELSIZE, TEXTUREPIXELSIZE);

        Locations(shaders);
    }

    void Locations(fcg::Shaders& shaders)
    {
        modelLoc = glGetUniformLocation(shaders.program, "model");
        viewLoc  = glGetUniformLocation(shaders.program, "view");
        projLoc  = glGetUniformLocation(shaders.program, "projection");

        GLint samplerLoc = glGetUniformLocation(shaders.program, "textureArray");
        glUniform1i(samplerLoc, 0);
    }

    void Draw(fcg::Shaders& shaders)
    {
        shaders.use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &camera.projMatrix[0][0]);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &camera.viewMatrix[0][0]);

        textureArray.Bind(0);

        //Disegna ogni chunk traslato nella sua posizione di griglia
        for(const auto& instance : chunks){
            glm::mat4 modelMatrix = fcg::translation(
                instance.chunkX * Blocks::CHUNK_SIZE_X,
                0.0f,
                instance.chunkZ * Blocks::CHUNK_SIZE_Z
            );
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
            instance.mesh.Draw();
        }
    }

    //Lancia un raggio da 'origin' lungo 'direction' (non serve normalizzata) e restituisce
    //il primo blocco solido incontrato entro 'maxDistance', con voxel traversal DDA
    RaycastHit RaycastBlock(glm::vec3 origin, glm::vec3 direction, float maxDistance){
        RaycastHit result;

        int x = (int) std::floor(origin.x);
        int y = (int) std::floor(origin.y);
        int z = (int) std::floor(origin.z);

        int stepX = direction.x > 0.0f ? 1 : -1;
        int stepY = direction.y > 0.0f ? 1 : -1;
        int stepZ = direction.z > 0.0f ? 1 : -1;

        float tDeltaX = direction.x != 0.0f ? std::abs(1.0f / direction.x) : std::numeric_limits<float>::infinity();
        float tDeltaY = direction.y != 0.0f ? std::abs(1.0f / direction.y) : std::numeric_limits<float>::infinity();
        float tDeltaZ = direction.z != 0.0f ? std::abs(1.0f / direction.z) : std::numeric_limits<float>::infinity();

        float tMaxX = NextBoundary(origin.x, x, direction.x, stepX);
        float tMaxY = NextBoundary(origin.y, y, direction.y, stepY);
        float tMaxZ = NextBoundary(origin.z, z, direction.z, stepZ);

        float traveled = 0.0f;
        while(traveled < maxDistance){
            if(IsSolidAtWorld(x, y, z)){
                result.hit = true;
                result.blockX = x;
                result.blockY = y;
                result.blockZ = z;
                return result;
            }

            if(tMaxX < tMaxY && tMaxX < tMaxZ){
                x += stepX;
                traveled = tMaxX;
                tMaxX += tDeltaX;
            }
            else if(tMaxY < tMaxZ){
                y += stepY;
                traveled = tMaxY;
                tMaxY += tDeltaY;
            }
            else{
                z += stepZ;
                traveled = tMaxZ;
                tMaxZ += tDeltaZ;
            }
        }

        return result; //Niente colpito entro maxDistance
    }

    private:
    //Cerca il chunk alle coordinate di griglia indicate, nullptr se non esiste
    Blocks::Chunk* GetChunkAt(int chunkX, int chunkZ){
        for(auto& instance : chunks){
            if(instance.chunkX == chunkX && instance.chunkZ == chunkZ){
                return &instance.chunk;
            }
        }
        return nullptr;
    }

    //Controlla se e' solido un blocco a coordinate locali (anche fuori dai bordi 0..15)
    //rispetto al chunk (chunkX, chunkZ), guardando nel chunk vicino se necessario
    bool IsSolidWorld(int chunkX, int chunkZ, int localX, int localY, int localZ){
        //Y non e' suddiviso in chunk: se esce sopra/sotto e' semplicemente aria
        if(localY < 0 || localY >= Blocks::CHUNK_SIZE_Y){
            return false;
        }

        int neighborChunkX = chunkX + (localX < 0 ? -1 : (localX >= Blocks::CHUNK_SIZE_X ? 1 : 0));
        int neighborChunkZ = chunkZ + (localZ < 0 ? -1 : (localZ >= Blocks::CHUNK_SIZE_Z ? 1 : 0));

        Blocks::Chunk* neighbor = GetChunkAt(neighborChunkX, neighborChunkZ);
        if(!neighbor){
            return false; //Bordo del mondo: nessun chunk vicino, quindi aria
        }

        int wrappedX = ((localX % Blocks::CHUNK_SIZE_X) + Blocks::CHUNK_SIZE_X) % Blocks::CHUNK_SIZE_X;
        int wrappedZ = ((localZ % Blocks::CHUNK_SIZE_Z) + Blocks::CHUNK_SIZE_Z) % Blocks::CHUNK_SIZE_Z;

        return neighbor->IsSolid(wrappedX, localY, wrappedZ);
    }

    //Divisione intera "verso il basso" (floor): serve perche' l'operatore % di C++
    //tronca verso zero, non verso -infinito, e con coordinate negative darebbe risultati sbagliati
    static int FloorDiv(int a, int b){
        int d = a / b;
        int r = a % b;
        return (r != 0 && ((r < 0) != (b < 0))) ? d - 1 : d;
    }

    //Distanza (in unita' di t lungo il raggio) dal punto 'originComp' al prossimo confine di voxel
    static float NextBoundary(float originComp, int voxelComp, float dirComp, int step){
        if(dirComp == 0.0f) return std::numeric_limits<float>::infinity();
        float boundary = step > 0 ? (float)(voxelComp + 1) : (float) voxelComp;
        return (boundary - originComp) / dirComp;
    }

    //Controlla se e' solido il blocco a coordinate MONDO, individuando da solo il chunk giusto
    bool IsSolidAtWorld(int worldX, int worldY, int worldZ){
        if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
            return false;
        }

        int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
        int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

        Blocks::Chunk* chunk = GetChunkAt(chunkX, chunkZ);
        if(!chunk){
            return false;
        }

        int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
        int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

        return chunk->IsSolid(localX, worldY, localZ);
    }
};


////////////////////
// Game  Bindings //
////////////////////

struct keyBindings{
    sf::Keyboard::Scancode key;
    std::function<void()> PressKey;
    std::function<void()> ReleaseKey = nullptr;
};

std::vector<keyBindings> ActionsKeyBindings(Scene& scene){
    return {
        { sf::Keyboard::Scancode::Escape, []() { exit(0); } },
        { 
            sf::Keyboard::Scancode::LShift, 
            [&scene](){ scene.camera.startSprint(); },
            [&scene](){ scene.camera.stopSprint(); }
        }

    };
}

////////////////////
// SFML Callbacks //
////////////////////

void Handle(const sf::Event::Resized& resized, Camera& camera){
    glViewport(0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize(resized.size.x, resized.size.y);
}

////////////////////
// AUX Functions  //
////////////////////

// Controlla se il tasto premuto o rilasciato è tra i keyBinding e chiama la funzione
void CheckBinding(sf::Keyboard::Scancode scancode, bool isPressed, const std::vector<keyBindings>& keyBinds) {
    for(const keyBindings &binding : keyBinds) {
        if(binding.key != scancode) continue;
        if(isPressed && binding.PressKey) binding.PressKey();
        else if(!isPressed && binding.ReleaseKey) binding.ReleaseKey();
    }
}


void HandleEvents(sf::Window& window, Camera& camera, const std::vector<keyBindings>& keyBinds, bool& running) {
    while (const std::optional event = window.pollEvent()) {
        if(event->is<sf::Event::Closed>()) 
            running = false;

        else if (const auto* resized = event->getIf<sf::Event::Resized>()) 
            Handle(*resized, camera);

        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            CheckBinding(keyPressed->scancode, true, keyBinds);  // Tasto premuto
        
        else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) 
            CheckBinding(keyReleased->scancode, false, keyBinds); // Tasto rilasciato
    }
}

void UpdateMouseInput(sf::Window& window, Camera& camera, const sf::Vector2i& windowCenter){
    if(window.hasFocus()) {
        sf::Vector2i curMousePos = sf::Mouse::getPosition(window);
        sf::Vector2i delta = curMousePos - windowCenter;

        camera.Look((float)delta.x, (float)delta.y);
        sf::Mouse::setPosition(windowCenter, window);
    }
}

//////////
// Main //
//////////

int main(){
    //// Startup ////
    Setup setup;
    sf::Window& window = *setup.window;

    //Prendiamo e centriamo il cursore per la camera FPS
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);
    sf::Vector2i windowCenter = { (int) (window.getSize().x / 2), (int) (window.getSize().y / 2) };
    sf::Mouse::setPosition(windowCenter, window);

    //Carichiamo la shader di base del mondo 3D
    fcg::Shaders shaders(dir + "shader_flat.vert", dir + "shader_flat.frag");
    shaders.use();

    //Creo la scena
    Scene scene(shaders);

    //Carichiamo la Crosshair e l'outline dei blocchi
    fcg::Crosshair crosshair(dir + "shader_crosshair.vert", dir + "shader_crosshair.frag");
    fcg::BlockOutline outline(dir + "shader_outline.vert", dir + "shader_outline.frag");
    
    //Per migliorare la performance ;-)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    //// Main Loop ////
    std::vector<keyBindings> keyBindings = ActionsKeyBindings(scene);
    sf::Clock clock; //Utile per il deltaTime
    bool running = true;
    
    while(running){
        //Controllo input Tastiera
        HandleEvents(window,scene.camera,keyBindings,running);

        float deltaTime = clock.restart().asSeconds();
        scene.camera.Move(deltaTime);

        //Mouse Input
        UpdateMouseInput(window,scene.camera,windowCenter);

        //Disegno il mondo 3D
        scene.Draw(shaders);

        //RayCast dalla camera
        RaycastHit target = scene.RaycastBlock(scene.camera.GetPosition(), scene.camera.GetForward(), REACH_DISTANCE);
        if(target.hit){
            outline.Draw(target.blockX, target.blockY, target.blockZ, scene.camera.viewMatrix, scene.camera.projMatrix);
        }

        //Disegno la Crosshair
        crosshair.Draw(scene.camera.GetAspectRatio());
        
        window.display();
    }

    return 0;
}