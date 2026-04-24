#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4Accumulable.hh"
#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include <fstream>
#include <iomanip>
#include <set>

class RunAction : public G4UserRunAction
{
    public:
        RunAction();
        ~RunAction();

        virtual void BeginOfRunAction(const G4Run *);
        virtual void EndOfRunAction(const G4Run *);

        void AddEventWithHit(G4int eventID);

        // Called by ibm.cc before each run in sweep mode
        static void SetCurrentEnergy(G4double e) { fCurrentEnergy = e; }
        static void SetWriteEfficiencyEnabled(G4bool enabled) { fWriteEfficiencyEnabled = enabled; }
        static G4int GetLastRunHits() { return fLastRunHits; }
        static G4int GetLastRunTotalEvents() { return fLastRunTotalEvents; }
        static G4double GetLastRunEfficiency() { return fLastRunEfficiency; }

    private:
        static G4double fCurrentEnergy;
        static G4bool fWriteEfficiencyEnabled;
        static G4int fLastRunHits;
        static G4int fLastRunTotalEvents;
        static G4double fLastRunEfficiency;
        G4Accumulable<G4int> fEventsWithHitsCount{0};
        std::set<G4int> eventsWithHits;
};

#endif