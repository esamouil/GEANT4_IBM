void stacked_hist() {

    TFile *f = TFile::Open("build/output0.root");
    TTree *t = (TTree*)f->Get("hits");

    int particle_id;
    double dep_energy;

    t->SetBranchAddress("particle_id",&particle_id);
    t->SetBranchAddress("dep_energy",&dep_energy);

    int bins = 200;
    double xmin = 0.0;
    double xmax = 2.0;

    TH1D *h1 = new TH1D("h1","Deposited Energy",bins,xmin,xmax);
    TH1D *h2 = new TH1D("h2","Deposited Energy",bins,xmin,xmax);
    TH1D *h3 = new TH1D("h3","Deposited Energy",bins,xmin,xmax);
    TH1D *h4 = new TH1D("h4","Deposited Energy",bins,xmin,xmax);
    TH1D *h5 = new TH1D("h5","Deposited Energy",bins,xmin,xmax);
    TH1D *h6 = new TH1D("h6","Deposited Energy",bins,xmin,xmax);
    TH1D *h7 = new TH1D("h7","Deposited Energy",bins,xmin,xmax);


    h1->SetFillColor(kOrange);
    h2->SetFillColor(kBlue);
    h3->SetFillColor(kGreen);
    h4->SetFillColor(kMagenta);
    h5->SetFillColor(kYellow);
    h6->SetFillColor(kCyan);
    h7->SetFillColor(kRed);

    Long64_t n = t->GetEntries();

    for(Long64_t i=0;i<n;i++){
        t->GetEntry(i);

        if(particle_id==1)  h1->Fill(dep_energy);
        if(particle_id==2)  h2->Fill(dep_energy);
        if(particle_id==3)  h3->Fill(dep_energy);
        if(particle_id==4)  h4->Fill(dep_energy);
        if(particle_id==5)  h5->Fill(dep_energy);
        if(particle_id==6)  h6->Fill(dep_energy);
        if(particle_id==-1) h7->Fill(dep_energy);
        
    }

    std::cout<<"Unkown Particle Events: "<<h7->Integral()<<endl;

    THStack *hs = new THStack("hs","Energy Deposit by Particle");

    hs->Add(h1);
    hs->Add(h2);
    hs->Add(h3);
    hs->Add(h4);
    hs->Add(h5);
    hs->Add(h6);
    hs->Add(h7);

    TCanvas *c = new TCanvas("c","stacked",900,700);
    c->SetGridx();
    c->SetGridy();

    hs->Draw("hist");

    hs->GetXaxis()->SetTitle("Deposited Energy");
    hs->GetYaxis()->SetTitle("Counts");
    hs->GetXaxis()->SetRangeUser(xmin,xmax);

    // legend
    TLegend *leg = new TLegend(0.7,0.7,0.9,0.9);
    leg->AddEntry(h1,"Gamma","f");
    leg->AddEntry(h2,"Alpha","f");
    leg->AddEntry(h3,"Lithium 7","f");
    leg->AddEntry(h4,"Electrons","f");
    leg->AddEntry(h5,"Positrons","f");
    leg->AddEntry(h6,"Ar41","f");
    if(h7->Integral()>0) leg->AddEntry(h7,"who knows","f");
    leg->Draw();

    c->Update();

    // --- Individual histograms ---
    TCanvas *c_ind[7];
    TH1D* hists[7] = {h1,h2,h3,h4,h5,h6,h7};
    const char* names[7] = {"Gamma","Alpha","Lithium 7","Electrons","Positrons","Ar41","Unknown"};

    for(int i=0;i<7;i++){
        c_ind[i] = new TCanvas(Form("c_ind%d",i),"Individual Histogram",800,600);
        c_ind[i]->SetGridx(); c_ind[i]->SetGridy();
        hists[i]->Draw("hist");
        hists[i]->GetXaxis()->SetTitle("Deposited Energy");
        hists[i]->GetYaxis()->SetTitle("Counts");
        c_ind[i]->SetTitle(names[i]);
        c_ind[i]->Update();
    }

}