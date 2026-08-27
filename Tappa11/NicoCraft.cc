#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <memory>
#include <vector>

#include "./Include/Player.hh"
#include "./Include/World.hh"
#include "./Include/Renderer.hh"
#include "./Include/Hotbar.hh"
#include "./Include/MainMenu.hh"
#include "./Include/PauseMenu.hh"
#include "./Include/Settings.hh"

const std::string dir = "../Tappa11/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa11";
const int TEXTUREPIXELSIZE = 32;

//File in cui vengono salvate le preferenze (risoluzione, FOV): vive nella cartella da cui
//viene lanciato l'eseguibile (tipicamente build/), non va quindi consegnato ne' versionato
const std::string settingsPath = "nicocraft_settings.cfg";

//Stato di alto livello del programma
enum class GameState{ MainMenu, Playing, Paused };

////////////////////////////
// OpenGL context settings //
////////////////////////////

sf::ContextSettings CreateContextSettings(){
    sf::ContextSettings settings;
    settings.depthBits = 32;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.attributeFlags = sf::ContextSettings::Attribute::Default;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    return settings;
}

/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup{
public:
    sf::RenderWindow window; //Senza usare pointer con new

    //width/height arrivano dal file di preferenze (Settings.hh), letto prima di questa
    //classe: la finestra nasce gia' alla risoluzione scelta l'ultima volta dall'utente
    Setup(int width, int height) : window(sf::VideoMode({(unsigned int) width, (unsigned int) height}), winTitle, sf::Style::Default, sf::State::Windowed, CreateContextSettings()){
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

//Salva su file la terna corrente di preferenze: chiamata ad ogni modifica di FOV o
//risoluzione, sia dal menu principale che dalla pausa
void PersistSettings(float fov, int width, int height){
    fcg::Settings settings;
    settings.fov = fov;
    settings.width = width;
    settings.height = height;
    fcg::SaveSettings(settingsPath, settings);
}

//Eventi durante lo stato MainMenu: resize, Esc per uscire, click sinistro sui tasti del
//menu (Genera Mondo/Opzioni/Esci nella schermata principale, FOV/risoluzione/Indietro
//in quella Opzioni)
void HandleMenuEvents(sf::RenderWindow& window, fcg::MainMenu& mainMenu, GameState& state, bool& programRunning){
    while(const std::optional event = window.pollEvent()){
        if(event->is<sf::Event::Closed>()){
            programRunning = false;
            return;
        }

        if(const auto* resized = event->getIf<sf::Event::Resized>()){
            glViewport(0, 0, resized->size.x, resized->size.y);
            mainMenu.SetWindowSize(resized->size.x, resized->size.y);
            return;
        }

        if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
            if(keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                programRunning = false;
                return;
            }
        }
        else if(const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            if(mousePressed->button == sf::Mouse::Button::Left){
                fcg::MainMenu::MenuAction action = mainMenu.HandleClick(mousePressed->position);
                switch(action){
                    case fcg::MainMenu::MenuAction::GenerateWorld:
                        state = GameState::Playing;
                        return;
                    case fcg::MainMenu::MenuAction::Exit:
                        programRunning = false;
                        return;
                    case fcg::MainMenu::MenuAction::FovChanged:
                    case fcg::MainMenu::MenuAction::ResolutionChanged:
                        PersistSettings(mainMenu.GetFov(), mainMenu.GetResolutionWidth(), mainMenu.GetResolutionHeight());
                        break;
                    default:
                        break; //None: navigazione interna gia' gestita da HandleClick
                }
            }
        }
    }
}

//Eventi durante lo stato Paused: Esc riprende il gioco, click sui tasti dell'overlay.
//FOV viene applicato subito alla Camera per un'anteprima live, la risoluzione no
//(si applichera' al prossimo avvio, vedi Settings.hh)
void HandlePauseEvents(sf::RenderWindow& window, fcg::PauseMenu& pauseMenu, fcg::Player& player, GameState& state, bool& programRunning){
    while(const std::optional event = window.pollEvent()){
        if(event->is<sf::Event::Closed>()){
            programRunning = false;
            return;
        }

        if(const auto* resized = event->getIf<sf::Event::Resized>()){
            glViewport(0, 0, resized->size.x, resized->size.y);
            pauseMenu.SetWindowSize(resized->size.x, resized->size.y);
            return;
        }

        if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
            if(keyPressed->scancode == sf::Keyboard::Scancode::Escape){
                state = GameState::Playing; //Esc durante la pausa: riprendi
                return;
            }
        }
        else if(const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            if(mousePressed->button == sf::Mouse::Button::Left){
                fcg::PauseMenu::MenuAction action = pauseMenu.HandleClick(mousePressed->position);
                switch(action){
                    case fcg::PauseMenu::MenuAction::Resume:
                        state = GameState::Playing;
                        return;
                    case fcg::PauseMenu::MenuAction::BackToMainMenu:
                        state = GameState::MainMenu;
                        return;
                    case fcg::PauseMenu::MenuAction::QuitGame:
                        programRunning = false;
                        return;
                    case fcg::PauseMenu::MenuAction::FovChanged:
                        player.getCamera().SetFov(pauseMenu.GetFov());
                        PersistSettings(pauseMenu.GetFov(), pauseMenu.GetResolutionWidth(), pauseMenu.GetResolutionHeight());
                        break;
                    case fcg::PauseMenu::MenuAction::ResolutionChanged:
                        PersistSettings(pauseMenu.GetFov(), pauseMenu.GetResolutionWidth(), pauseMenu.GetResolutionHeight());
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

//Eventi durante lo stato Playing: identica alla logica di gioco gia' esistente, a parte
//Esc che ora apre la pausa invece di chiudere il programma
void HandleEvents(sf::Window& window, fcg::Player& player, fcg::Hotbar& hotbar, sf::Vector2i& windowCenter, GameState& state, bool& programRunning){
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
                    state = GameState::Paused;
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
                int direction = mouseScrolled->delta > 0.0f ? -1 : 1;
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
    fcg::Settings startupSettings = fcg::LoadSettings(settingsPath);

    Setup setup(startupSettings.width, startupSettings.height);
    sf::RenderWindow& window = setup.window;

    GameState state = GameState::MainMenu;

    std::unique_ptr<fcg::MainMenu> mainMenu = std::make_unique<fcg::MainMenu>(res, startupSettings.fov, startupSettings.width, startupSettings.height);
    mainMenu->SetWindowSize((int) window.getSize().x, (int) window.getSize().y);

    //Nel menu il cursore resta visibile e libero, per poter cliccare sui tasti
    window.setMouseCursorVisible(true);
    window.setMouseCursorGrabbed(false);

    //Player, Renderer, Hotbar, World e PauseMenu nascono tutti insieme, solo alla
    //pressione di "Genera Mondo": prima di quel momento non esiste nessuna risorsa di
    //gioco, e tornando al menu principale vengono distrutti (vedi ramo Paused->MainMenu),
    //cosi' il mondo smette letteralmente di essere renderizzato
    std::unique_ptr<fcg::Player> player;
    std::unique_ptr<fcg::Renderer> renderer;
    std::unique_ptr<fcg::Hotbar> hotbar;
    std::unique_ptr<fcg::World> world;
    std::unique_ptr<fcg::PauseMenu> pauseMenu;

    fcg::RaycastHit target; //Ultimo blocco puntato: resta "congelato" mentre si e' in pausa

    sf::Vector2i windowCenter = {(int)(window.getSize().x / 2), (int)(window.getSize().y / 2)};

    sf::Clock clock;
    bool programRunning = true;

    while(programRunning){
        if(state == GameState::MainMenu){
            HandleMenuEvents(window, *mainMenu, state, programRunning);
            if(!programRunning) break;

            if(state == GameState::Playing){
                //Transizione menu -> gioco: qui, e SOLO qui, nascono Player, Renderer,
                //Hotbar, World (chunk + mesh compresi) e PauseMenu
                player = std::make_unique<fcg::Player>();
                player->getCamera().SetWindowSize((int) window.getSize().x, (int) window.getSize().y);
                player->getCamera().SetFov(mainMenu->GetFov());

                renderer = std::make_unique<fcg::Renderer>(
                    std::vector<fcg::ShaderFiles>{
                        {"world",     dir + "shader_flat.vert",     dir + "shader_flat.frag"},
                        {"crosshair", dir + "shader_crosshair.vert", dir + "shader_crosshair.frag"},
                        {"outline",   dir + "shader_outline.vert",   dir + "shader_outline.frag"},
                    },
                    res,
                    TEXTUREPIXELSIZE,
                    dir);

                hotbar = std::make_unique<fcg::Hotbar>(res);
                hotbar->SetWindowSize((int) window.getSize().x, (int) window.getSize().y);

                world = std::make_unique<fcg::World>();

                pauseMenu = std::make_unique<fcg::PauseMenu>(res, mainMenu->GetFov(), mainMenu->GetResolutionWidth(), mainMenu->GetResolutionHeight());
                pauseMenu->SetWindowSize((int) window.getSize().x, (int) window.getSize().y);

                glEnable(GL_PROGRAM_POINT_SIZE); //Necessario per gl_PointSize nel vertex shader delle stelle
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glEnable(GL_DEPTH_TEST);

                window.setMouseCursorVisible(false);
                window.setMouseCursorGrabbed(true);
                windowCenter = {(int)(window.getSize().x / 2), (int)(window.getSize().y / 2)};
                sf::Mouse::setPosition(windowCenter, window);

                target = fcg::RaycastHit{};
                clock.restart(); //Evita un deltaTime enorme dovuto al tempo passato nel menu
                continue;
            }

            mainMenu->UpdateHover(sf::Mouse::getPosition(window));

            window.clear(sf::Color(18, 18, 26));
            window.pushGLStates();
            mainMenu->Draw(window);
            window.popGLStates();
            window.display();
            continue;
        }

        if(state == GameState::Paused){
            HandlePauseEvents(window, *pauseMenu, *player, state, programRunning);
            if(!programRunning) break;

            if(state == GameState::Playing){
                //Ripresa: riaggancia il mouse esattamente come all'ingresso in Playing
                window.setMouseCursorVisible(false);
                window.setMouseCursorGrabbed(true);
                windowCenter = {(int)(window.getSize().x / 2), (int)(window.getSize().y / 2)};
                sf::Mouse::setPosition(windowCenter, window);
                clock.restart();
                continue;
            }

            if(state == GameState::MainMenu){
                //Si torna al menu principale: distruggiamo tutto cio' che serviva solo
                //per giocare, World compreso. Da qui in poi non c'e' piu' nulla da
                //renderizzare finche' non si preme di nuovo "Genera Mondo"
                player.reset();
                renderer.reset();
                hotbar.reset();
                world.reset();
                pauseMenu.reset();

                window.setMouseCursorVisible(true);
                window.setMouseCursorGrabbed(false);

                //Ricostruita dalle preferenze salvate, cosi' riflette eventuali modifiche
                //fatte nel pannello Opzioni della pausa
                fcg::Settings currentSettings = fcg::LoadSettings(settingsPath);
                mainMenu = std::make_unique<fcg::MainMenu>(res, currentSettings.fov, currentSettings.width, currentSettings.height);
                mainMenu->SetWindowSize((int) window.getSize().x, (int) window.getSize().y);
                continue;
            }

            pauseMenu->UpdateHover(sf::Mouse::getPosition(window));

            clock.restart(); //Scarta il tempo passato in pausa: alla ripresa niente salti di deltaTime

            renderer->Draw(*world, player->getCamera(), target, 0.0f); //Mondo "congelato": il ciclo giorno/notte non avanza

            window.pushGLStates();
            hotbar->Draw(window);
            pauseMenu->Draw(window);
            window.popGLStates();

            window.display();
            continue;
        }

        //// state == GameState::Playing ////
        HandleEvents(window, *player, *hotbar, windowCenter, state, programRunning);
        if(!programRunning) break;

        if(state == GameState::Paused){
            //Esc appena premuto: apri l'overlay, libera il cursore, non processare input di gioco
            pauseMenu->Reset();
            window.setMouseCursorVisible(true);
            window.setMouseCursorGrabbed(false);
            continue;
        }

        float deltaTime = clock.restart().asSeconds();

        PlayerInput currentInput = CapturePlayerInput();
        player->UpdatePosition(deltaTime, *world, currentInput);

        UpdateMouseInput(window, player->getCamera(), windowCenter);

        target = world->RaycastBlock(
            player->getCamera().getPosition(),
            player->getCamera().GetForward(),
            player->getReach()
        );

        world->ProcessBlockInteractions(*player, target, hotbar->GetSelectedBlockType());

        renderer->Draw(*world, player->getCamera(), target, deltaTime);

        window.pushGLStates();
        hotbar->Draw(window);
        window.popGLStates();

        window.display();
    }

    return 0;
}
