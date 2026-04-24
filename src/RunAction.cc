#include "RunAction.hh"

G4double RunAction::fCurrentEnergy = 0.0;
G4bool RunAction::fWriteEfficiencyEnabled = true;
G4int RunAction::fLastRunHits = 0;
G4int RunAction::fLastRunTotalEvents = 0;
G4double RunAction::fLastRunEfficiency = 0.0;

RunAction::RunAction()
{
    G4AccumulableManager::Instance()->Register(fEventsWithHitsCount);

    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetNtupleMerging(true);

    //analysisManager->CreateH1("Edep","Energy Deposit",100,0,2*MeV);

    //Ntuple creation
    analysisManager->CreateNtuple("hits","hits");

    //setting the ntuple columns
    analysisManager->CreateNtupleIColumn("event_id");       // col 0
    analysisManager->CreateNtupleIColumn("track_id");       // col 1
    analysisManager->CreateNtupleIColumn("particle_pdg_id");// col 2

    analysisManager->CreateNtupleDColumn("edep");           // col 3
    //analysisManager->CreateNtupleDColumn("total_edep");    

    analysisManager->CreateNtupleDColumn("x_pos_pre");      // col 4
    analysisManager->CreateNtupleDColumn("y_pos_pre");      // col 5
    analysisManager->CreateNtupleDColumn("z_pos_pre");      // col 6
    analysisManager->CreateNtupleDColumn("x_pos_post");     // col 7
    analysisManager->CreateNtupleDColumn("y_pos_post");     // col 8
    analysisManager->CreateNtupleDColumn("z_pos_post");     // col 9

    analysisManager->CreateNtupleDColumn("t_pre");          // col 10
    analysisManager->CreateNtupleDColumn("t_post");         // col 11

    analysisManager->CreateNtupleIColumn("proc_id");        // col 12
    analysisManager->CreateNtupleDColumn("pre_ke");         // col 13

    analysisManager->FinishNtuple(0);

}

RunAction::~RunAction()
{

}

void RunAction::AddEventWithHit(G4int eventID)
{
    if (eventsWithHits.insert(eventID).second)
    {
        ++fEventsWithHitsCount;
    }
}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    G4AccumulableManager::Instance()->Reset();
    eventsWithHits.clear();

    G4int runID = run->GetRunID();

    std::stringstream strRunID;
    strRunID<<runID;
    analysisManager->OpenFile("output"+strRunID.str()+".root");

}

void RunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();
    G4AccumulableManager::Instance()->Merge();

    analysisManager->Write();

    analysisManager->CloseFile();

    G4int runID = run->GetRunID();

    // Calculate efficiency
    G4int numEventsWithHits = fEventsWithHitsCount.GetValue();
    G4int totalEvents = run->GetNumberOfEvent();
    G4double efficiency = (totalEvents > 0) ? (G4double)numEventsWithHits / totalEvents : 0.0;

    if (IsMaster())
    {
        fLastRunHits = numEventsWithHits;
        fLastRunTotalEvents = totalEvents;
        fLastRunEfficiency = efficiency;

        G4cout << "Finishing run " << runID << G4endl;
        G4cout << "Efficiency: " << efficiency << " (" << numEventsWithHits << "/" << totalEvents << " events with hits)" << G4endl;

        if (fWriteEfficiencyEnabled)
        {
            std::ofstream outFile("efficiency.txt", std::ios::app);
            outFile << std::scientific << std::setprecision(6)
                    << fCurrentEnergy / eV << "\t"
                    << efficiency         << "\t"
                    << numEventsWithHits  << "\t"
                    << totalEvents        << "\n";
        }
    }
}