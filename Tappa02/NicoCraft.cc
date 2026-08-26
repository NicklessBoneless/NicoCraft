#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include "./Include/Blocks.hh" //Messo per primo per dipendenze
#include "./Include/Matrices.hh"
#include "./Include/Hotshaders.hh"

const std::string dir = "../Tappa02/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa02";

/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup
{
public:
    //Width x Height 
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::Window* window;

    Setup ()
    {
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
    glm::mat4 pr;
    
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

    void ViewProjection (){
        float ncp = 0.1f;
        float fcp = 100.0f;

        glm::mat4 ry = fcg::rotation_y (phiDeg);
        glm::mat4 rx = fcg::rotation_x (thetaDeg);
        glm::mat4 t  = fcg::translation (-cameraPos.x, -cameraPos.y, -cameraPos.z);

        v = rx * ry * t; 

        float a = (fcp + ncp) / (ncp - fcp); //
        float b = 2.0f * fcp * ncp / (ncp - fcp); //

        pr = glm::mat4(
            fd,  0.0,     0.0,  0.0,
            0.0, fd * ar, 0.0,  0.0,
            0.0, 0.0,       a, -1.0,
            0.0, 0.0,       b,  0.0
        );
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
    GLuint ebo;
    Blocks::TextureArray textureArray;

public:
    GPUCube (const std::string& texture_path) { Load (texture_path); }
    ~GPUCube () { Clean (); }

    void Load (const std::string& texture_path)
    {
        float vertices[] = {
            // --- FACCIA FRONTALE (+Z) -> Layer 3 (Lato Erba) ---
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  1.0f,

            // --- FACCIA POSTERIORE (-Z) -> Layer 3 (Lato Erba) ---
             0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  2.0f,
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  2.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  2.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  2.0f,

            // --- FACCIA SINISTRA (-X) -> Layer 3 (Lato Erba) ---
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  2.0f,
            -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  2.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  2.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  2.0f,

            // --- FACCIA DESTRA (+X) -> Layer 3 (Lato Erba) ---
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  2.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  2.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  2.0f,
            0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  2.0f,

            // --- FACCIA SUPERIORE (+Y) -> Layer 1 (Erba Sopra) ---
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,

            // --- FACCIA INFERIORE (-Y) -> Layer 2 (Terra Sotto) ---
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f
        };

        //36 Indici per collegare i 24 vertici
        unsigned int indices[] = {
            0,  1,  2,  2,  3,  0, // Front
            4,  5,  6,  6,  7,  4, // Back
            8,  9, 10, 10, 11,  8, // Left
            12, 13, 14, 14, 15, 12, // Right
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20  // Bottom
        };

        std::vector<std::string> texture_paths = {
            texture_path+"missingTextureBlock.png",
            texture_path+"grassTop.png",    // Indice 1 -> Erba Sopra
            texture_path+"dirt.png",         // Indice 2 -> Terra
            texture_path+"grassSide.png",   // ... 3 -> Lato Erba
            texture_path+"stone.png"         // .. 4 -> Pietra
        };

        // 1. Genera e Binda il VAO prima di qualsiasi VBO o EBO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // 2. Genera e carica il VBO (dati dei vertici)
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // 3. Genera e carica l'EBO (indici)
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        GLsizei stride = 6 * sizeof(float);

        // Attribute 0: position (x, y, z)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);

        // Attribute 1: uv (u, v)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Attribute 2: layer (layer index della TextureArray)
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Unbind del VAO per sicurezza (L'EBO rimane salvato dentro al VAO)
        glBindVertexArray(0);

        // 4. Carica la TextureArray passandole il vettore con i percorsi dei file delle texture
        textureArray.LoadTextures(texture_paths, 32, 32);
    }

    void Clean ()
    {
        glDeleteVertexArrays (1, &vao);
        glDeleteBuffers (1, &vbo);
        glDeleteBuffers(1,&ebo);
    }

    void Draw()
    {
        glBindVertexArray(vao);
        
        //Bind del TextureArray e imposta la shader
        textureArray.Bind(0); 

        // Disegna 36 indici usando i triangoli
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
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
    GLint view_loc;
    GLint proj_loc;

public:
    Scene (fcg::Shaders& shaders) : cube (res) //Costruttore per il singolo cubo
    {
        Locations (shaders);
    }

    void Locations (fcg::Shaders& shaders)
    {
        model_loc = glGetUniformLocation (shaders.program, "model");
        view_loc  = glGetUniformLocation (shaders.program, "view");
        proj_loc  = glGetUniformLocation (shaders.program, "projection");

        GLint samplerLoc = glGetUniformLocation(shaders.program,"textureArray");
        glUniform1i(samplerLoc, 0);
    }

    void Draw ()
    {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Separiamo la matrice View dalla Projection (presenti nella classe Camera)
        glUniformMatrix4fv (proj_loc, 1, GL_FALSE, &camera.pr[0][0]); // Assicurati che pr sia pubblica in Camera
        glUniformMatrix4fv (view_loc, 1, GL_FALSE, &camera.v[0][0]);

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

    glEnable (GL_CULL_FACE); //[cite: 3]
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
