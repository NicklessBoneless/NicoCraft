#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <iostream>
#include <cstdlib>

#include "matrices.hh"
#include "hotshaders.hh"

// Percorso relativo alle risorse (shader), valido se l'eseguibile viene
// lanciato dalla cartella di build generata da CMake.
const std::string dir = "../Tappa01/";


/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup
{
public:
    static const int window_width = 800;
    static const int window_height = 800;

    sf::Window* window;

    Setup ()
    {
        sf::ContextSettings settings;
        settings.depthBits = 32;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Attribute::Core;
        settings.majorVersion = 4;
        settings.minorVersion = 1;

        window = new sf::Window (
                                 sf::VideoMode({window_width, window_height}),
                                 "NicoCraft - Tappa01",
                                 sf::Style::Default,
                                 sf::State::Windowed,
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
// Lights          //
////////////////////

class Lights
{
public:
    glm::vec3 light_direct_pos_relative = {1.0, 1.0, 1.0}; // xyz (relativa alla camera)
    glm::vec3 light_direct_pos = {0.0, 0.0, 0.0};           // xyz (assoluta, world space)
    glm::vec3 light_direct_val = {1.0, 1.0, 1.0};   // rgb
    glm::vec3 light_ambient_val = {0.2, 0.2, 0.2};  // rgb
    glm::vec3 material_diffuse = {0.8, 0.7, 0.6};   // rgb
    glm::vec3 material_ambient = {0.5, 0.5, 0.8};   // rgb
    glm::vec3 material_specular = {1.0, 1.0, 1.0};  // rgb
    float material_shininess = 1000.0;

private:
    GLint light_direct_pos_loc;
    GLint light_direct_val_loc;
    GLint light_ambient_val_loc;
    GLint material_diffuse_loc;
    GLint material_ambient_loc;
    GLint material_specular_loc;
    GLint material_shininess_loc;

public:
    Lights (fcg::Shaders& shaders)
    {
        locations (shaders);
    }

    void locations (fcg::Shaders& shaders)
    {
        light_direct_pos_loc = glGetUniformLocation (shaders.program, "light.direct_pos");
        light_direct_val_loc = glGetUniformLocation (shaders.program, "light.direct_val");
        light_ambient_val_loc = glGetUniformLocation (shaders.program, "light.ambient_val");
        material_diffuse_loc = glGetUniformLocation (shaders.program, "material.diffuse");
        material_ambient_loc = glGetUniformLocation (shaders.program, "material.ambient");
        material_specular_loc = glGetUniformLocation (shaders.program, "material.specular");
        material_shininess_loc = glGetUniformLocation (shaders.program, "material.shininess");
    }

    void send_parameters ()
    {
        glUniform3fv (light_direct_val_loc, 1, &light_direct_val[0]);
        glUniform3fv (light_ambient_val_loc, 1, &light_ambient_val[0]);
        glUniform3fv (material_diffuse_loc, 1, &material_diffuse[0]);
        glUniform3fv (material_ambient_loc, 1, &material_ambient[0]);
        glUniform3fv (material_specular_loc, 1, &material_specular[0]);
        glUniform1fv (material_shininess_loc, 1, &material_shininess);
    }

    void send_position_relative (const glm::mat4& inverse_view_matrix)
    {
        glm::vec4 p = glm::vec4 (light_direct_pos_relative, 1.0);
        p = inverse_view_matrix * p;
        light_direct_pos = {p.x, p.y, p.z};
        send_position ();
    }

    void send_position ()
    {
        glUniform3fv (light_direct_pos_loc, 1, &light_direct_pos[0]);
    }
};



////////////////////
// Camera          //
////////////////////

class Camera
{
public:
    glm::mat4 v;
    glm::mat4 inv_v;
    glm::mat4 vp;

private:
    /** Parametri intrinseci **/
    float fd = 50.0f / 18.0f; // focal distance (lente "normale")
    float ar = 1.0f;          // aspect ratio

    /** Parametri estrinseci **/
    glm::vec3 camera_pos = {0.0f, 0.0f, 2.5f};
    GLint camera_pos_loc;
    float phi_deg = 0.0f;    // yaw (rotazione attorno a Y)
    float theta_deg = 0.0f;  // pitch (rotazione attorno a X)

    /** Look-around col mouse (tenuto premuto) **/
    bool looking = false;
    float last_mouse_x = 0.0f;
    float last_mouse_y = 0.0f;
    const float mouse_sensitivity = 0.15f;

    /** Movimento WASD **/
    const float move_speed = 2.0f; // unità al secondo

public:
    Camera (fcg::Shaders& shaders)
    {
        locations (shaders);
        set_window_size (Setup::window_width, Setup::window_height);
        view_projection ();
    }

    void locations (fcg::Shaders& shaders)
    {
        camera_pos_loc = glGetUniformLocation (shaders.program, "camera_pos");
    }

    void set_window_size (int w, int h)
    {
        ar = ((float) w) / (float) h;
        view_projection ();
    }

    // da chiamare alla pressione del tasto sinistro del mouse
    void start_look (float x, float y)
    {
        looking = true;
        last_mouse_x = x;
        last_mouse_y = y;
    }

    // da chiamare al rilascio del tasto sinistro del mouse
    void stop_look ()
    {
        looking = false;
    }

    // da chiamare ad ogni evento di movimento del mouse
    void look (float x, float y)
    {
        if (!looking)
            return;

        float dx = x - last_mouse_x;
        float dy = y - last_mouse_y;
        last_mouse_x = x;
        last_mouse_y = y;

        phi_deg += dx * mouse_sensitivity;
        theta_deg += dy * mouse_sensitivity;
        theta_deg = theta_deg > 89.0f ? 89.0f : theta_deg;
        theta_deg = theta_deg < -89.0f ? -89.0f : theta_deg;

        view_projection ();
    }

    // da chiamare una volta per frame con il delta time in secondi
    void move (float dt)
    {
        bool w = sf::Keyboard::isKeyPressed (sf::Keyboard::Key::W);
        bool s = sf::Keyboard::isKeyPressed (sf::Keyboard::Key::S);
        bool a = sf::Keyboard::isKeyPressed (sf::Keyboard::Key::A);
        bool d = sf::Keyboard::isKeyPressed (sf::Keyboard::Key::D);

        if (!w && !s && !a && !d)
            return;

        float phi_rad = glm::radians (phi_deg);

        // movimento sul piano orizzontale: il pitch non influisce (stile Minecraft)
        glm::vec3 forward = { glm::sin (phi_rad), 0.0f, -glm::cos (phi_rad) };
        glm::vec3 right   = { glm::cos (phi_rad), 0.0f,  glm::sin (phi_rad) };

        glm::vec3 move_dir = {0.0f, 0.0f, 0.0f};
        if (w) move_dir += forward;
        if (s) move_dir -= forward;
        if (d) move_dir += right;
        if (a) move_dir -= right;

        if (glm::length (move_dir) > 0.0001f)
            move_dir = glm::normalize (move_dir);

        camera_pos += move_dir * move_speed * dt;

        view_projection ();
    }

    void view_projection ()
    {
        float ncp = 0.1f;
        float fcp = 100.0f;

        glm::mat4 ry = fcg::rotation_y (phi_deg);
        glm::mat4 rx = fcg::rotation_x (theta_deg);
        glm::mat4 t  = fcg::translation (-camera_pos.x, -camera_pos.y, -camera_pos.z);

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
        inv_v = glm::inverse (v);

        glUniform3fv (camera_pos_loc, 1, &camera_pos[0]);
    }
};



////////////////////
// Cube (hard-coded, no mesh loading) //
////////////////////

class GPUCube
{
private:
    GLuint vbo;
    GLuint vao;

public:
    GPUCube () { load (); }
    ~GPUCube () { clean (); }

    void load ()
    {
        // 6 facce * 2 triangoli * 3 vertici = 36 vertici.
        // ogni vertice: posizione (3 float) + normale di faccia (3 float)
        static const float vertices[] = {
            // Front (+Z)  normal (0,0,1)
            -0.5f,-0.5f, 0.5f,  0,0,1,
             0.5f,-0.5f, 0.5f,  0,0,1,
             0.5f, 0.5f, 0.5f,  0,0,1,
             0.5f, 0.5f, 0.5f,  0,0,1,
            -0.5f, 0.5f, 0.5f,  0,0,1,
            -0.5f,-0.5f, 0.5f,  0,0,1,

            // Back (-Z)  normal (0,0,-1)
             0.5f,-0.5f,-0.5f,  0,0,-1,
            -0.5f,-0.5f,-0.5f,  0,0,-1,
            -0.5f, 0.5f,-0.5f,  0,0,-1,
            -0.5f, 0.5f,-0.5f,  0,0,-1,
             0.5f, 0.5f,-0.5f,  0,0,-1,
             0.5f,-0.5f,-0.5f,  0,0,-1,

            // Left (-X)  normal (-1,0,0)
            -0.5f,-0.5f,-0.5f, -1,0,0,
            -0.5f,-0.5f, 0.5f, -1,0,0,
            -0.5f, 0.5f, 0.5f, -1,0,0,
            -0.5f, 0.5f, 0.5f, -1,0,0,
            -0.5f, 0.5f,-0.5f, -1,0,0,
            -0.5f,-0.5f,-0.5f, -1,0,0,

            // Right (+X)  normal (1,0,0)
             0.5f,-0.5f, 0.5f,  1,0,0,
             0.5f,-0.5f,-0.5f,  1,0,0,
             0.5f, 0.5f,-0.5f,  1,0,0,
             0.5f, 0.5f,-0.5f,  1,0,0,
             0.5f, 0.5f, 0.5f,  1,0,0,
             0.5f,-0.5f, 0.5f,  1,0,0,

            // Top (+Y)  normal (0,1,0)
            -0.5f, 0.5f, 0.5f,  0,1,0,
             0.5f, 0.5f, 0.5f,  0,1,0,
             0.5f, 0.5f,-0.5f,  0,1,0,
             0.5f, 0.5f,-0.5f,  0,1,0,
            -0.5f, 0.5f,-0.5f,  0,1,0,
            -0.5f, 0.5f, 0.5f,  0,1,0,

            // Bottom (-Y)  normal (0,-1,0)
            -0.5f,-0.5f,-0.5f,  0,-1,0,
             0.5f,-0.5f,-0.5f,  0,-1,0,
             0.5f,-0.5f, 0.5f,  0,-1,0,
             0.5f,-0.5f, 0.5f,  0,-1,0,
            -0.5f,-0.5f, 0.5f,  0,-1,0,
            -0.5f,-0.5f,-0.5f,  0,-1,0,
        };

        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

        glGenVertexArrays (1, &vao);
        glBindVertexArray (vao);

        // Attribute 0: position (x, y, z)
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof (float), (void*) 0);
        glEnableVertexAttribArray (0);

        // Attribute 1: normal (x, y, z)
        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof (float), (void*) (3 * sizeof (float)));
        glEnableVertexAttribArray (1);
    }

    void clean ()
    {
        glDeleteVertexArrays (1, &vao);
        glDeleteBuffers (1, &vbo);
    }

    void draw ()
    {
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
    Lights lights;
    GPUCube cube;

private:
    GLint model_loc;
    GLint vp_loc;
    GLint tr_inv_model_loc;

public:
    Scene (fcg::Shaders& shaders) : camera (shaders), lights (shaders)
    {
        locations (shaders);
        update_all ();
    }

    void locations (fcg::Shaders& shaders)
    {
        camera.locations (shaders);
        lights.locations (shaders);

        model_loc = glGetUniformLocation (shaders.program, "model");
        vp_loc = glGetUniformLocation (shaders.program, "vp");
        tr_inv_model_loc = glGetUniformLocation (shaders.program, "tr_inv_model");
    }

    void update_all ()
    {
        camera.view_projection ();
        lights.send_parameters ();
        lights.send_position_relative (camera.inv_v);
    }

    void draw ()
    {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv (vp_loc, 1, GL_FALSE, &camera.vp[0][0]);

        // il cubo sta fermo nell'origine, model matrix identità
        glm::mat4 model = fcg::identity ();
        glm::mat3 tr_inv_model = fcg::identity3 ();
        glUniformMatrix4fv (model_loc, 1, GL_FALSE, &model[0][0]);
        glUniformMatrix3fv (tr_inv_model_loc, 1, GL_FALSE, &tr_inv_model[0][0]);

        cube.draw ();
    }
};



////////////////////
// SFML Callbacks //
////////////////////

void handle (const sf::Event::Resized& resized, Camera& camera)
{
    glViewport (0, 0, resized.size.x, resized.size.y);
    camera.set_window_size (resized.size.x, resized.size.y);
}

void handle (const sf::Event::KeyPressed& key, fcg::Shaders& shaders, Scene& scene)
{
    switch (key.scancode) {
    case sf::Keyboard::Scancode::Escape:
        exit (0);
    case sf::Keyboard::Scancode::G:
        shaders.reload (dir + "shader_gouraud.vert", dir + "shader_gouraud.frag");
        shaders.use ();
        scene.locations (shaders);
        scene.update_all ();
        return;
    case sf::Keyboard::Scancode::P:
        shaders.reload (dir + "shader_phong.vert", dir + "shader_phong.frag");
        shaders.use ();
        scene.locations (shaders);
        scene.update_all ();
        return;
    case sf::Keyboard::Scancode::F:
        shaders.reload (dir + "shader_flat.vert", dir + "shader_flat.frag");
        shaders.use ();
        scene.locations (shaders);
        scene.update_all ();
        return;
    case sf::Keyboard::Scancode::C:
        shaders.reload (dir + "shader_normals.vert", dir + "shader_normals.frag");
        shaders.use ();
        scene.locations (shaders);
        scene.update_all ();
        return;
    default:
        return;
    }
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

    sf::Clock clock;
    bool running = true;
    while (running)
    {
        while (const std::optional event = window.pollEvent ())
        {
            if (event->is<sf::Event::Closed> ())
                running = false;
            else if (const auto* resized = event->getIf<sf::Event::Resized> ())
                handle (*resized, scene.camera);
            else if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed> ())
                handle (*key_pressed, shaders, scene);
            else if (const auto* mouse_pressed = event->getIf<sf::Event::MouseButtonPressed> ()) {
                if (mouse_pressed->button == sf::Mouse::Button::Left)
                    scene.camera.start_look (mouse_pressed->position.x, mouse_pressed->position.y);
            }
            else if (const auto* mouse_released = event->getIf<sf::Event::MouseButtonReleased> ()) {
                if (mouse_released->button == sf::Mouse::Button::Left)
                    scene.camera.stop_look ();
            }
            else if (const auto* mouse_moved = event->getIf<sf::Event::MouseMoved> ())
                scene.camera.look (mouse_moved->position.x, mouse_moved->position.y);
        }

        float dt = clock.restart ().asSeconds ();
        scene.camera.move (dt);

        scene.draw ();
        window.display ();
    }

    return 0;
}