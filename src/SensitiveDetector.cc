#include "SensitiveDetector.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4RunManager.hh"

#include "G4Alpha.hh"
#include "G4Gamma.hh"
#include "G4Neutron.hh"
#include "G4Event.hh"
#include "G4IonTable.hh"
#include "RunAction.hh"


namespace {
    constexpr G4bool kVerboseBoronSteps    = false;
    constexpr G4bool kSaveOnlyAlphaLithium = true;  // true = only alpha & Li, false = all except neutrons
}

SensitiveDetector::SensitiveDetector(G4String name, G4bool isBoron) : G4VSensitiveDetector(name), fIsBoronSD(isBoron)
{
    fTotalEnergyDeposited = 0.;
    fCurrentTrackID = -1;
    fTrackEdep = 0.;
    fTrackRecorded = false;
}

SensitiveDetector::~SensitiveDetector() {}

void SensitiveDetector::Initialize(G4HCofThisEvent *)
{
    fTotalEnergyDeposited = 0.; // total for the whole event
    fCurrentTrackID = -1;       // reset track tracking
    fTrackEdep = 0.;
    fTrackRecorded = false;
}

void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
{

}

G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    G4Track* track = aStep->GetTrack();
    G4ParticleDefinition* particle = track->GetDefinition();

    const G4String& particleName = particle->GetParticleName();
    const G4bool isAlpha = (particleName == "alpha");
    const G4bool isLithium =
        (particle->GetParticleType() == "nucleus" && particle->GetAtomicNumber() == 3);

    G4VPhysicalVolume* preVolume =
        aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume();

    const G4String volumeName = preVolume ? preVolume->GetName() : "";
    const G4bool inBoronPlate = (volumeName == "phys_boron_plate");

    if (kVerboseBoronSteps && inBoronPlate && (isAlpha || isLithium))
    {
        const G4VProcess* proc = aStep->GetPostStepPoint()->GetProcessDefinedStep();
        const G4String procName = proc ? proc->GetProcessName() : "none";

        G4cout
            << "[boronSD] "
            << "particle=" << particleName
            << " trackID=" << track->GetTrackID()
            << " step#=" << track->GetCurrentStepNumber()
            << " stepLength=" << aStep->GetStepLength() / nm << " nm"
            << " preKE=" << aStep->GetPreStepPoint()->GetKineticEnergy() / keV << " keV"
            << " postKE=" << aStep->GetPostStepPoint()->GetKineticEnergy() / keV << " keV"
            << " process=" << procName
            << G4endl;
    }

    // Debug: print neutron steps in boron slab
    // if (fIsBoronSD && particle == G4Neutron::NeutronDefinition()) {
    //     G4double stepLength = aStep->GetStepLength();
    //     G4cout << "NEUTRON in BORON - Step length: " << stepLength/nm << " nm" << G4endl;
    // }

    // Ignore neutrons
    if (particle == G4Neutron::NeutronDefinition())
        return true;

    // Filter: save only alpha and lithium if flag is set
    if (kSaveOnlyAlphaLithium)
    {
        if (!isAlpha && !isLithium)
            return true;
    }

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

    // Debug: print step length of the particles entering the gas volume
    // G4double stepLength = aStep->GetStepLength();
    // G4cout << "Step length: " << stepLength/nm << " nm | Particle: " << particle->GetParticleName() << G4endl;

    // Step midpoint
    // G4ThreeVector pos = (aStep->GetPreStepPoint()->GetPosition() +
    //                      aStep->GetPostStepPoint()->GetPosition()) / 2.;

    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Make a scheme to save the creator process names as integers
    G4int creatorProcessID = -1;
    const G4VProcess* creatorProcess = track->GetCreatorProcess();

    if (creatorProcess) {
        const G4String& creatorName = creatorProcess->GetProcessName();

        if      (creatorName == "neutronInelastic") creatorProcessID = 1;
        else if (creatorName == "nCaptureHP")       creatorProcessID = 2;
        else if (creatorName == "phot")             creatorProcessID = 3;
        else if (creatorName == "compt")            creatorProcessID = 4;
        else if (creatorName == "conv")             creatorProcessID = 5;
        else if (creatorName == "eIoni")            creatorProcessID = 6;
        else if (creatorName == "eBrem")            creatorProcessID = 7;
        else if (creatorName == "annihil")          creatorProcessID = 8;
    }

    // Only tag events from the boron sensitive detector.
    if (fIsBoronSD) {
        G4int pdg = particle->GetPDGEncoding();
        if ((creatorProcessID == 1) &&
            (pdg == 1000020040 || pdg == 1000030060 || pdg == 1000030070)) {
            RunAction* runAction = const_cast<RunAction*>(dynamic_cast<const RunAction*>(G4RunManager::GetRunManager()->GetUserRunAction()));
            if (runAction) {
                runAction->AddEventWithHit(eventID);
            }
        }

        return true;
    }

    // --- Fill per-step info only for gas SD ---
    analysisManager->FillNtupleIColumn(0,0,eventID);
    analysisManager->FillNtupleIColumn(0,1,trackID);
    analysisManager->FillNtupleIColumn(0,2,particle->GetPDGEncoding());

    analysisManager->FillNtupleDColumn(0,3,edep);

    analysisManager->FillNtupleDColumn(0,4,aStep->GetPreStepPoint()->GetPosition().x());
    analysisManager->FillNtupleDColumn(0,5,aStep->GetPreStepPoint()->GetPosition().y());
    analysisManager->FillNtupleDColumn(0,6,aStep->GetPreStepPoint()->GetPosition().z());

    analysisManager->FillNtupleDColumn(0,7,aStep->GetPostStepPoint()->GetPosition().x());
    analysisManager->FillNtupleDColumn(0,8,aStep->GetPostStepPoint()->GetPosition().y());
    analysisManager->FillNtupleDColumn(0,9,aStep->GetPostStepPoint()->GetPosition().z());

    analysisManager->FillNtupleDColumn(0,10,aStep->GetPreStepPoint()->GetGlobalTime());
    analysisManager->FillNtupleDColumn(0,11,aStep->GetPostStepPoint()->GetGlobalTime());

    analysisManager->FillNtupleIColumn(0,12,creatorProcessID);
    analysisManager->FillNtupleDColumn(0,13,
        aStep->GetPreStepPoint()->GetKineticEnergy() / MeV);

    analysisManager->AddNtupleRow(0);

    // --- Print total energy for selected creator processes at track end ---
    G4StepStatus stepStatus = aStep->GetPostStepPoint()->GetStepStatus();
    if ((stepStatus == fGeomBoundary || track->GetTrackStatus() == fStopAndKill) &&
        !fTrackRecorded)
    {
        fTrackRecorded = true;

        G4String creatorProcessName = "primary";
        if (track->GetCreatorProcess())
            creatorProcessName = track->GetCreatorProcess()->GetProcessName();

        G4cout << "Event: " << eventID
               << " | Particle: " << particle->GetParticleName()
               << " | TrackID: " << trackID
               << " | Deposited energy in this track: " << fTrackEdep/keV << " keV"
               << " | Creator process: " << creatorProcessName
               << " | PDG Particle ID: " << particle->GetPDGEncoding()
               << G4endl;
    }

    return true;
}