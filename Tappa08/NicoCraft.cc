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
#include "PlayerPhysics.hh"

const std::string dir = "../Tappa08/";
const std::string res = "../Resources/";
const std::string winTitle = "NicoCraft - Tappa08";
const int TEXTUREPIXELSIZE = 32;
const int WORLDSIZECHUNKSX = 4;
const int WORLDSIZECHUNKSZ = 4;


/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup {
public:
    static const int window_width = 1920;
    static const int window_height = 1080;
    sf::Window window; //Senza usare pointer con new

    Setup() : window(sf::VideoMode({window_width, window_height}), winTitle, sf::Style::Default, sf::State::Windowed, createSettings()) {
        window.setVerticalSyncEnabled(true);

        if (!window.setActive(true)) {
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit(1);
        }

        int version = gladLoadGL(sf::Context::getFunction);
        if (!version) {
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit(1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    }

private:
    static sf::ContextSettings createSettings() {
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

struct PlayerInput {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool flyUp = false;   // Usato in NoClip
    bool flyDown = false; // Usato in NoClip
};

////////////////////
//    Camera      //
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

    const float mouseSensitivity = 0.15f;

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
        pitchDeg = pitchDeg > 89.9f ? 89.9f : pitchDeg;
        pitchDeg = pitchDeg < -89.9f ? -89.9f : pitchDeg;
        ViewProjection();
    }

    float GetAspectRatio() const{
        return aspectRatio;
    }

    glm::vec3 getPosition() const{
        return cameraPos;
    }

    //Impone direttamente la posizione della camera (usato quando e' il corpo fisico a comandare il movimento)
    void SetPosition(glm::vec3 position){
        cameraPos = position;
        ViewProjection();
    }

    glm::vec3 GetForward() const{
        float yawRad = glm::radians(yawDeg);
        float pitchRad = glm::radians(pitchDeg);
        return glm::vec3(
            glm::sin(yawRad) * glm::cos(pitchRad),
            -glm::sin(pitchRad),
            -glm::cos(yawRad) * glm::cos(pitchRad)
        );
    }

    //Direzione di movimento orizzontale (piano XZ) da WASD, in base allo yaw corrente.
    //Niente Space/LControl qui: quelli sono gestiti dalla fisica (salto) o dal volo libero (moveBindings)
    //Direzione di movimento orizzontale (piano XZ) da WASD, in base allo yaw corrente.
    //Usa SOLO le prime 4 celle del binding array (W,S,D,A): Space/LControl (se presenti,
    //per il noclip) vengono ignorati a prescindere, cosi' non c'e' rischio di introdurre
    //una componente Y "fantasma" quando questa funzione viene chiamata durante la camminata normale
    glm::vec3 computeHorizontalMovement(const PlayerInput& input, bool isNoClip) const {
        float yawRad = glm::radians(yawDeg);
        glm::vec3 forward = { glm::sin(yawRad), 0.0f, -glm::cos(yawRad) };
        glm::vec3 right   = { glm::cos(yawRad), 0.0f,  glm::sin(yawRad) };
        glm::vec3 up{0.0f,1.0f,0.0f};
        glm::vec3 moveDirection{0.0f};

        float x,y,z;
        x = y = z = 1.0f;

        z = input.moveForward - input.moveBackward;
        x = input.moveRight - input.moveLeft;
        y = isNoClip ? input.flyUp - input.flyDown : 0.0f;

        moveDirection += (forward * z) + (right * x) + (up * y);

        if(glm::length(moveDirection) > 0.0001f){
            moveDirection = glm::normalize(moveDirection);
        }

        return moveDirection;
    }

    glm::vec3 getHorizontalMovement(const PlayerInput& input) const{
        return computeHorizontalMovement(input,false);
    }

    //Movimento libero (NOCLIP): usato solo per debug, vola in ogni direzione senza collisioni
    void NoClipMove(float deltaTime, float speed,const PlayerInput& input){
        glm::vec3 moveDirection = computeHorizontalMovement(input,true);
        if (glm::length(moveDirection) < 0.0001f) return;

        cameraPos += moveDirection * speed * deltaTime;
        ViewProjection();
    }

    //Modifica il FOV di un delta (usato per il "kick" visivo durante lo sprint)
    void AdjustFov(float delta){
        fovDegrees += delta;
        ViewProjection();
    }

    glm::mat4 ViewProjection(){
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        glm::mat4 ry = fcg::rotation_y(yawDeg);
        glm::mat4 rx = fcg::rotation_x(pitchDeg);
        glm::mat4 t  = fcg::translation(-cameraPos.x, -cameraPos.y, -cameraPos.z);

        viewMatrix = rx * ry * t; 

        float perspectiveA = (farPlane + nearPlane) / (nearPlane - farPlane);
        float perspectiveB = 2.0f * farPlane * nearPlane / (nearPlane - farPlane);
        float focalDistance = 1.0f / glm::tan(glm::radians(fovDegrees / 2.0f));

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
//    Player      //
////////////////////
class Player {
private:
    Camera camera;
    fcg::PlayerPhysics physics;

    bool sprinting = false;
    bool noclip = false; 
    bool breakBlock = false; 
    bool placeBlock = false; 

    const float REACH_DISTANCE = 6.0f; 
    static constexpr float sprintFovKick = 0.5f;
    float moveSpeed = 4.0f;

public:
    Player() : physics({40.0f, 40.0f, 40.0f}) {
        camera.SetPosition(physics.GetEyePosition());
    }

    Camera& getCamera() { return camera; }
    float getReach() const { return REACH_DISTANCE; }

    void Look(float deltaX, float deltaY) {
        camera.Look(deltaX, deltaY);
    }

    void ToggleNoclip() {
        noclip = !noclip;
        if (!noclip) {
            glm::vec3 feetPosition = camera.getPosition() - glm::vec3(0.0f, fcg::PlayerPhysics::eyeHeight, 0.0f);
            physics.Teleport(feetPosition);
        }
    }

    void StartSprint(){
        if(!sprinting){
            moveSpeed *= 2;
            camera.AdjustFov(sprintFovKick);
            sprinting = true;
        }
    }

    void StopSprint(){
        if(sprinting){
            sprinting = false;
            moveSpeed /= 2;
            camera.AdjustFov(-sprintFovKick);
        }
    }

    void QueueBreakBlock(){
        breakBlock = true;
    }

    //Restituisce true se un break era stato richiesto, e resetta il flag
    //(va chiamato una sola volta per frame)
    bool ConsumeBreakBlock(){
        bool request = breakBlock;
        breakBlock = false;
        return request;
    }

    void QueuePlaceBlock(){
        placeBlock = true;
    }

    bool ConsumePlaceBlock(){
        bool request = placeBlock;
        placeBlock = false;
        return request;
    }

    bool IsPlayerOccupyingBlock(int worldX, int worldY, int worldZ) const {
        return physics.OccupiesBlock(worldX, worldY, worldZ);
    }

    void UpdatePosition(float deltaTime, fcg::IWorld& world, const PlayerInput& input) {
        noclip ? camera.NoClipMove(deltaTime, moveSpeed,input) : NormalMove(deltaTime, world,input);
    }

private:
    //Metodo helper che isola il polling di SFML e l'aggiornamento fisico
    void NormalMove(float deltaTime, fcg::IWorld& world,const PlayerInput& input) {
        if(input.jump) {
            physics.Jump();
        }

        glm::vec3 horizontalVelocity = camera.getHorizontalMovement(input) * moveSpeed;
        physics.UpdatePlayerPosition(deltaTime, world, horizontalVelocity);
        camera.SetPosition(physics.GetEyePosition());
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
    int hitFace = -1; //Faccia colpita del blocco (indice in Blocks::FACE_OFFSETS), -1 se hit=false
};

//Riempie un chunk con il terreno
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
            }
        }
    }
    
}

////////////////////
//     Scene      //
////////////////////

class Scene : public fcg::IWorld {
public:
    Player player;
    std::vector<ChunkInstance> chunks;
    Blocks::TextureArray textureArray;

private:
    GLint modelLoc, viewLoc, projLoc;

public:
    Scene(fcg::Shaders& shaders) {
        chunks.reserve(WORLDSIZECHUNKSX * WORLDSIZECHUNKSZ);
        
        GenerateAllChunks();
        BuildAllMeshes();
        InitializeTextures();
        Locations(shaders);
    }

    void Draw(fcg::Shaders& shaders)
    {
        shaders.use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &player.getCamera().projMatrix[0][0]);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &player.getCamera().viewMatrix[0][0]);

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

        int enteredFace = -1; //Faccia da cui si e' entrati nella cella corrente

            float traveled = 0.0f;
        while(traveled < maxDistance){
            if(IsSolidAtWorld(x, y, z)){
                result.hit = true;
                result.blockX = x;
                result.blockY = y;
                result.blockZ = z;
                result.hitFace = enteredFace;
                return result;
            }

            if(tMaxX < tMaxY && tMaxX < tMaxZ){
                x += stepX;
                traveled = tMaxX;
                tMaxX += tDeltaX;
                enteredFace = stepX > 0 ? 2 : 3; //Entrando in +X si tocca la faccia Left(2) del blocco; in -X la Right(3)
            }
            else if(tMaxY < tMaxZ){
                y += stepY;
                traveled = tMaxY;
                tMaxY += tDeltaY;
                enteredFace = stepY > 0 ? 5 : 4; //+Y -> Bottom(5), -Y -> Top(4)
            }
            else{
                z += stepZ;
                traveled = tMaxZ;
                tMaxZ += tDeltaZ;
                enteredFace = stepZ > 0 ? 1 : 0; //+Z -> Back(1), -Z -> Front(0)
            }
        }

        return result;
    }

    //Rompe (rimuove) il blocco alle coordinate MONDO indicate, aggiornando la mesh
    //del chunk modificato e, se necessario, quella dei chunk adiacenti
    void BreakBlockWorld(int worldX, int worldY, int worldZ){
        if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
            return;
        }

        int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
        int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

        ChunkInstance* instance = GetChunkInstanceAt(chunkX, chunkZ);
        if(!instance){
            return;
        }

        int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
        int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

        instance->chunk.Set(localX, worldY, localZ, Blocks::BlockType::AIR);
        RebuildChunkMesh(*instance);

        //Se il blocco era sul bordo del chunk, il vicino potrebbe aver nascosto
        //(o mostrato) una faccia in base al vecchio stato: va rigenerato anche lui
        if(localX == 0) RebuildNeighborIfExists(chunkX - 1, chunkZ);
        if(localX == Blocks::CHUNK_SIZE_X - 1) RebuildNeighborIfExists(chunkX + 1, chunkZ);
        if(localZ == 0) RebuildNeighborIfExists(chunkX, chunkZ - 1);
        if(localZ == Blocks::CHUNK_SIZE_Z - 1) RebuildNeighborIfExists(chunkX, chunkZ + 1);
    }

    //Piazza un blocco alle coordinate MONDO indicate (tipo fisso per ora), aggiornando la mesh
    //del chunk modificato e, se necessario, quella dei chunk adiacenti
    void PlaceBlockWorld(int worldX, int worldY, int worldZ, Blocks::BlockType type){
        if(worldY < 0 || worldY >= Blocks::CHUNK_SIZE_Y){
            return;
        }

        //Non si puo' piazzare un blocco dentro il volume occupato dal player
        if(player.IsPlayerOccupyingBlock(worldX, worldY, worldZ)){
            return;
        }

        int chunkX = FloorDiv(worldX, Blocks::CHUNK_SIZE_X);
        int chunkZ = FloorDiv(worldZ, Blocks::CHUNK_SIZE_Z);

        ChunkInstance* instance = GetChunkInstanceAt(chunkX, chunkZ);
        if(!instance){
            return;
        }

        int localX = worldX - chunkX * Blocks::CHUNK_SIZE_X;
        int localZ = worldZ - chunkZ * Blocks::CHUNK_SIZE_Z;

        //Sicurezza: non sovrascrivere un blocco gia' solido (non dovrebbe succedere, placeX/Y/Z e' gia' aria)
        if(instance->chunk.IsSolid(localX, worldY, localZ)){
            return;
        }

        instance->chunk.Set(localX, worldY, localZ, type);
        RebuildChunkMesh(*instance);

        if(localX == 0) RebuildNeighborIfExists(chunkX - 1, chunkZ);
        if(localX == Blocks::CHUNK_SIZE_X - 1) RebuildNeighborIfExists(chunkX + 1, chunkZ);
        if(localZ == 0) RebuildNeighborIfExists(chunkX, chunkZ - 1);
        if(localZ == Blocks::CHUNK_SIZE_Z - 1) RebuildNeighborIfExists(chunkX, chunkZ + 1);
    }

    //Controlla se e' solido il blocco a coordinate MONDO, individuando da solo il chunk giusto.
    //Override di fcg::IWorldQuery: e' cosi' che PlayerPhysics interroga il mondo senza conoscere Scene
    bool IsSolidAtWorld(int worldX, int worldY, int worldZ) override{
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

private:
    void GenerateAllChunks() {
        for (int chunkZ = 0; chunkZ < WORLDSIZECHUNKSZ; chunkZ++) {
            for (int chunkX = 0; chunkX < WORLDSIZECHUNKSX; chunkX++) {
                chunks.emplace_back();
                ChunkInstance& instance = chunks.back();
                instance.chunkX = chunkX;
                instance.chunkZ = chunkZ;
                fillExistingChunks(instance.chunk);
            }
        }
    }

    void BuildAllMeshes() {
        // La lambda è dentro RebuildChunkMesh e non si tocca!
        for (ChunkInstance& instance : chunks) {
            RebuildChunkMesh(instance); 
        }
    }

    void InitializeTextures() {
        std::vector<std::string> texturePaths = {
            res + "MissingTextureBlock.png", res + "grassTop.png",
            res + "dirt.png", res + "grassSide.png",
            res + "stone.png", res + "logTop.png",
            res + "logSide.png", res + "leaves.png" 
        };
        textureArray.LoadTextures(texturePaths, TEXTUREPIXELSIZE, TEXTUREPIXELSIZE);
    }

    void Locations(fcg::Shaders& shaders)
    {
        modelLoc = glGetUniformLocation(shaders.program, "model");
        viewLoc  = glGetUniformLocation(shaders.program, "view");
        projLoc  = glGetUniformLocation(shaders.program, "projection");

        GLint samplerLoc = glGetUniformLocation(shaders.program, "textureArray");
        glUniform1i(samplerLoc, 0);
    }

    void RebuildNeighborIfExists(int chunkX, int chunkZ){
        ChunkInstance* neighbor = GetChunkInstanceAt(chunkX, chunkZ);
        if(neighbor){
            RebuildChunkMesh(*neighbor);
        }
    }

    //Cerca il chunk alle coordinate di griglia indicate, nullptr se non esiste
    Blocks::Chunk* GetChunkAt(int chunkX, int chunkZ){
        for(auto& instance : chunks){
            if(instance.chunkX == chunkX && instance.chunkZ == chunkZ){
                return &instance.chunk;
            }
        }
        return nullptr;
    }

    //Cerca la ChunkInstance completa (chunk + mesh) alle coordinate di griglia indicate
    ChunkInstance* GetChunkInstanceAt(int chunkX, int chunkZ){
        for(auto& instance : chunks){
            if(instance.chunkX == chunkX && instance.chunkZ == chunkZ){
                return &instance;
            }
        }
        return nullptr;
    }

    //Rigenera la mesh di una ChunkInstance leggendo lo stato attuale del chunk
    void RebuildChunkMesh(ChunkInstance& instance){
        auto isSolidOutside = [this, &instance](int localX, int localY, int localZ){
            return IsSolidWorld(instance.chunkX, instance.chunkZ, localX, localY, localZ);
        };
        auto meshData = Blocks::BuildChunkMesh(instance.chunk, isSolidOutside);
        instance.mesh.Upload(meshData);
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
};


////////////////////
// Game  Bindings //
////////////////////



////////////////////
// SFML Callbacks //
////////////////////

void Handle(const sf::Event::Resized& resized, Camera& camera, sf::Vector2i& windowCenter){
    glViewport(0, 0, resized.size.x, resized.size.y);
    camera.SetWindowSize(resized.size.x, resized.size.y);
    windowCenter = { (int)(resized.size.x / 2), (int)(resized.size.y / 2) };
}

////////////////////
// AUX Functions  //
////////////////////
void HandleEvents(sf::Window& window, Player& player, sf::Vector2i& windowCenter, bool& programRunning) {
    while(const std::optional event = window.pollEvent()){
        if (event->is<sf::Event::Closed>()){
            programRunning = false;
            return;
        }
        if(const auto* resized = event->getIf<sf::Event::Resized>()){
            Handle(*resized, player.getCamera(), windowCenter);
            return;
        }
        
        if(const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
            switch (keyPressed->scancode) {
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
                    break; // Ignora gli altri tasti
            }
        }
        
        else if(const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()){
            if(keyReleased->scancode == sf::Keyboard::Scancode::LShift){
                player.StopSprint();
            }
        }
        
        else if(const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            switch(mousePressed->button) {
                case sf::Mouse::Button::Left:
                    player.QueueBreakBlock();
                    break;
                case sf::Mouse::Button::Right:
                    player.QueuePlaceBlock();
                    break;
                default:
                    break; // Ignora gli altri tasti del mouse
            }
        }
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

void ProcessBlockInteractions(Scene& scene, RaycastHit& target) {
    bool wantBreak = scene.player.ConsumeBreakBlock();
    bool wantPlace = scene.player.ConsumePlaceBlock();

    if(wantBreak && target.hit){
        scene.BreakBlockWorld(target.blockX, target.blockY, target.blockZ);
        target.hit = false;
    }
    else if(wantPlace && target.hit && target.hitFace >= 0){
        const auto& offset = Blocks::FACE_OFFSETS[target.hitFace];
        int placeX = target.blockX + offset[0];
        int placeY = target.blockY + offset[1];
        int placeZ = target.blockZ + offset[2];
        scene.PlaceBlockWorld(placeX, placeY, placeZ, Blocks::BlockType::STONE);
    }
}

PlayerInput CapturePlayerInput() {
    PlayerInput input;
    input.moveForward  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    input.moveBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    input.moveLeft     = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    input.moveRight    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    
    // Lo spazio e il control servono sia per il salto che per il volo libero
    input.jump         = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    input.flyUp        = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    input.flyDown      = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
    
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
    sf::Clock clock; //Utile per il deltaTime
    bool programRunning = true;
    
    while(programRunning){
        //Eventi standard (chiusura finestra, toggle, click singoli)
        HandleEvents(window, scene.player, windowCenter, programRunning);

        float deltaTime = clock.restart().asSeconds();

        //Cattura input di movimento (tick corrente)
        PlayerInput currentInput = CapturePlayerInput();

        //Aggiorniamo il player passando la struct
        scene.player.UpdatePosition(deltaTime, scene, currentInput);

        //Mouse Input
        UpdateMouseInput(window, scene.player.getCamera(), windowCenter);

        //Raycast (Guardiamo il Blocco?)
        RaycastHit target = scene.RaycastBlock(
            scene.player.getCamera().getPosition(), 
            scene.player.getCamera().GetForward(), 
            scene.player.getReach()
        );

        //Azioni sui blocchi
        ProcessBlockInteractions(scene, target);

        //Rendering
        scene.Draw(shaders);

        if(target.hit) { //Overlay blocco
            outline.Draw(target.blockX, target.blockY, target.blockZ, 
                         scene.player.getCamera().viewMatrix, 
                         scene.player.getCamera().projMatrix);
        }

        //Crosshair
        crosshair.Draw(scene.player.getCamera().GetAspectRatio());

        //Display finale!
        window.display();
    }

    return 0;
}
