#include "SensitiveDetector.hh"

#include "G4Alpha.hh"
#include "G4Gamma.hh"
#include "G4Neutron.hh"
#include "G4Event.hh"
#include "G4IonTable.hh"


SensitiveDetector::SensitiveDetector(G4String name) : G4VSensitiveDetector(name)
{
    fTotalEnergyDeposited = 0.;
    fCurrentTrackID = -1;
    fTrackEdep = 0.;
}

SensitiveDetector::~SensitiveDetector() {}

void SensitiveDetector::Initialize(G4HCofThisEvent *)
{
    fTotalEnergyDeposited = 0.; // total for the whole event
    fCurrentTrackID = -1;       // reset track tracking
    fTrackEdep = 0.;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{

}

G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    G4Track* track = aStep->GetTrack();
    G4ParticleDefinition* particle = track->GetDefinition();

    // Ignore neutrons
    if (particle == G4Neutron::NeutronDefinition())
        return true;

    G4int trackID = track->GetTrackID();

    // Reset per-track accumulator for new tracks
    if (trackID != fCurrentTrackID)
    {
        fCurrentTrackID = trackID;
        fTrackEdep = 0.;
        fTrackRecorded = false;
    }

    // Energy deposited in this step
    G4double edep = aStep->GetTotalEnergyDeposit();
    fTrackEdep += edep;

    // Step midpoint
    G4ThreeVector pos = (aStep->GetPreStepPoint()->GetPosition() +
                         aStep->GetPostStepPoint()->GetPosition()) / 2.;

    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();


    //Make a scheme to save the creator process names as integers
    G4int creatorProcessID;
    if(track->GetCreatorProcess()->GetProcessName() == "neutronInelastic" ) creatorProcessID = 1 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "nCaptureHP" ) creatorProcessID = 2 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "phot" ) creatorProcessID = 3 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "compt" ) creatorProcessID = 4 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "conv" ) creatorProcessID = 5 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "eIoni" ) creatorProcessID = 6 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "eBrem" ) creatorProcessID = 7 ;
    else if(track->GetCreatorProcess()->GetProcessName()== "annihil" ) creatorProcessID = 8 ;
    else creatorProcessID=-1;



    // --- Fill per-step info only (no total energy) ---
    analysisManager->FillNtupleIColumn(0,0,eventID);
    analysisManager->FillNtupleIColumn(0,1,trackID);
    analysisManager->FillNtupleIColumn(0,2,particle->GetPDGEncoding());

    //fill the energy deposition per step
    analysisManager->FillNtupleDColumn(0,3,edep); // step energy

    //fill pre-step point coordinates
    analysisManager->FillNtupleDColumn(0,4,aStep->GetPreStepPoint()->GetPosition().x());
    analysisManager->FillNtupleDColumn(0,5,aStep->GetPreStepPoint()->GetPosition().y());
    analysisManager->FillNtupleDColumn(0,6,aStep->GetPreStepPoint()->GetPosition().z());
    //fill post-step point coordinates
    analysisManager->FillNtupleDColumn(0,7,aStep->GetPostStepPoint()->GetPosition().x());
    analysisManager->FillNtupleDColumn(0,8,aStep->GetPostStepPoint()->GetPosition().y());
    analysisManager->FillNtupleDColumn(0,9,aStep->GetPostStepPoint()->GetPosition().z());

    //fill the pre and post step global times:
    analysisManager->FillNtupleDColumn(0,10,aStep->GetPreStepPoint()->GetGlobalTime());
    analysisManager->FillNtupleDColumn(0,11,aStep->GetPostStepPoint()->GetGlobalTime());
    //fill the creator process integer 
    analysisManager->FillNtupleIColumn(0,12,creatorProcessID);

    analysisManager->AddNtupleRow(0);

    // --- Print total energy for selected creator processes at track end ---
    G4StepStatus stepStatus = aStep->GetPostStepPoint()->GetStepStatus();
    if ((stepStatus == fGeomBoundary || track->GetTrackStatus() == fStopAndKill) &&
        !fTrackRecorded)
    {
        fTrackRecorded = true;

        G4String creatorProcess = "primary";
        if (track->GetCreatorProcess())
            creatorProcess = track->GetCreatorProcess()->GetProcessName();

        
            G4cout << "Event: " << eventID
                   << " | Particle: " << particle->GetParticleName()
                   << " | TrackID: " << trackID
                   << " | Deposited energy in this track: " << fTrackEdep/keV << " keV"
                   << " | Creator process: " << creatorProcess
                   << " | PDG Particle ID: " << particle->GetPDGEncoding()
                 //  <<"  | Pre_step_time: "<<aStep->GetPreStepPoint()->GetGlobalTime()
                 //  <<"  | Post_step_time: "<<aStep->GetPostStepPoint()->GetGlobalTime()
                   << G4endl;
        
    }

    return true;
}