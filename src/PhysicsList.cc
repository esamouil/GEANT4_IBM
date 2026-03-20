#include "PhysicsList.hh"

PhysicsList::PhysicsList()
{
    // Electromagnetic physics (needed for energy deposition)
    RegisterPhysics(new G4EmStandardPhysics());

    // High precision hadronic physics
    RegisterPhysics(new G4HadronPhysicsQGSP_BERT_HP());


}

PhysicsList:: ~PhysicsList()
{
    
}