#include "PrimaryGenerator.hh"

PrimaryGenerator::PrimaryGenerator()
{
    //one particle per event
    fParticleGun = new G4ParticleGun(1);

    //Particle position 
    G4double x=-5 *cm;
    G4double y=0 *cm;
    G4double z=0 *cm;

    //Particle direction
    G4double px = 1;
    G4double py = 0;
    G4double pz = 0;

    //Particle Kinetic energy
    G4double KE = 0.025 *eV;

    G4ThreeVector position(x,y,z);
    G4ThreeVector momentum(px,py,pz);

    //Particle type
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle("neutron");

    fParticleGun->SetParticlePosition(position);
    fParticleGun->SetParticleMomentumDirection(momentum);
    fParticleGun->SetParticleEnergy(KE);
    fParticleGun->SetParticleDefinition(particle);

}

PrimaryGenerator::~PrimaryGenerator()
{
    delete fParticleGun;
}

void PrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    //Create Vertex
    fParticleGun->GeneratePrimaryVertex(anEvent);
}