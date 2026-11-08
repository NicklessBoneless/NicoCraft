#ifndef RENDER_STATS_HH
#define RENDER_STATS_HH

namespace fcg{
    //Contatori del frame corrente: "Total" = tutta la geometria dei chunk non vuoti,
    //"Drawn" = quella effettivamente mandata alla GPU dopo il culling
    struct RenderStats{
        int sectionsTotal = 0;
        int sectionsDrawn = 0;
        long long verticesTotal = 0;
        long long verticesDrawn = 0;
        long long trianglesTotal = 0;
        long long trianglesDrawn = 0;
    };
}

#endif