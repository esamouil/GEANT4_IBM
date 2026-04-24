#ifndef SENSITIVEDETECTOR_HH
#define SENSITIVEDETECTOR_HH

#include "G4VSensitiveDetector.hh"

#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"



class SensitiveDetector : public G4VSensitiveDetector
{
    public:
        SensitiveDetector(G4String, G4bool isBoron = false);
        ~SensitiveDetector();

    private:

        G4double fTotalEnergyDeposited;
        G4bool fIsBoronSD;

        virtual void Initialize(G4HCofThisEvent *) override;
        virtual void EndOfEvent(G4HCofThisEvent *) override;

        virtual G4bool ProcessHits(G4Step *, G4TouchableHistory *);

        G4double fTotalEdep = 0.;
        G4int fCurrentTrackID = -1;
        G4double fTrackEdep = 0.;
        G4bool fTrackRecorded = false;
};

#endif