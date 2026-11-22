#ifndef TERRAIN_GENERATOR_HH
#define TERRAIN_GENERATOR_HH

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <utility>
#include <vector>
#include <array>
 
#include "Chunk.hh"
#include "Noise.hh"
#include "TerrainConfig.hh"

namespace fcg{
    /*
        Genera il contenuto di un singolo chunk a partire da un seed esplicito.
        Non conosce World, mesh o rendering: riceve un Chunk e lo riempie.
        Ogni chunk usa un generatore casuale derivato da (seed, chunkX, chunkZ):
        il risultato non dipende dall'ordine in cui i chunk vengono generati
    */
    class TerrainGenerator{
    private:
        uint32_t seed;
        std::unique_ptr<Noise> noise;
        FractalParams terrainParameters;
        CavesConfig cavesConfig;
        float heightAmplitude;

        //HeightMap per la generazione del mondo (salviamo le altezze)
        //Array di grandezza Chunk_Size_x * Chunk_size_z, HeightMap è un alias
        using HeightMap = std::array<int, Blocks::CHUNK_SIZE_X * Blocks::CHUNK_SIZE_Z>;
        

        static constexpr int baseHeight = Blocks::CHUNK_SIZE_Y / 4; //Quota media del terreno
        static constexpr int dirtDepth = 5; //Strati di terra sotto l'erba
        static constexpr int wormGridSize = 4; //Passo (blocchi) della griglia di campionamento dei worm, SEMPRE DIVISORE DI 16
        static constexpr int spongeGridSize = 2; //Passo (blocchi) della griglia di campionamento della spugna
        static constexpr int treeRarity = 400;
        static constexpr int minTreeDistance = 3; //Distanza minima (in blocchi) tra due tronchi
        static constexpr int treeTrunkHeight = 5;
        static constexpr int treeClearance = treeTrunkHeight + 2; //Spazio da lasciare sopra il terreno per la chioma
        
        
        //Ogni campo ha il suo offset (in blocchi), cosi' sacche e tunnel non si sovrappongono sempre negli stessi punti
        static constexpr float spongeOffsetX = 251.3f;
        static constexpr float spongeOffsetY = 97.1f;
        static constexpr float spongeOffsetZ = 173.9f;

        static constexpr float wormAOffsetX = -181.7f;
        static constexpr float wormAOffsetY = 43.9f;
        static constexpr float wormAOffsetZ = 307.3f;

        static constexpr float wormBOffsetX = 131.7f;
        static constexpr float wormBOffsetY = 57.3f;
        static constexpr float wormBOffsetZ = 211.1f;

        static constexpr float spaghettiPathOffsetX = 1733.1f;
        static constexpr float spaghettiPathOffsetZ = -2291.7f;
        static constexpr float spaghettiElevationOffsetX = -3187.3f;
        static constexpr float spaghettiElevationOffsetZ = 1459.9f;
    
    public:
        TerrainGenerator(uint32_t seed, std::unique_ptr<Noise> noise, FractalParams terrainParameters = FractalParams{},CavesConfig cavesConfig = CavesConfig{},float heightAmplitude = 30.0f) : 
        seed(seed), 
        noise(std::move(noise)), 
        terrainParameters(terrainParameters),
        cavesConfig(cavesConfig),
        heightAmplitude(heightAmplitude){}

        uint32_t GetSeed() const{
            return seed;
        }

        /*
            Quota Y del primo blocco libero sopra il terreno, in coordinate MONDO.
            Il rumore e' campionato in coordinate mondo (non locali al chunk):
            cosi' non ci sono cuciture ai bordi tra chunk adiacenti
        */
        int GetSurfaceHeight(int worldX, int worldZ) const{
            float MediumElevationValue = Fractal2D(*noise, (float) worldX, (float) worldZ, terrainParameters); //n = [-1,1]
            int height = baseHeight + (int) std::floor(MediumElevationValue * heightAmplitude);
            //Clamp serve per far rimanere il valore dell'altezza (height) tra [2 - Chunk_Size_Y]
            return std::clamp(height, 2, Blocks::CHUNK_SIZE_Y); 
        }     

        void GenerateChunk(Blocks::Chunk& chunk, int chunkX, int chunkZ) const{
            //Costruiamo la heightmap per questo chunk tramite perlin2D
            HeightMap heightMap = GenerateHeightMapForChunkXZ(chunkX, chunkZ);

            //Riempe il chunk di blocchi
            FillTerrain(chunk, heightMap);

            //Crea le caverne nel chumk
            GenerateCaves(chunk, chunkX, chunkZ, heightMap); 

            //Aggiunge gli alberi al chunk
            GenerateTrees(chunk, chunkX, chunkZ, heightMap);
        }

    private:
        //Divide e arrotonda per eccesso
        static int divideAndRoundUp(int dividend, int divisor){
            return (dividend + (divisor-1)) / divisor;
        }

        //Calcola l'indice per la heightmap del chunk
        static int HeightIndex(int x, int z){
            return x + Blocks::CHUNK_SIZE_X * z;
        }

        //Calcola l'indice per la maschera worm del chunk: un byte per cella (x, y, z locali)
        static int CarveIndex(int x, int y, int z){
            return (y * Blocks::CHUNK_SIZE_Z + z) * Blocks::CHUNK_SIZE_X + x;
        }

        //Calcola l'indice della griglia di grandezza gridStepSize
        static int GridIndex(int i, int j, int k, int gridStepsSize){
            return (j * gridStepsSize + k) * gridStepsSize + i;
        }

        //Interpolazione trilineare degli 8 angoli (ordine: indice = dx + 2*dy + 4*dz)
        static float Trilinear(const float* v, float fx, float fy, float fz){
            float c00 = v[0] + fx * (v[1] - v[0]);
            float c10 = v[2] + fx * (v[3] - v[2]);
            float c01 = v[4] + fx * (v[5] - v[4]);
            float c11 = v[6] + fx * (v[7] - v[6]);
            float c0 = c00 + fy * (c10 - c00);
            float c1 = c01 + fy * (c11 - c01);
            return c0 + fz * (c1 - c0);
        }

        /*
            Controlla se tutti gli angoli sono fuori dalla threshold
            TRUE se tutti gli angoli sono fuori da [-threshold, threshold] dallo stesso lato.
            FALSE se almeno uno degli angoli è all'interno della threshold
        */
        static bool AllCornersOutside(const float* v, float threshold){
            bool allAbove = true;
            bool allBelow = true;
            for(int i = 0; i < 8; i++){
                if(v[i] <= threshold) allAbove = false;
                if(v[i] >= -threshold) allBelow = false;
            }
            return allAbove || allBelow;
        }

        /*
            Costruisce una heightMap tramite Perlin Noise 2D
            ChunkX,Z specificano il chunk si cui si va a operare con il perlin
            servono a converitire in coordinate mondo le coordinate locali
        */
        HeightMap GenerateHeightMapForChunkXZ(int chunkX, int chunkZ) const{
            HeightMap heightMap;
            for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                    heightMap[HeightIndex(x, z)] = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                }
            }
            return heightMap;
        }

        /*
            Riempe il chunk da [0-Surface] con i blocchi prestabiliti.
            La surface dipende dal valore della heightmap calcolata con Perlin Noise 2D
        */
        void FillTerrain(Blocks::Chunk& chunk, const HeightMap& heightMap) const{
            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    int surface = heightMap[HeightIndex(x,z)];
                    chunk.Set(x, 0, z, Blocks::BlockType::BEDROCK);
                    for(int y = 1; y < surface; y++){
                        if(y == surface - 1) chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                        else if(y >= (surface-1)-dirtDepth) chunk.Set(x, y, z, Blocks::BlockType::DIRT);
                        else chunk.Set(x, y, z, Blocks::BlockType::STONE);
                    }
                }
            }
        }

        /*
            Individua i blocchi da rimuovere e rimpiazzare con aria seguento l'algoritmo delle sacche (spugna) del chunk. 
            Il perlin 3D si valuta solo sui punti di una griglia (spongeGridSize) e poi si interpola; 
            la soglia, che dipende da y e dalla superficie della colonna, si confronta con ogni singolo blocco.
        */
        void CarveSpongeCave(int chunkX , int chunkZ, int maximumHeightY, const HeightMap& heightMap, std::vector<uint8_t>& carveMap) const{
            const int minHeight = cavesConfig.minY;
            if(minHeight > maximumHeightY) return;

            const int actualWorkingHeight = (maximumHeightY - minHeight);
            const int numberOfCellsY = divideAndRoundUp(actualWorkingHeight,spongeGridSize);
            if(numberOfCellsY <= 0) return;

            //Al posto di andare a calcolare 16^2 punti di perlin noise, ne calcoliamo 9^2, con spongeGridSize = 2
            const int numberOfCellsXZ  = Blocks::CHUNK_SIZE_X / spongeGridSize;
            const int numberOfPointsXZ = numberOfCellsXZ + 1;
            const int numberOfPointsY  = numberOfCellsY  + 1;

            /*
                Riempiamo il vettore con i valori di perlin noise 3D
                Campioniamo (numeroPunti^2 * Height).
                Quindi avremo (256 * Height), con la griglia 1 blocco = 1 cella
                Invece avremo (81 * Height/2 + 1), con la griglia 1 blocco = 1/4 cella (2x2x2)
            */
            std::vector<float> perlinNoiseValues(numberOfPointsXZ * numberOfPointsXZ * numberOfPointsY);
            for(int y = 0; y < numberOfPointsY; y++){
                for(int z = 0; z < numberOfPointsXZ; z++){
                    for(int x = 0; x < numberOfPointsXZ; x++){
                        //Campioniamo Perlin Noise 3D sui 9 punti del chunk  
                        float worldX = (float) (chunkX * Blocks::CHUNK_SIZE_X + x * spongeGridSize);
                        float worldY = (float) (minHeight + y * spongeGridSize);
                        float worldZ = (float) (chunkZ * Blocks::CHUNK_SIZE_Z + z * spongeGridSize);
                        float height3D = Fractal3D(*noise,worldX + spongeOffsetX,
                            (worldY + spongeOffsetY) * cavesConfig.horizontalScale,
                            worldZ + spongeOffsetZ,
                            cavesConfig.spongeFractal);

                        perlinNoiseValues[GridIndex(x, y, z,numberOfPointsXZ)] =  height3D;
                    }
                }
            }

            //Cicliamo sulle celle 2x2x2
            for(int cellY = 0; cellY < numberOfCellsY; cellY++){
                for(int cellZ = 0; cellZ < numberOfCellsXZ; cellZ++){
                    for(int cellX = 0; cellX < numberOfCellsXZ; cellX++){
                        float corners[8];
                        float maxCorner = -2.0f;
                        for(int c = 0 ; c < 8 ; c++){
                            //I 3 bit di c codificano quale angolo della cella stiamo leggendo
                            int offsetX = c & 1;        //bit 0
                            int offsetY = (c >> 1) & 1; //bit 1
                            int offsetZ = (c >> 2) & 1; //bit 2

                            int index = GridIndex(cellX + offsetX, cellY + offsetY, cellZ + offsetZ, numberOfPointsXZ);
                            corners[c] = perlinNoiseValues[index];
                            maxCorner = std::max(maxCorner, corners[c]);
                        }

                        //Se maxCorner <= soglia allora scartiamo, non si scava.
                        if(maxCorner <= cavesConfig.spongeThreshold) continue;

                        //Andiamo a ciclare sulla cella se > soglia
                        //cella Step x Step x Step
                        for(int insideCellY = 0; insideCellY < spongeGridSize; insideCellY++){
                            int localChunkY = minHeight + cellY * spongeGridSize + insideCellY; // y chunk [0-15]
                            if(localChunkY >= Blocks::CHUNK_SIZE_Y) continue;

                            for(int insideCellZ = 0; insideCellZ < spongeGridSize; insideCellZ++){
                                for(int insideCellX = 0; insideCellX < spongeGridSize; insideCellX++){
                                    int localChunkX = cellX * spongeGridSize + insideCellX; //x chunk [0-15]
                                    int localChunkZ = cellZ * spongeGridSize + insideCellZ; //z chunk [0-15]

                                    //Altezza del terreno in quella colonna
                                    int surface = heightMap[HeightIndex(localChunkX, localChunkZ)];
                                    //Range del terreno scavabile
                                    float depthRange = (float) std::max(surface - minHeight, 1);
                                    //Posizione relativa tra [Bedrock-Altezza Massima] in numero float [0.0-1.0] 
                                    float nearSurface = std::clamp((float) (localChunkY - minHeight) / depthRange, 0.0f, 1.0f);
                                    //Threshold - La threshold aumenta se la superficie è più vicina
                                    float threshold = cavesConfig.spongeThreshold + cavesConfig.depthBias * nearSurface;

                                    //Interpolazione lineare tra i valori di tutti i corner
                                    float value = Trilinear(corners, (float)insideCellX / (float)spongeGridSize, (float)insideCellY / (float)spongeGridSize, (float)insideCellZ / (float)spongeGridSize);
                                    if(value > threshold){
                                        carveMap[CarveIndex(localChunkX, localChunkY, localChunkZ)] = 1; //Caverna!
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /*
            Individua i blocchi da rimuovere e rimpiazzare con aria seguento l'algoritmo dei tunnel (worm) del chunk. 
            Il perlin 3D si valuta solo sui punti di una griglia (wormGridSize) e poi si interpola;        
        */
        void CarveWormCave(int chunkX, int chunkZ, int maximumHeightY, std::vector<uint8_t>& carveMap) const{
            const int minHeight = cavesConfig.minY;
            if(minHeight > maximumHeightY) return;

            const int actualWorkingHeight = (maximumHeightY - minHeight);
            const int numberOfCellsY = divideAndRoundUp(actualWorkingHeight,wormGridSize);
            if(numberOfCellsY <= 0) return;

            //DensityScale scala la frequenza e threshold assieme [sqrt perchè i tunnel per volume hanno come frequenza^2]
            float densityScale = std::sqrt(cavesConfig.wormCaveDensity);
            FractalParams scaledFractal = cavesConfig.wormFractal;

            scaledFractal.frequency *= densityScale;
            float scaledWormThreshold = cavesConfig.wormThreshold * densityScale;

            const int numberOfCellsXZ = Blocks::CHUNK_SIZE_X / wormGridSize;
            const int numberOfPointsXZ = numberOfCellsXZ + 1;
            const int numberOfPointsY  = numberOfCellsY  + 1;

            //Allochiamo i due vettori che sarannno usati per contenere i valori di perlin noise 3D
            std::vector<float> perlinNoiseValuesA(numberOfPointsXZ * numberOfPointsXZ * numberOfPointsY);
            std::vector<float> perlinNoiseValuesB(numberOfPointsXZ * numberOfPointsXZ * numberOfPointsY);
        
            /*
                Riempiamo i vettori con i valori di perlin noise 3D
                Campioniamo solo per 1/4 delle volte, 
                approssimiamo utilizzando l'interpolazione trilineare 
                e con una griglia di minore granularità (4x4x4)
            */
            for(int y = 0; y < numberOfPointsY; y++){
                for(int z = 0; z < numberOfPointsXZ; z++){
                    for(int x = 0; x < numberOfPointsXZ; x++){
                        float worldX = (float) (chunkX * Blocks::CHUNK_SIZE_X + x * wormGridSize);
                        float worldY = (float) (minHeight + y * wormGridSize);
                        float worldZ = (float) (chunkZ * Blocks::CHUNK_SIZE_Z + z * wormGridSize);
                        int index = GridIndex(x, y, z, numberOfPointsXZ);

                        //Perlin due volte con offset diversi
                        perlinNoiseValuesA[index] = Fractal3D(*noise,
                            worldX + wormAOffsetX,
                            (worldY + wormAOffsetY) * cavesConfig.wormVerticalScale,
                            worldZ + wormAOffsetZ,
                            scaledFractal);
                        perlinNoiseValuesB[index] = Fractal3D(*noise,
                            worldX + wormBOffsetX,
                            (worldY + wormBOffsetY) * cavesConfig.wormVerticalScale,
                            worldZ + wormBOffsetZ,
                            scaledFractal);
                    }
                }
            }

            //Cicliamo sulle celle 4x4x4
            for(int cellY = 0; cellY < numberOfCellsY; cellY++){
                for(int cellZ = 0; cellZ < numberOfCellsXZ; cellZ++){
                    for(int cellX = 0; cellX < numberOfCellsXZ; cellX++){
                        float cornersA[8];
                        float cornersB[8];
                        for(int c = 0; c < 8; c++){
                            //I 3 bit di c codificano quale angolo della cella stiamo leggendo
                            int offsetX = c        & 1; //bit 0 [0-1]
                            int offsetY = (c >> 1) & 1; //bit 1 [0-1]
                            int offsetZ = (c >> 2) & 1; //bit 2 [0-1]

                            int index = GridIndex(cellX + offsetX, cellY + offsetY, cellZ + offsetZ, numberOfPointsXZ);
                            cornersA[c] = perlinNoiseValuesA[index];
                            cornersB[c] = perlinNoiseValuesB[index];
                        }
                        //Controlliamo se i valori dei Corners della cella 4x4x4 sono al di fuori di threshold e -threshold, se lo è skip
                        if(AllCornersOutside(cornersA, scaledWormThreshold) || AllCornersOutside(cornersB, scaledWormThreshold)) continue;

                        //Andiamo a ciclare sulla cella non saltata (4x4x4)
                        for(int insideCellY = 0; insideCellY < wormGridSize; insideCellY++){
                            int localChunkY = minHeight + cellY * wormGridSize + insideCellY;
                            if(localChunkY >= Blocks::CHUNK_SIZE_Y) continue;
                            for(int insideCellZ = 0; insideCellZ < wormGridSize; insideCellZ++){
                                for(int insideCellX = 0; insideCellX < wormGridSize; insideCellX++){
                                    float normalizedCellBlockX = (float)insideCellX / (float)wormGridSize;
                                    float normalizedCellBlockY = (float)insideCellY / (float)wormGridSize;
                                    float normalizedCellBlockZ = (float)insideCellZ / (float)wormGridSize;

                                    //Interpolazione e Intersezione 
                                    float valueA = std::abs(Trilinear(cornersA, normalizedCellBlockX, normalizedCellBlockY, normalizedCellBlockZ));
                                    if(valueA >= scaledWormThreshold) continue;
                                    float valueB = std::abs(Trilinear(cornersB, normalizedCellBlockX, normalizedCellBlockY, normalizedCellBlockZ));
                                    if(valueB >= scaledWormThreshold) continue;

                                    int localChunkX = cellX * wormGridSize + insideCellX; //x chunk [0-15]
                                    int localChunkZ = cellZ * wormGridSize + insideCellZ; //z chunk [0-15]

                                    for(int dy = 0 ; dy < cavesConfig.minCeilingHeight && localChunkY + dy < maximumHeightY ; dy++){
                                        carveMap[CarveIndex(localChunkX, localChunkY + dy, localChunkZ)] = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /*
            Scava le caverne campionando il Perlin 3D in coordinate MONDO (niente cuciture tra chunk).
            Si scava solo tra minY e (superficie - surfaceMargin): la crosta resta sempre intatta
        */
        void GenerateCaves(Blocks::Chunk& chunk,int chunkX,int chunkZ,const HeightMap& heightMap) const{
            if(!cavesConfig.enabled) return;

            std::vector<uint8_t> caveCarveMap(Blocks::CHUNK_SIZE_X * Blocks::CHUNK_SIZE_Y * Blocks::CHUNK_SIZE_Z, 0);
            int maximumHeight = *std::max_element(heightMap.begin(), heightMap.end());
            if(cavesConfig.spongeEnabled) CarveSpongeCave(chunkX, chunkZ, maximumHeight, heightMap, caveCarveMap);
            if(cavesConfig.wormsEnabled)  CarveWormCave(chunkX, chunkZ, maximumHeight, caveCarveMap);

            //Andiamo a genererare le caverne con heightMap
            for(int x = 0; x < Blocks::CHUNK_SIZE_X ; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z ; z++){
                    int surface = heightMap[HeightIndex(x,z)];
                    int maxY = std::min(surface - cavesConfig.surfaceMargin, Blocks::CHUNK_SIZE_Y);

                    for(int y = cavesConfig.minY; y < maxY; y++){
                        //Se è 0 allora non è caverna
                        if(caveCarveMap[CarveIndex(x, y, z)] == 0) continue;

                        //Se è 1, allora dobbiamo sostituire con l'aria
                        if(chunk.Get(x,y-1,z) == Blocks::BlockType::DIRT){
                            chunk.Set(x, y-1, z, Blocks::BlockType::GRASS);
                        }
                        chunk.Set(x, y, z, Blocks::BlockType::AIR);   
                    }
                }
            }

            CarveSpaghetti2D(chunk, chunkX, chunkZ, heightMap);
        }

        /*
            Spaghetti 2D, l'idea è di ridurre il problema da 3D a 2D.
            Usiamo due texture perlin noise 2D valutati una volta per colonna (x,z)
        */
        void CarveSpaghetti2D(Blocks::Chunk& chunk, int chunkX, int chunkZ, const HeightMap& heightMap) const{
            if(!cavesConfig.spaghettiEnabled) return;
            if(cavesConfig.spaghettiMinY > cavesConfig.spaghettiMaxY) return;

            float averageHeightY   = 0.5f * (float) (cavesConfig.spaghettiMinY + cavesConfig.spaghettiMaxY);
            float halfWidthheightY = 0.5f * (float) (cavesConfig.spaghettiMaxY - cavesConfig.spaghettiMinY);
            FractalParams scaledPathFractal = cavesConfig.spaghettiPathFractal;
            scaledPathFractal.frequency *= cavesConfig.spaghettiDensity;
            float scaledThreshold = cavesConfig.spaghettiThreshold * cavesConfig.spaghettiDensity;

            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    float worldX = (float) (chunkX * Blocks::CHUNK_SIZE_X + x);
                    float worldZ = (float) (chunkZ * Blocks::CHUNK_SIZE_Z + z);

                    float path = Fractal2D(*noise, worldX + spaghettiPathOffsetX, worldZ + spaghettiPathOffsetZ, scaledPathFractal);
                    float distance = std::abs(path);
                    if(distance >= scaledThreshold) continue;

                    float elevation = Fractal2D(*noise, worldX + spaghettiElevationOffsetX, worldZ + spaghettiElevationOffsetZ, cavesConfig.spaghettiElevationFractal);
                    float center = averageHeightY + elevation * halfWidthheightY;


                    //1 al centro del tunnel, 0 sui bordi: sezione tondeggiante
                    float ratio = distance / scaledThreshold;
                    float roundness = std::sqrt(1.0f - ratio * ratio);
                    float halfHeight = 0.5f * (float) cavesConfig.spaghettiHeight * roundness;

                    int surface = heightMap[HeightIndex(x, z)];
                    int yStart = std::max((int) std::ceil(center - halfHeight), cavesConfig.minY);
                    int yEnd = std::min((int) std::floor(center + halfHeight), surface - cavesConfig.surfaceMargin - 1);

                    if(yStart > yEnd) continue;
                    if(chunk.Get(x,yStart-1,z) == Blocks::BlockType::DIRT){
                        chunk.Set(x,yStart-1, z, Blocks::BlockType::GRASS);
                    }
                    for(int y = yStart; y <= yEnd; y++){
                        chunk.Set(x, y, z, Blocks::BlockType::AIR);
                    }
                }
            }
        }

        void GenerateTrees(Blocks::Chunk& chunk, int chunkX, int chunkZ,const HeightMap& heightMap) const{
            /*
                seed_seq mescola seed e coordinate del chunk: stesso seed e stesso chunk
                danno sempre gli stessi alberi, indipendentemente dall'ordine di generazione
            */
            std::seed_seq seq{seed, static_cast<uint32_t>(chunkX), static_cast<uint32_t>(chunkZ)};
            std::mt19937 rng(seq);

            std::vector<std::pair<int,int>> treePositions; //Tronchi gia' piazzati in questo chunk (x,z)

            //x/z partono da 1 e si fermano a CHUNK_SIZE-2: cosi' x-1, x+1, z-1, z+1 restano sempre in bounds
            for(int x = 1; x < Blocks::CHUNK_SIZE_X - 1; x++){
                for(int z = 1; z < Blocks::CHUNK_SIZE_Z - 1; z++){
                    if(rng() % treeRarity != 0) continue;
                    if(IsTooCloseToOtherTree(x, z, treePositions)) continue;
                    
                    int surface = heightMap[HeightIndex(x, z)];;
                    if(!chunk.IsSolid(x,surface-1,z)) continue;
                    
                    treePositions.push_back({x, z});
                    int treeTop = surface + treeTrunkHeight;

                    for(int y = surface; y < treeTop; y++){
                        chunk.Set(x, y, z, Blocks::BlockType::LOGWOOD);
                    }

                    for(int y = surface + 3; y < treeTop + 1; y++){
                        for(int dx = -1; dx <= 1; dx++){
                            for(int dz = -1; dz <= 1; dz++){
                                if(dx == 0 && dz == 0 && y != treeTop) continue; //Al centro c'e' il tronco
                                PlaceLeaf(chunk, x + dx, y, z + dz);
                            }
                        }
                    }
                }
            }
        }

        //Piazza una foglia solo se la cella è libera: su terreno irregolare non deve scavare le colline vicine
        static void PlaceLeaf(Blocks::Chunk& chunk, int x, int y, int z){
            if(chunk.Get(x, y, z) != Blocks::BlockType::AIR) return;
            chunk.Set(x, y, z, Blocks::BlockType::LEAVES);
        }

        //Controlla se (x,z) e' entro minTreeDistance da un tronco gia' piazzato
        static bool IsTooCloseToOtherTree(int x, int z, const std::vector<std::pair<int,int>>& treePositions){
            int minDistanceSquared = minTreeDistance * minTreeDistance;
            for(const auto& pos : treePositions){
                int dx = x - pos.first;
                int dz = z - pos.second;
                if(dx * dx + dz * dz < minDistanceSquared) return true;
            }
            return false;
        }
    };
}

#endif
