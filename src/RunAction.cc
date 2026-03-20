#include "RunAction.hh"

RunAction::RunAction()
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

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

    analysisManager->FinishNtuple(0);

}

RunAction::~RunAction()
{

}

void RunAction::BeginOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    G4int runID = run->GetRunID();

    std::stringstream strRunID;
    strRunID<<runID;
    analysisManager->OpenFile("output"+strRunID.str()+".root");

}

void RunAction::EndOfRunAction(const G4Run *run)
{
    G4AnalysisManager *analysisManager = G4AnalysisManager::Instance();

    analysisManager->Write();

    analysisManager->CloseFile();

    G4int runID = run->GetRunID();

    G4cout<<"Finishing run "<< runID<<G4endl;
}