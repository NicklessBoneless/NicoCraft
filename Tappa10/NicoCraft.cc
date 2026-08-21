#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>

#include "./Include/Player.hh"
#include "./Include/World.hh"
#include "./Include/Renderer.hh"
#include "./Include/Hotbar.hh"

const std::string dir = "../Tappa10/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa10";
const int TEXTUREPIXELSIZE = 32;


/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup{
public:
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::RenderWindow window; //Senza usare pointer con new

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
        settings.attributeFlags = sf::ContextSettings::Attribute::Default;
        settings.majorVersion = 4;
        settings.minorVersion = 1;
        return settings;
    }
};

////////////////////
// SFML Callbacks //
////////////////////

void Handle(const sf::Event::Resized& resized, fcg::Camera& camera, fcg::Hotbar& hotbar, sf::Vector2i& windowCenter){
    glViewport(0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize(resized.size.x, resized.size.y);
    hotbar.SetWindowSize(resized.size.x, resized.size.y);
    windowCenter = {(int)(resized.size.x / 2), (int)(resized.size.y / 2)};
}

////////////////////
// AUX Functions  //
////////////////////

void HandleEvents(sf::Window& window, fcg::Player& player, fcg::Hotbar& hotbar, sf::Vector2i& windowCenter, bool& programRunning){
    while(const std::optional event = window.pollEvent()){
        if(event->is<sf::Event::Closed>()){
            programRunning = false;
            return;
        }
        if(const auto* resized = event->getIf<sf::Event::Resized>()){
            Handle(*resized, player.getCamera(), hotbar, windowCenter);
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
                case sf::Keyboard::Scancode::Num1:
                    hotbar.SetSelected(0);
                    break;
                case sf::Keyboard::Scancode::Num2:
                    hotbar.SetSelected(1);
                    break;
                case sf::Keyboard::Scancode::Num3:
                    hotbar.SetSelected(2);
                    break;
                case sf::Keyboard::Scancode::Num4:
                    hotbar.SetSelected(3);
                    break;
                case sf::Keyboard::Scancode::Num5:
                    hotbar.SetSelected(4);
                    break;
                case sf::Keyboard::Scancode::Num6:
                    hotbar.SetSelected(5);
                    break;
                case sf::Keyboard::Scancode::Num7:
                    hotbar.SetSelected(6);
                    break;
                case sf::Keyboard::Scancode::Num8:
                    hotbar.SetSelected(6);
                    break;
                case sf::Keyboard::Scancode::Num9:
                    hotbar.SetSelected(6);
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
                    break;
            }
        }
        else if(const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()){
            if(mouseScrolled->wheel == sf::Mouse::Wheel::Vertical){
                //delta > 0 = rotellina verso l'alto: avanti di uno slot; delta < 0 = indietro
                int direction = mouseScrolled->delta > 0.0f ? 1 : -1;
                hotbar.ScrollSelected(direction);
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
    sf::RenderWindow& window = setup.window;

    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);
    sf::Vector2i windowCenter = {(int)(window.getSize().x / 2), (int)(window.getSize().y / 2)};

    fcg::Player player;
    player.getCamera().SetWindowSize(Setup::window_width, Setup::window_height);

    fcg::World world;

    fcg::Renderer renderer(
    {
        {"world",     dir + "shader_flat.vert",     dir + "shader_flat.frag"},
        {"crosshair", dir + "shader_crosshair.vert", dir + "shader_crosshair.frag"},
        {"outline",   dir + "shader_outline.vert",   dir + "shader_outline.frag"}
    },
    res,
    TEXTUREPIXELSIZE);

    fcg::Hotbar hotbar(res);
    hotbar.SetWindowSize(Setup::window_width, Setup::window_height);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    sf::Clock clock;
    bool programRunning = true;
    sf::Mouse::setPosition(windowCenter, window); //Sync forzato prima di fidarci del delta

    while(programRunning){
        HandleEvents(window, player, hotbar, windowCenter, programRunning);

        float deltaTime = clock.restart().asSeconds();

        PlayerInput currentInput = CapturePlayerInput();
        player.UpdatePosition(deltaTime, world, currentInput);

       
            UpdateMouseInput(window, player.getCamera(), windowCenter);
        
        
        

        fcg::RaycastHit target = world.RaycastBlock(
            player.getCamera().getPosition(),
            player.getCamera().GetForward(),
            player.getReach()
        );

        world.ProcessBlockInteractions(player, target, hotbar.GetSelectedBlockType());

        renderer.Draw(world, player.getCamera(), target,deltaTime);

        window.pushGLStates();
        hotbar.Draw(window);
        window.popGLStates();

        window.display();
    }

    return 0;
}
