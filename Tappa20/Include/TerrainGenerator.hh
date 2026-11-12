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
        //Array di size x * z, HeightMap è un alias
        using HeightMap = std::array<int, Blocks::CHUNK_SIZE_X * Blocks::CHUNK_SIZE_Z>;
        

        static constexpr int baseHeight = Blocks::CHUNK_SIZE_Y / 4; //Quota media del terreno
        static constexpr int dirtDepth = 5; //Strati di terra sotto l'erba
        static constexpr int wormGridStep = 4; //Passo (blocchi) della griglia di campionamento dei worm, SEMPRE DIVISORE DI 16
        static constexpr int spongeGridStep = 2; //Passo (blocchi) della griglia di campionamento della spugna
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
            HeightMap heightMap = BuildHeightMap(chunkX, chunkZ);

            //Riempo il mondo di blocchi
            FillTerrain(chunk, heightMap);

            //Creo le caverne
            GenerateCaves(chunk, chunkX, chunkZ, heightMap); 

            //Aggiungo gli alberi e altro
            GenerateTrees(chunk, chunkX, chunkZ, heightMap);
        }

    private:
        static int HeightIndex(int x, int z){
            return x + Blocks::CHUNK_SIZE_X * z;
        }

        //Una sola valutazione del fBm 2D per colonna: poi tutte le passate leggono da qui
        HeightMap BuildHeightMap(int chunkX, int chunkZ) const{
            HeightMap heightMap;
            for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                    heightMap[HeightIndex(x, z)] = GetSurfaceHeight(chunkX * Blocks::CHUNK_SIZE_X + x, chunkZ * Blocks::CHUNK_SIZE_Z + z);
                }
            }
            return heightMap;
        }

        void FillTerrain(Blocks::Chunk& chunk, const HeightMap& heightMap) const{
            for(int x = 0; x < Blocks::CHUNK_SIZE_X; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z; z++){
                    int surface = heightMap[HeightIndex(x,z)];
                    chunk.Set(x, 0, z, Blocks::BlockType::STONE); //Bedrock (SOON)
                    for(int y = 1; y < surface; y++){
                        if(y == surface - 1) chunk.Set(x, y, z, Blocks::BlockType::GRASS);
                        else if(y >= (surface-1)-dirtDepth) chunk.Set(x, y, z, Blocks::BlockType::DIRT);
                        else chunk.Set(x, y, z, Blocks::BlockType::STONE);
                    }
                }
            }
        }

        //Indice della maschera worm del chunk: un byte per cella (x, y, z locali)
        static int WormMaskIndex(int x, int y, int z){
            return (y * Blocks::CHUNK_SIZE_Z + z) * Blocks::CHUNK_SIZE_X + x;
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

        //Vero se tutti gli angoli sono fuori da [-threshold, threshold] dallo stesso lato:
        //allora anche l'interpolato lo e' e la cella non puo' contenere tunnel
        static bool AllCornersOutside(const float* v, float threshold){
            bool allAbove = true;
            bool allBelow = true;
            for(int i = 0; i < 8; i++){
                if(v[i] <= threshold) allAbove = false;
                if(v[i] >= -threshold) allBelow = false;
            }
            return allAbove || allBelow;
        }

        static int GridIndex(int i, int j , int k , int pointsXZ){
            return (j * pointsXZ + k) * pointsXZ + i;
        }

        /*
            Costruisce la maschera dei tunnel worm del chunk: i due campi 3D si valutano solo sui
            punti di una griglia (passo wormGridStep) e poi si interpolano. Densita' = scala di
            frequenza e soglia insieme (sqrt perche' i tunnel per volume vanno come frequenza^2),
            cosi' la larghezza resta invariata e cambia solo la quantita'
        */
        void BuildWormMask(int chunkX, int chunkZ, int maximumHeight, std::vector<uint8_t>& mask) const{
            const int step = wormGridStep;
            const int minY = cavesConfig.minY;
            const int cellsY = (maximumHeight - minY + step - 1) / step;
            if(cellsY <= 0) return;

            float densityScale = std::sqrt(cavesConfig.wormCaveDensity);
            FractalParams scaledFractal = cavesConfig.wormFractal;
            scaledFractal.frequency *= densityScale;
            float threshold = cavesConfig.wormThreshold * densityScale;

            const int cellsXZ = Blocks::CHUNK_SIZE_X / step;
            const int pointsXZ = cellsXZ + 1;
            const int pointsY = cellsY + 1;

            //Allochiamo i due field che sarannno usati per le celle 4x4x4
            std::vector<float> fieldA(pointsXZ * pointsXZ * pointsY);
            std::vector<float> fieldB(pointsXZ * pointsXZ * pointsY);
        
            //Riempiamo field con i valori di perlin 3D (Approssimazione)
            for(int j = 0; j < pointsY; j++){
                for(int k = 0; k < pointsXZ; k++){
                    for(int i = 0; i < pointsXZ; i++){
                        float worldX = (float) (chunkX * Blocks::CHUNK_SIZE_X + i * step);
                        float worldY = (float) (minY + j * step);
                        float worldZ = (float) (chunkZ * Blocks::CHUNK_SIZE_Z + k * step);
                        int index = GridIndex(i, j, k, pointsXZ);

                        //Perlin due volte con offset diversi
                        fieldA[index] = Fractal3D(*noise,
                            worldX + wormAOffsetX,
                            (worldY + wormAOffsetY) * cavesConfig.wormVerticalScale,
                            worldZ + wormAOffsetZ,
                            scaledFractal);
                        fieldB[index] = Fractal3D(*noise,
                            worldX + wormBOffsetX,
                            (worldY + wormBOffsetY) * cavesConfig.wormVerticalScale,
                            worldZ + wormBOffsetZ,
                            scaledFractal);
                    }
                }
            }

            //Cicliamo sulle celle 4x4x4
            for(int cellY = 0; cellY < cellsY; cellY++){
                for(int cellZ = 0; cellZ < cellsXZ; cellZ++){
                    for(int cellX = 0; cellX < cellsXZ; cellX++){
                        float cornersA[8];
                        float cornersB[8];
                        for(int c = 0; c < 8; c++){
                            int index = GridIndex(cellX + (c & 1), cellY + ((c >> 1) & 1), cellZ + ((c >> 2) & 1),pointsXZ);
                            cornersA[c] = fieldA[index];
                            cornersB[c] = fieldB[index];
                        }
                        //Controlliamo che il valore dei cornersi sia tra threshold e -threshold, se non lo è skip
                        if(AllCornersOutside(cornersA, threshold) || AllCornersOutside(cornersB, threshold)) continue;

                        //Andiamo a ciclare sulla cella non saltata
                        for(int ly = 0; ly < step; ly++){
                            int y = minY + cellY * step + ly;
                            if(y >= Blocks::CHUNK_SIZE_Y) continue;
                            for(int lz = 0; lz < step; lz++){
                                for(int lx = 0; lx < step; lx++){
                                    float fx = (float) lx / (float) step;
                                    float fy = (float) ly / (float) step;
                                    float fz = (float) lz / (float) step;

                                    //Interpolazione 
                                    if(std::abs(Trilinear(cornersA, fx, fy, fz)) >= threshold) continue;
                                    if(std::abs(Trilinear(cornersB, fx, fy, fz)) >= threshold) continue;

                                    mask[WormMaskIndex(cellX * step + lx, y, cellZ * step + lz)] = 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        /*
            Scrive nella maschera le sacche (spugna) del chunk. Il campo 3D si valuta solo sui punti
            di una griglia (passo spongeGridStep) e poi si interpola; la soglia, che dipende da y e
            dalla superficie della colonna, si calcola invece esatta per ogni blocco
        */
        void BuildSpongeMask(int chunkX, int chunkZ, int topY, const HeightMap& heightMap, std::vector<uint8_t>& mask) const{
            const int step = spongeGridStep;
            const int minY = cavesConfig.minY;
            const int cellsY = (topY - minY + step - 1) / step;
            if(cellsY <= 0) return;

            const int cellsXZ = Blocks::CHUNK_SIZE_X / step;
            const int pointsXZ = cellsXZ + 1;
            const int pointsY = cellsY + 1;

            //Riempiamo field con i valori di perlin 3D (Approssimazione)
            std::vector<float> field(pointsXZ * pointsXZ * pointsY);
            for(int j = 0; j < pointsY; j++){
                for(int k = 0; k < pointsXZ; k++){
                    for(int i = 0; i < pointsXZ; i++){
                        float worldX = (float) (chunkX * Blocks::CHUNK_SIZE_X + i * step);
                        float worldY = (float) (minY + j * step);
                        float worldZ = (float) (chunkZ * Blocks::CHUNK_SIZE_Z + k * step);
                        float height3D = Fractal3D(*noise,worldX + spongeOffsetX,
                            (worldY + spongeOffsetY) * cavesConfig.verticalScale,
                            worldZ + spongeOffsetZ,
                            cavesConfig.spongeFractal);

                        field[GridIndex(i, j, k,pointsXZ)] =  height3D;
                    }
                }
            }

            //Cicliamo sulle celle 2x2x2
            for(int cellY = 0; cellY < cellsY; cellY++){
                for(int cellZ = 0; cellZ < cellsXZ; cellZ++){
                    for(int cellX = 0; cellX < cellsXZ; cellX++){
                        float corners[8];
                        float maxCorner = -2.0f;
                        for(int c = 0 ; c < 8 ; c++){
                            corners[c] = field[GridIndex(cellX + (c & 1), cellY + ((c >> 1) & 1), cellZ + ((c >> 2) & 1), pointsXZ)];
                            maxCorner = std::max(maxCorner, corners[c]);
                        }

                        //Se maxCorner <= soglia allora scartiamo
                        if(maxCorner <= cavesConfig.spongeThreshold) continue;

                        //Andiamo a ciclare sulla cella se > threshold
                        //cella Step x Step x Step
                        for(int ly = 0; ly < step; ly++){
                            int y = minY + cellY * step + ly; // y chunk
                            if(y >= Blocks::CHUNK_SIZE_Y) continue;

                            for(int lz = 0; lz < step; lz++){
                                for(int lx = 0; lx < step; lx++){
                                    int localX = cellX * step + lx; //x chunk
                                    int localZ = cellZ * step + lz; //z chunk

                                    //Altezza terreno colonna
                                    int surface = heightMap[HeightIndex(localX, localZ)];
                                    //Ampiezza terreno scavabile
                                    float depthRange = (float) std::max(surface - minY, 1);
                                    //[0-1] Posizione relativa tra [Bedrock-Altezza Massima] 
                                    float nearSurface = std::clamp((float) (y - minY) / depthRange, 0.0f, 1.0f);
                                    //Threshold - La threshold aumenta se la superficie è più vicina
                                    float threshold = cavesConfig.spongeThreshold + cavesConfig.depthBias * nearSurface;

                                    //Interpolazione lineare tra i valori di tutti i corner
                                    float value = Trilinear(corners, (float) lx / (float) step, (float) ly / (float) step, (float) lz / (float) step);
                                    if(value > threshold){
                                        mask[WormMaskIndex(localX, y, localZ)] = 1; //Caverna!
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

            std::vector<uint8_t> caveMask(Blocks::CHUNK_SIZE_X * Blocks::CHUNK_SIZE_Y * Blocks::CHUNK_SIZE_Z, 0);
            int maximumHeight = *std::max_element(heightMap.begin(), heightMap.end());
            if(cavesConfig.spongeEnabled) BuildSpongeMask(chunkX, chunkZ, maximumHeight, heightMap, caveMask);
            if(cavesConfig.wormsEnabled) BuildWormMask(chunkX, chunkZ, maximumHeight, caveMask);

            //Andiamo a genererare le caverne con heightMap
            for(int x = 0; x < Blocks::CHUNK_SIZE_X ; x++){
                for(int z = 0; z < Blocks::CHUNK_SIZE_Z ; z++){
                    int surface = heightMap[HeightIndex(x,z)];
                    int maxY = std::min(surface - cavesConfig.surfaceMargin, Blocks::CHUNK_SIZE_Y);

                    for(int y = cavesConfig.minY; y < maxY; y++){
                        if(!caveMask[WormMaskIndex(x, y, z)]) continue;
                        if(chunk.Get(x,y-1,z) == Blocks::BlockType::DIRT){
                            chunk.Set(x, y-1, z, Blocks::BlockType::GRASS);
                        }
                        for(int dy = 0 ; dy < cavesConfig.minCeilingHeight && y + dy < surface ; dy++){
                            chunk.Set(x, y + dy, z, Blocks::BlockType::AIR);
                        }   
                    }
                }
            }

            GenerateSpaghetti2D(chunk, chunkX, chunkZ, heightMap);
        }

        /*
            Spaghetti 2D: i due campi si valutano UNA volta per colonna (non per cella).
            Dove |path| < soglia c'e' un tunnel; elevation ne decide la quota del centro.
            La sezione e' tondeggiante: l'altezza cala verso i bordi della larghezza
        */
        void GenerateSpaghetti2D(Blocks::Chunk& chunk, int chunkX, int chunkZ, const HeightMap& heightMap) const{
            if(!cavesConfig.spaghettiEnabled) return;

            float centerMid = 0.5f * (float) (cavesConfig.spaghettiMinY + cavesConfig.spaghettiMaxY);
            float centerHalfRange = 0.5f * (float) (cavesConfig.spaghettiMaxY - cavesConfig.spaghettiMinY);
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
                    float center = centerMid + elevation * centerHalfRange;


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
                    //Probabilita' 1%. Uso % su rng() invece di uniform_int_distribution
                    //perche' quest'ultima non e' riproducibile tra standard library diverse
                    if(rng() % treeRarity != 0) continue;
                    if(IsTooCloseToOtherTree(x, z, treePositions)) continue;
                    
                    int groundY = heightMap[HeightIndex(x, z)];;
                    if(!chunk.IsSolid(x,groundY-1,z)) continue;
                    
                    treePositions.push_back({x, z});
                    int treeTop = groundY + treeTrunkHeight;

                    for(int y = groundY; y < treeTop; y++){
                        chunk.Set(x, y, z, Blocks::BlockType::LOGWOOD);
                    }

                    for(int y = groundY + 3; y < treeTop + 1; y++){
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
            if(chunk.Get(x, y, z) == Blocks::BlockType::AIR){
                chunk.Set(x, y, z, Blocks::BlockType::LEAVES);
            }
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
