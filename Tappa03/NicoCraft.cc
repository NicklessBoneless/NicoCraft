#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include "Blocks.hh" //Messo per primo per dipendenze
#include "Chunk.hh"

const std::string dir = "../Tappa03/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa03";
const int TEXTUREPIXELSIZE = 32;

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

        window = new sf::Window (
                                 sf::VideoMode({window_width, window_height}),
                                 winTitle, //Title
                                 sf::Style::Default,
                                 sf::State::Windowed, //Window Type
                                 settings
                                 );
        window->setVerticalSyncEnabled (true);

        if(!window->setActive(true)){
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit (1);
        }

        int version = gladLoadGL (sf::Context::getFunction);
        if (!version) {
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit (1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

        //Colore della skybox di OpenGL
        glClearColor (0.53f, 0.81f, 0.92f, 1.0f);
    }

    ~Setup (){
        delete window;
    }
};

////////////////////
// Camera         //
////////////////////

class Camera{
public:
    glm::mat4 v;
    glm::mat4 vp;
    glm::mat4 pr;
    
private:
    float FovDegrees = 70.0f;
    float ar = 1.0f;

    glm::vec3 cameraPos = {8.0f, 40.0f, 40.0f};
    float phiDeg = 0.0f;
    float thetaDeg = 30.0f;

    bool sprinting = false;
    bool looking = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
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
    Camera (){
        SetWindowSize (Setup::window_width, Setup::window_height);
        ViewProjection ();
    }

    void SetWindowSize (int w, int h){
        ar = ((float) w) / (float) h;
        ViewProjection ();
    }

    void StartLook (float x, float y){
        looking = true;
        lastMouseX = x;
        lastMouseY = y;
    }

    void StopLook (){
        looking = false;
    }

    void Look (float x, float y){
        if (!looking)
            return;

        float dx = x - lastMouseX;
        float dy = y - lastMouseY;
        lastMouseX = x;
        lastMouseY = y;

        phiDeg += dx * mouseSensitivity;
        thetaDeg += dy * mouseSensitivity;
        thetaDeg = thetaDeg > 89.0f ? 89.0f : thetaDeg;
        thetaDeg = thetaDeg < -89.0f ? -89.0f : thetaDeg;

        ViewProjection ();
    }

    void Move (float dt){
        float phiRad = glm::radians (phiDeg);

        glm::vec3 forward = { glm::sin (phiRad), 0.0f, -glm::cos (phiRad) };
        glm::vec3 right   = { glm::cos (phiRad), 0.0f,  glm::sin (phiRad) };
        glm::vec3 up      = { 0.0f, 1.0f, 0.0f };

        glm::vec3 moveDir = {0.0f, 0.0f, 0.0f};

        for (const auto& keyBinding : moveBindings){
            if (sf::Keyboard::isKeyPressed (keyBinding.key)){
                moveDir += keyBinding.direction.x * right + keyBinding.direction.y * up + keyBinding.direction.z * forward;
            }
        }

        if (glm::length(moveDir) < 0.0001f)
            return;

        moveDir = glm::normalize (moveDir);
        cameraPos += moveDir * moveSpeed * dt;

        ViewProjection ();
    }

    void startSprint(){
        if(!sprinting){
            moveSpeed = 6.0f;
            FovDegrees += 0.5;
            sprinting = true;
            ViewProjection();
            return;
        }
    }

    void stopSprint(){
        if(sprinting){
            sprinting = false;
            moveSpeed = 2.0f;
            FovDegrees -= 0.5;
            ViewProjection();
        }
    }

    glm::mat4 ViewProjection(){
        float ncp = 0.1f;
        float fcp = 100.0f;

        glm::mat4 ry = fcg::rotation_y (phiDeg);
        glm::mat4 rx = fcg::rotation_x (thetaDeg);
        glm::mat4 t  = fcg::translation (-cameraPos.x, -cameraPos.y, -cameraPos.z);

        v = rx * ry * t; 

        float a = (fcp + ncp) / (ncp - fcp); //
        float b = 2.0f * fcp * ncp / (ncp - fcp); //

        float fd = 1.0f / glm::tan(glm::radians(FovDegrees / 2.0f));

        //Salvala direttamente in 'pr' della classe
        pr = glm::mat4(
            fd,  0.0,     0.0,  0.0,
            0.0, fd * ar, 0.0,  0.0,
            0.0, 0.0,       a, -1.0,
            0.0, 0.0,       b,  0.0
        ); 

        return pr * v;
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

//Riempie un chunk con lo stesso terreno di prova usato finora (erba sopra, terra sotto)
void FillTestChunk(Blocks::Chunk& chunk){
    const int SURFACE = Blocks::CHUNK_SIZE_Y*0.5;
    for (int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
        for (int y = 0; y < SURFACE; y++){
            for (int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                if(y == SURFACE-1) chunk.Set (x, y, z, Blocks::BlockType::GRASS);
                else if(y > SURFACE*0.8) chunk.Set (x, y, z, Blocks::BlockType::DIRT);
                else chunk.Set (x, y, z, Blocks::BlockType::STONE);
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

    static const int WORLD_CHUNKS_X = 2;
    static const int WORLD_CHUNKS_Z = 2;

public:
    Scene (fcg::Shaders& shaders){
        chunks.reserve (WORLD_CHUNKS_X * WORLD_CHUNKS_Z);

        //Fase 1: genera tutti i chunk (senza ancora costruire le mesh)
        for(int cz = 0; cz < WORLD_CHUNKS_Z; cz++){
            for(int cx = 0; cx < WORLD_CHUNKS_X; cx++){
                chunks.emplace_back ();
                ChunkInstance& instance = chunks.back ();
                instance.chunkX = cx;
                instance.chunkZ = cz;
                FillTestChunk (instance.chunk);
            }
        }

        //Fase 2: ora che tutti i chunk esistono, costruisci le mesh
        //potendo controllare correttamente i blocchi dei chunk vicini
        for(auto& instance : chunks){
            auto isSolidOutside = [this, &instance] (int lx, int ly, int lz) {
                return IsSolidWorld (instance.chunkX, instance.chunkZ, lx, ly, lz);
            };
            auto meshData = Blocks::BuildChunkMesh (instance.chunk, isSolidOutside);
            instance.mesh.Upload (meshData);
        }

        std::vector<std::string> texturePaths = {
            res + "MissingTextureBlock.png",
            res + "grassTop.png",
            res + "dirt.png",
            res + "grassSide.png",
            res + "stone.png"
        };

        textureArray.LoadTextures (texturePaths, TEXTUREPIXELSIZE, TEXTUREPIXELSIZE);

        Locations (shaders);
    }

    void Locations (fcg::Shaders& shaders)
    {
        modelLoc = glGetUniformLocation (shaders.program, "model");
        viewLoc  = glGetUniformLocation (shaders.program, "view");
        projLoc  = glGetUniformLocation (shaders.program, "projection");

        GLint samplerLoc = glGetUniformLocation (shaders.program, "textureArray");
        glUniform1i (samplerLoc, 0);
    }

    void Draw ()
    {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv (projLoc, 1, GL_FALSE, &camera.pr[0][0]);
        glUniformMatrix4fv (viewLoc, 1, GL_FALSE, &camera.v[0][0]);

        textureArray.Bind (0);

        //Disegna ogni chunk traslato nella sua posizione di griglia
        for (const auto& instance : chunks){
            glm::mat4 model = fcg::translation (
                instance.chunkX * Blocks::CHUNK_SIZE_X,
                0.0f,
                instance.chunkZ * Blocks::CHUNK_SIZE_Z
            );
             glUniformMatrix4fv (modelLoc, 1, GL_FALSE, &model[0][0]);
            instance.mesh.Draw ();
        }
    }

    private:
    //Cerca il chunk alle coordinate di griglia indicate, nullptr se non esiste
    Blocks::Chunk* GetChunkAt (int cx, int cz){
        for(auto& instance : chunks){
            if(instance.chunkX == cx && instance.chunkZ == cz) {
                return &instance.chunk;
            }
        }
        return nullptr;
    }

    //Controlla se e' solido un blocco a coordinate locali (anche fuori dai bordi 0..15)
    //rispetto al chunk (chunkX, chunkZ), guardando nel chunk vicino se necessario
    bool IsSolidWorld (int chunkX, int chunkZ, int localX, int localY, int localZ){
        //Y non e' suddiviso in chunk: se esce sopra/sotto e' semplicemente aria
        if(localY < 0 || localY >= Blocks::CHUNK_SIZE_Y) {
            return false;
        }

        int neighborChunkX = chunkX + (localX < 0 ? -1 : (localX >= Blocks::CHUNK_SIZE_X ? 1 : 0));
        int neighborChunkZ = chunkZ + (localZ < 0 ? -1 : (localZ >= Blocks::CHUNK_SIZE_Z ? 1 : 0));

        Blocks::Chunk* neighbor = GetChunkAt (neighborChunkX, neighborChunkZ);
        if(!neighbor) {
            return false; //Bordo del mondo: nessun chunk vicino, quindi aria
        }

        int wrappedX = ((localX % Blocks::CHUNK_SIZE_X) + Blocks::CHUNK_SIZE_X) % Blocks::CHUNK_SIZE_X;
        int wrappedZ = ((localZ % Blocks::CHUNK_SIZE_Z) + Blocks::CHUNK_SIZE_Z) % Blocks::CHUNK_SIZE_Z;

        return neighbor->IsSolid (wrappedX, localY, wrappedZ);
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

std::vector<keyBindings>ActionsKeyBindings (Scene& scene){
    return {
        { sf::Keyboard::Scancode::Escape, [] () { exit (0); } },
        { 
            sf::Keyboard::Scancode::LShift, 
            [&scene]() { scene.camera.startSprint(); },
            [&scene]() { scene.camera.stopSprint(); }
        }

    };
}

////////////////////
// SFML Callbacks //
////////////////////

void Handle (const sf::Event::Resized& resized, Camera& camera){
    glViewport (0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize (resized.size.x, resized.size.y);
}

//////////
// Main //
//////////

int main (){
    //// Startup ////

    Setup setup;
    sf::Window& window = *setup.window;

    fcg::Shaders shaders (dir + "shader_flat.vert", dir + "shader_flat.frag");
    shaders.use ();

    Scene scene (shaders);

    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);

    glEnable (GL_DEPTH_TEST);


    //// Main Loop ////
    std::vector<keyBindings> bindings = ActionsKeyBindings(scene);
    sf::Clock clock;
    bool running = true;
    while (running){
        while (const std::optional event = window.pollEvent ()){
            if (event->is<sf::Event::Closed> ())
                running = false;
            else if (const auto* resized = event->getIf<sf::Event::Resized> ())
                Handle (*resized, scene.camera);
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                for (const auto& binding : bindings) {
                    if (keyPressed->scancode == binding.key && binding.PressKey) {
                        binding.PressKey();
                    }
                }
            }
            else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                for (const auto& binding : bindings) {
                    if (keyReleased->scancode == binding.key && binding.ReleaseKey) {
                        binding.ReleaseKey();
                    }
                }
            }
            else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed> ()) {
                if (mousePressed->button == sf::Mouse::Button::Left)
                    scene.camera.StartLook (mousePressed->position.x, mousePressed->position.y);
            }
            else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased> ()) {
                if (mouseReleased->button == sf::Mouse::Button::Left)
                    scene.camera.StopLook ();
            }
            else if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved> ())
                scene.camera.Look (mouseMoved->position.x, mouseMoved->position.y);
        }

        float dt = clock.restart ().asSeconds ();
        scene.camera.Move (dt);

        scene.Draw ();
        window.display ();
        
    }

    return 0;
}