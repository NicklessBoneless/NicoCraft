#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>

#include "./Include/Player.hh"
#include "./Include/World.hh"
#include "./Include/Renderer.hh"

const std::string dir = "../Tappa08/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa08";
const int TEXTUREPIXELSIZE = 32;


/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup{
public:
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::Window window; //Senza usare pointer con new

    Setup() : window(sf::VideoMode({window_width, window_height}), winTitle, sf::Style::Default, sf::State::Windowed, createSettings()){
        window.setVerticalSyncEnabled(true);

        if(!window.setActive(true)){
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit(1);
        }

        int version = gladLoadGL(sf::Context::getFunction);
        if(!version){
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit(1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    }

private:
    static sf::ContextSettings createSettings(){
        sf::ContextSettings settings;
        settings.depthBits = 32;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 4;
        settings.attributeFlags = sf::ContextSettings::Attribute::Core;
        settings.majorVersion = 4;
        settings.minorVersion = 1;
        return settings;
    }
};

////////////////////
// SFML Callbacks //
////////////////////

void Handle(const sf::Event::Resized& resized, fcg::Camera& camera, sf::Vector2i& windowCenter){
    glViewport(0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize(resized.size.x, resized.size.y);
    windowCenter = {(int)(resized.size.x / 2), (int)(resized.size.y / 2)};
}

////////////////////
// AUX Functions  //
////////////////////

void HandleEvents(sf::Window& window, fcg::Player& player, sf::Vector2i& windowCenter, bool& programRunning){
    while(const std::optional event = window.pollEvent()){
        if(event->is<sf::Event::Closed>()){
            programRunning = false;
            return;
        }
        if(const auto* resized = event->getIf<sf::Event::Resized>()){
            Handle(*resized, player.getCamera(), windowCenter);
            return;
        }

        if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
            switch(keyPressed->scancode){
                case sf::Keyboard::Scancode::Escape:
                    programRunning = false;
                    return;
                case sf::Keyboard::Scancode::F:
                    player.ToggleNoclip();
                    break;
                case sf::Keyboard::Scancode::LShift:
                    player.StartSprint();
                    break;
                default:
                    break; //Ignora gli altri tasti
            }
        }

        else if(const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()){
            if(keyReleased->scancode == sf::Keyboard::Scancode::LShift){
                player.StopSprint();
            }
        }

        else if(const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            switch(mousePressed->button){
                case sf::Mouse::Button::Left:
                    player.QueueBreakBlock();
                    break;
                case sf::Mouse::Button::Right:
                    player.QueuePlaceBlock();
                    break;
                default:
                    break; //Ignora gli altri tasti del mouse
            }
        }
    }
}

void UpdateMouseInput(sf::Window& window, fcg::Camera& camera, const sf::Vector2i& windowCenter){
    if(window.hasFocus()){
        sf::Vector2i curMousePos = sf::Mouse::getPosition(window);
        sf::Vector2i delta = curMousePos - windowCenter;

        camera.Look((float) delta.x, (float) delta.y);
        sf::Mouse::setPosition(windowCenter, window);
    }
}

PlayerInput CapturePlayerInput(){
    PlayerInput input;
    input.moveForward  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    input.moveBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    input.moveLeft     = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    input.moveRight     = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    //Lo spazio e il control servono sia per il salto che per il volo libero
    input.jump          = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    input.flyUp         = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    input.flyDown       = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);

    return input;
}

//////////
// Main //
//////////

int main(){
    //// Startup ////
    Setup setup;
    sf::Window& window = setup.window;

    //Prendiamo e centriamo il cursore per la camera FPS
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);
    sf::Vector2i windowCenter = {(int)(window.getSize().x / 2), (int)(window.getSize().y / 2)};
    sf::Mouse::setPosition(windowCenter, window);

    //Player e camera
    fcg::Player player;
    player.getCamera().SetWindowSize(Setup::window_width, Setup::window_height);

    //Mondo di gioco (simulazione: chunk, editing, raycast)
    fcg::World world;

    //Renderer (shader, texture, crosshair, outline: tutto il disegno vive qui)
    fcg::Renderer renderer(
    {
        {"world",     dir + "shader_flat.vert",     dir + "shader_flat.frag"},
        {"crosshair", dir + "shader_crosshair.vert", dir + "shader_crosshair.frag"},
        {"outline",   dir + "shader_outline.vert",   dir + "shader_outline.frag"}
    },
    {
        res + "missingTextureBlock.png", res + "grassTop.png",
        res + "dirt.png", res + "grassSide.png",
        res + "stone.png", res + "logTop.png",
        res + "logSide.png", res + "leaves.png"
    },
    TEXTUREPIXELSIZE);

    //Per migliorare la performance ;-)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    //// Main Loop ////
    sf::Clock clock; //Utile per il deltaTime
    bool programRunning = true;

    while(programRunning){
        //Eventi standard (chiusura finestra, toggle, click singoli)
        HandleEvents(window, player, windowCenter, programRunning);

        float deltaTime = clock.restart().asSeconds();

        //Cattura input di movimento (tick corrente)
        PlayerInput currentInput = CapturePlayerInput();

        //Aggiorniamo il player (camera + fisica)
        player.UpdatePosition(deltaTime, world, currentInput);

        //Mouse Input
        UpdateMouseInput(window, player.getCamera(), windowCenter);

        //Raycast (Guardiamo il Blocco?)
        fcg::RaycastHit target = world.RaycastBlock(
            player.getCamera().getPosition(),
            player.getCamera().GetForward(),
            player.getReach()
        );

        //Azioni sui blocchi (break/place)
        world.ProcessBlockInteractions(player, target,Blocks::BlockType::STONE);

        //Rendering (mondo + outline + crosshair, tutto in un unico punto)
        renderer.Draw(world, player.getCamera(), target);

        //Display finale!
        window.display();
    }

    return 0;
}
