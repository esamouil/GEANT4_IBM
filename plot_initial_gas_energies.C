#include <map>
#include <utility>
#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TStyle.h"

struct TrackInfo {
    bool   seen       = false;
    double first_tpre = 1e99;
    double initial_ke = 0.0;
    double total_edep = 0.0;
    int    pdg        = 0;
};

void plot_initial_gas_energies()
{
    const char* filename = "output_b_factor_0.root";

    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) {
        std::cout << "Error: cannot open " << filename << std::endl;
        return;
    }

    TTree* tree = dynamic_cast<TTree*>(file->Get("hits"));
    if (!tree) {
        std::cout << "Error: tree 'hits' not found" << std::endl;
        file->ls();
        return;
    }

    Int_t    event_id        = 0;
    Int_t    track_id        = 0;
    Int_t    particle_pdg_id = 0;
    Double_t t_pre           = 0.0;
    Double_t pre_ke          = 0.0;
    Double_t edep            = 0.0;

    tree->SetBranchAddress("event_id",        &event_id);
    tree->SetBranchAddress("track_id",        &track_id);
    tree->SetBranchAddress("particle_pdg_id", &particle_pdg_id);
    tree->SetBranchAddress("t_pre",           &t_pre);
    tree->SetBranchAddress("pre_ke",          &pre_ke);
    tree->SetBranchAddress("edep",            &edep);

    std::map<std::pair<int,int>, TrackInfo> trackMap;

    const Long64_t nEntries = tree->GetEntries();
    std::cout << "Total entries: " << nEntries << std::endl;

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);

        const bool isAlpha   = (particle_pdg_id == 1000020040);
        const bool isLithium = (particle_pdg_id == 1000030060 ||
                                particle_pdg_id == 1000030070);

        if (!isAlpha && !isLithium)
            continue;

        const std::pair<int,int> key(event_id, track_id);
        auto& info = trackMap[key];

        info.total_edep += edep;
        info.pdg = particle_pdg_id;

        if (!info.seen || t_pre < info.first_tpre) {
            info.seen       = true;
            info.first_tpre = t_pre;
            info.initial_ke = pre_ke;   // MeV
        }
    }

    TH1D* hAlphaInitial = new TH1D(
        "hAlphaInitial",
        "Alpha initial energy at gas entry;Energy [MeV];Counts",
        100, 0.0, 2.5
    );

    TH1D* hLithiumInitial = new TH1D(
        "hLithiumInitial",
        "Lithium initial energy at gas entry;Energy [MeV];Counts",
        100, 0.0, 2.5
    );

    TH1D* hAlphaEdep = new TH1D(
        "hAlphaEdep",
        "Alpha total deposited energy in gas;Energy [MeV];Counts",
        100, 0.0, 2.5
    );

    TH1D* hLithiumEdep = new TH1D(
        "hLithiumEdep",
        "Lithium total deposited energy in gas;Energy [MeV];Counts",
        100, 0.0, 2.5
    );

    for (const auto& item : trackMap) {
        const TrackInfo& info = item.second;

        if (info.pdg == 1000020040) {
            hAlphaInitial->Fill(info.initial_ke);
            hAlphaEdep->Fill(info.total_edep);
        } else {
            hLithiumInitial->Fill(info.initial_ke);
            hLithiumEdep->Fill(info.total_edep);
        }
    }

    std::cout << "Alpha tracks   : " << hAlphaInitial->GetEntries() << std::endl;
    std::cout << "Lithium tracks : " << hLithiumInitial->GetEntries() << std::endl;

    gStyle->SetOptStat(1110);

    TCanvas* c = new TCanvas("c", "Initial energy and total deposited energy", 1200, 900);
    c->Divide(2, 2);

    c->cd(1);
    hAlphaInitial->SetLineColor(kRed + 1);
    hAlphaInitial->SetLineWidth(2);
    hAlphaInitial->Draw();

    c->cd(2);
    hLithiumInitial->SetLineColor(kBlue + 1);
    hLithiumInitial->SetLineWidth(2);
    hLithiumInitial->Draw();

    c->cd(3);
    hAlphaEdep->SetLineColor(kRed + 1);
    hAlphaEdep->SetLineWidth(2);
    hAlphaEdep->Draw();

    c->cd(4);
    hLithiumEdep->SetLineColor(kBlue + 1);
    hLithiumEdep->SetLineWidth(2);
    hLithiumEdep->Draw();

    c->SaveAs("initial_and_edep_histograms.png");
    std::cout << "Saved: initial_and_edep_histograms.png" << std::endl;
}