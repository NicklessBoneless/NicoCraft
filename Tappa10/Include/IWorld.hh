#ifndef IWORLD_HH
#define IWORLD_HH

namespace fcg
{
    //Interfaccia minima che la fisica del player usa per interrogare il mondo: le serve solo
    //sapere se una cella e' solida, nient'altro (non le mesh, non i chunk, non il rendering).
    //PlayerPhysics dipende SOLO da questa interfaccia, non dalla classe World per intero:
    //cosi' non ha ne' bisogno ne' accesso a nulla che non le serva (Draw, RaycastBlock, texture...).
    //World la implementera' (public IWorld) in World.hh.
    class IWorld{
    public:
        virtual bool IsSolidAtWorld(int worldX, int worldY, int worldZ) = 0;
        virtual ~IWorld() = default;
    };
}

#endif
