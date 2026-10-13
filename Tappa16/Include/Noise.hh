#ifndef NOISE_HH
#define NOISE_HH

namespace fcg{
    //Interfaccia comune: ogni algoritmo restituisce un valore in [-1, 1]
    class Noise{
    public:
        virtual float Sample2D(float x, float z) const = 0;
        virtual float Sample3D(float x, float y, float z) const = 0;
        virtual ~Noise() = default;
    };

    struct FractalParams{
        int octaves = 4;
        float frequency = 0.02f;

        //Lacunarity: di quanto cresce la frequenza ad ogni ottava
        float frequencyMultiplier = 2.0f;

        //Persistence: di quanto cala l'ampiezza ad ogni ottava
        float amplitudeMultiplier = 0.5f;
    };

    //Somma di ottave (fBm), normalizzata in [-1, 1]
    inline float Fractal2D(const Noise& noise, float x, float z, const FractalParams& params){
        float sum = 0.0f;
        float amplitude = 1.0f;
        float maxAmplitude = 0.0f;
        float frequency = params.frequency;

        //Per ogni x e z ripetiamo per il numero di ottave
        for(int i = 0; i < params.octaves; i++){
            sum += noise.Sample2D(x * frequency, z * frequency) * amplitude;
            maxAmplitude += amplitude;
            amplitude *= params.amplitudeMultiplier;
            frequency *= params.frequencyMultiplier;
        }
        return sum / maxAmplitude;
    }
}

#endif
