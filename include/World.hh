#ifndef WORLD_QUERY_HH
#define WORLD_QUERY_HH

namespace fcg
{
    //Interfaccia minima che la fisica del player usa per interrogare il mondo: le serve solo
    //sapere se una cella e' solida, nient'altro (non le mesh, non i chunk, non il rendering).
    //PlayerPhysics dipende SOLO da questa interfaccia, non dalla classe Scene per intero:
    //cosi' non ha ne' bisogno ne' accesso a nulla che non le serva (Draw, RaycastBlock, texture...).
    //Scene la implementera' (public IWorldQuery) piu' sotto in NicoCraft.cc.
    class IWorldQuery{
    public:
        virtual bool IsSolidAtWorld(int worldX, int worldY, int worldZ) = 0;
        virtual ~IWorldQuery() = default;
    };
}

#endif
