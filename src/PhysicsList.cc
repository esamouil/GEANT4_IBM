#include "PhysicsList.hh"


PhysicsList::PhysicsList()
{
    // Electromagnetic physics (needed for energy deposition)
    RegisterPhysics(new G4EmStandardPhysics());

    // High precision hadronic physics
    RegisterPhysics(new G4HadronPhysicsQGSP_BERT_HP());

    //Step limiter physics (to ensure small steps in the boron layer)
    RegisterPhysics(new G4StepLimiterPhysics());


}

PhysicsList:: ~PhysicsList()
{
    
}