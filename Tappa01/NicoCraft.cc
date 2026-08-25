#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <iostream>
#include <cstdlib>

#include "./Include/Matrices.hh"
#include "./Include/Hotshaders.hh"

const std::string dir = "../Tappa01/";
const std::string res = "../Resources/";

/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup{
public:
    //Width x Height 
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::Window* window;

    Setup (){
        sf::ContextSettings settings; //SFML Options
        settings.depthBits = 32;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Attribute::Core;
        settings.majorVersion = 4;
        settings.minorVersion = 1;

        window = new sf::Window (
                                 sf::VideoMode({window_width, window_height}),
                                 "NicoCraft - Tappa01", //Title
                                 sf::Style::Default,
                                 sf::State::Windowed, //Window Type
                                 settings
                                 );
        window->setVerticalSyncEnabled (true);

        if (!window->setActive (true)) {
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit (1);
        }

        int version = gladLoadGL (sf::Context::getFunction);
        if (!version) {
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit (1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;
    }

    ~Setup ()
    {
        delete window;
    }
};

////////////////////
// Camera         //
////////////////////

class Camera
{
public:
    glm::mat4 v;
    glm::mat4 vp;

    
private:
    float fd = 50.0f / 18.0f;
    float ar = 1.0f;

    glm::vec3 cameraPos = {0.0f, 0.0f, 2.5f};
    float phiDeg = 0.0f;
    float thetaDeg = 0.0f;

    bool sprinting = false;
    bool looking = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    const float mouseSensitivity = 0.15f;
    float moveSpeed = 2.0f;

    // Ad ogni tasto è correllata una direzione di movimento in coordinate della camera.
    // Ogni riga corrisponde ad un input di movimento
    struct MovementBindings
    {
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
    Camera ()
    {
        SetWindowSize (Setup::window_width, Setup::window_height);
        ViewProjection ();
    }

    void SetWindowSize (int w, int h)
    {
        ar = ((float) w) / (float) h;
        ViewProjection ();
    }

    void StartLook (float x, float y)
    {
        looking = true;
        lastMouseX = x;
        lastMouseY = y;
    }

    void StopLook ()
    {
        looking = false;
    }

    void Look (float x, float y)
    {
        if (!looking) return;

        float dx = x - lastMouseX;
        float dy = y - lastMouseY;
        lastMouseX = x;
        lastMouseY = y;

        phiDeg += dx * mouseSensitivity;
        thetaDeg += dy * mouseSensitivity;
        thetaDeg = thetaDeg > 90.0f ? 90.0f : thetaDeg;
        thetaDeg = thetaDeg < -90.0f ? -90.0f : thetaDeg;

        ViewProjection ();
    }

    void Move (float dt)
    {
        float phiRad = glm::radians (phiDeg);

        glm::vec3 forward = { glm::sin (phiRad), 0.0f, -glm::cos (phiRad) };
        glm::vec3 right   = { glm::cos (phiRad), 0.0f,  glm::sin (phiRad) };
        glm::vec3 up      = { 0.0f, 1.0f, 0.0f };

        glm::vec3 moveDir = {0.0f, 0.0f, 0.0f};

        for (const auto& keyBinding : moveBindings) {
            if (sf::Keyboard::isKeyPressed (keyBinding.key)) {
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
            sprinting = true;
            return;
        }
    }

    void stopSprint(){
        if(sprinting){
            sprinting = false;
            moveSpeed = 2.0f;
        }
    }

    void ViewProjection ()
    {
        float ncp = 0.1f;
        float fcp = 100.0f;

        glm::mat4 ry = fcg::rotation_y (phiDeg);
        glm::mat4 rx = fcg::rotation_x (thetaDeg);
        glm::mat4 t  = fcg::translation (-cameraPos.x, -cameraPos.y, -cameraPos.z);

        float a = (fcp + ncp) / (ncp - fcp);
        float b = 2.0f * fcp * ncp / (ncp - fcp);

        glm::mat4 pr = glm::mat4(
                                 fd,  0.0,     0.0,  0.0,
                                 0.0, fd * ar, 0.0,  0.0,
                                 0.0, 0.0,       a, -1.0,
                                 0.0, 0.0,       b,  0.0
                                 );

        v = rx * ry * t;
        vp = pr * v;
    }
};

////////////////////
// Cube (texture, niente luce dinamica) //
////////////////////

class GPUCube
{
private:
    GLuint vbo;
    GLuint vao;
    GLuint texture;

public:
    GPUCube (const std::string& texture_path) { Load (texture_path); }
    ~GPUCube () { Clean (); }

    void Load (const std::string& texture_path)
    {
        // pos (3) + uv (2) = 5 float per vertice (niente normali: non servono più)
        static const float vertices[] = {
            // Front (+Z)
            -0.5f,-0.5f, 0.5f,  0,0,
             0.5f,-0.5f, 0.5f,  1,0,
             0.5f, 0.5f, 0.5f,  1,1,
             0.5f, 0.5f, 0.5f,  1,1,
            -0.5f, 0.5f, 0.5f,  0,1,
            -0.5f,-0.5f, 0.5f,  0,0,

            // Back (-Z)
             0.5f,-0.5f,-0.5f,  0,0,
            -0.5f,-0.5f,-0.5f,  1,0,
            -0.5f, 0.5f,-0.5f,  1,1,
            -0.5f, 0.5f,-0.5f,  1,1,
             0.5f, 0.5f,-0.5f,  0,1,
             0.5f,-0.5f,-0.5f,  0,0,

            // Left (-X)
            -0.5f,-0.5f,-0.5f,  0,0,
            -0.5f,-0.5f, 0.5f,  1,0,
            -0.5f, 0.5f, 0.5f,  1,1,
            -0.5f, 0.5f, 0.5f,  1,1,
            -0.5f, 0.5f,-0.5f,  0,1,
            -0.5f,-0.5f,-0.5f,  0,0,

            // Right (+X)
             0.5f,-0.5f, 0.5f,  0,0,
             0.5f,-0.5f,-0.5f,  1,0,
             0.5f, 0.5f,-0.5f,  1,1,
             0.5f, 0.5f,-0.5f,  1,1,
             0.5f, 0.5f, 0.5f,  0,1,
             0.5f,-0.5f, 0.5f,  0,0,

            // Top (+Y)
            -0.5f, 0.5f, 0.5f,  0,0,
             0.5f, 0.5f, 0.5f,  1,0,
             0.5f, 0.5f,-0.5f,  1,1,
             0.5f, 0.5f,-0.5f,  1,1,
            -0.5f, 0.5f,-0.5f,  0,1,
            -0.5f, 0.5f, 0.5f,  0,0,

            // Bottom (-Y)
            -0.5f,-0.5f,-0.5f,  0,0,
             0.5f,-0.5f,-0.5f,  1,0,
             0.5f,-0.5f, 0.5f,  1,1,
             0.5f,-0.5f, 0.5f,  1,1,
            -0.5f,-0.5f, 0.5f,  0,1,
            -0.5f,-0.5f,-0.5f,  0,0,
        };

        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays (1, &vao);
        glBindVertexArray (vao);

        // Attribute 0: position (x, y, z)
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof (float), (void*) 0);
        glEnableVertexAttribArray (0);

        // Attribute 1: uv (u, v)
        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof (float), (void*) (3 * sizeof (float)));
        glEnableVertexAttribArray (1);

        LoadTexture (texture_path);
    }

    void LoadTexture (const std::string& path){
        sf::Image image;
        bool loaded = image.loadFromFile (path);

        if (!loaded) {
            std::cerr << "Warning: Failed to load texture: " << path
                    << " — using fallback texture." << std::endl;

            std::string fallbackPath = res + "missingTextureBlock.png";
            if (!image.loadFromFile (fallbackPath)) {
                std::cerr << "Error: Failed to load fallback texture: " << fallbackPath << std::endl;
                exit (1);
            }
        }

        auto size = image.getSize ();

        glGenTextures (1, &texture);
        glBindTexture (GL_TEXTURE_2D, texture);

        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr ());
    }

    void Clean ()
    {
        glDeleteVertexArrays (1, &vao);
        glDeleteBuffers (1, &vbo);
        glDeleteTextures (1, &texture);
    }

    void Draw ()
    {
        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, texture);
        glBindVertexArray (vao);
        glDrawArrays (GL_TRIANGLES, 0, 36);
    }
};



////////////////////
// Scene           //
////////////////////

class Scene
{
public:
    Camera camera;
    GPUCube cube;

private:
    GLint model_loc;
    GLint vp_loc;

public:
    Scene (fcg::Shaders& shaders) : cube (dir + "block.png")
    {
        Locations (shaders);
    }

    void Locations (fcg::Shaders& shaders)
    {
        model_loc = glGetUniformLocation (shaders.program, "model");
        vp_loc = glGetUniformLocation (shaders.program, "vp");
    }

    void Draw ()
    {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv (vp_loc, 1, GL_FALSE, &camera.vp[0][0]);

        glm::mat4 model = fcg::identity ();
        glUniformMatrix4fv (model_loc, 1, GL_FALSE, &model[0][0]);

        cube.Draw ();
    }
};

////////////////////
// Game  Bindings //
////////////////////

struct keyBindings
{
    sf::Keyboard::Scancode key;
    std::function<void()> PressKey;
    std::function<void()> ReleaseKey = nullptr;
};

std::vector<keyBindings>ActionsKeyBindings (Scene& scene)
{
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

void Handle (const sf::Event::Resized& resized, Camera& camera)
{
    glViewport (0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize (resized.size.x, resized.size.y);
}

//////////
// Main //
//////////

int main ()
{
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
    while (running)
    {
        while (const std::optional event = window.pollEvent ())
        {
            if (event->is<sf::Event::Closed> ())
                running = false;
            else if (const auto* resized = event->getIf<sf::Event::Resized> ())
                Handle (*resized, scene.camera);
            else if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                // Scorriamo tutti i binding che abbiamo definito
                for (const auto& binding : bindings) {
                    // Se il tasto premuto coincide con la chiave e c'è una funzione definita
                    if (key_pressed->scancode == binding.key && binding.PressKey) {
                        binding.PressKey(); // Esegue ad es. scene.camera.startSprint()
                    }
                }
            }
            else if (const auto* key_released = event->getIf<sf::Event::KeyReleased>()) {
                // Scorriamo nuovamente i binding
                for (const auto& binding : bindings) {
                    // Se il tasto rilasciato coincide con la chiave e c'è una funzione di rilascio
                    if (key_released->scancode == binding.key && binding.ReleaseKey) {
                        binding.ReleaseKey(); // Esegue ad es. scene.camera.stopSprint()
                    }
                }
            }
            else if (const auto* mouse_pressed = event->getIf<sf::Event::MouseButtonPressed> ()) {
                if (mouse_pressed->button == sf::Mouse::Button::Left)
                    scene.camera.StartLook (mouse_pressed->position.x, mouse_pressed->position.y);
            }
            else if (const auto* mouse_released = event->getIf<sf::Event::MouseButtonReleased> ()) {
                if (mouse_released->button == sf::Mouse::Button::Left)
                    scene.camera.StopLook ();
            }
            else if (const auto* mouse_moved = event->getIf<sf::Event::MouseMoved> ())
                scene.camera.Look (mouse_moved->position.x, mouse_moved->position.y);
        }

        float dt = clock.restart ().asSeconds ();
        scene.camera.Move (dt);

        scene.Draw ();
        window.display ();
    }

    return 0;
}
