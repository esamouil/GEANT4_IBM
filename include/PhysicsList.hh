#ifndef PHYSICSLIST_HH
#define PHYSICSLIST_HH

#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4HadronPhysicsQGSP_BERT_HP.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4StepLimiter.hh"


class PhysicsList : public G4VModularPhysicsList
{

    public: 
        PhysicsList();
        ~PhysicsList();

};




#endif
