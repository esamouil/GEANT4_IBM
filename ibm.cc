#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4SystemOfUnits.hh"

#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "RunAction.hh"

int main(int argc, char** argv)
{
    // ---------------------------------------------------------------
    // Load sweep config from energies.txt
    // Supported settings:  nThreads = N,  nEvents = N
    // Energies: plain numbers (in eV), one per line
    // ---------------------------------------------------------------
    std::vector<G4double> sweepEnergies;
    G4int nThreads = 10;
    G4int nEvents  = 10000000;
    G4bool useFixedHits = false;
    G4int targetHits = 1000;
    G4int batchEvents = 100000;
    {
        std::ifstream energyFile("energies.txt");
        if (!energyFile.is_open())
            throw std::runtime_error("Cannot open energies.txt — make sure it is in the working directory.");
        std::string line;
        while (std::getline(energyFile, line))
        {
            if (line.empty() || line[0] == '#') continue;
            // Check for key = value settings
            auto eq = line.find('=');
            if (eq != std::string::npos)
            {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                // trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of(" \t") + 1);

                std::string keyLower = key;
                std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                std::string valLower = val;
                std::transform(valLower.begin(), valLower.end(), valLower.begin(),
                               [](unsigned char c) { return std::tolower(c); });

                if (key == "nThreads") nThreads = std::stoi(val);
                else if (key == "nEvents")  nEvents  = std::stoi(val);
                else if (keyLower == "usefixedhits")
                    useFixedHits = (valLower == "1" || valLower == "true" || valLower == "yes" || valLower == "on");
                else if (keyLower == "targethits")
                    targetHits = std::stoi(val);
                else if (keyLower == "batchevents")
                    batchEvents = std::stoi(val);
                continue;
            }
            std::istringstream iss(line);
            G4double val;
            if (iss >> val)
                sweepEnergies.push_back(val * eV);
        }
        if (sweepEnergies.empty())
            throw std::runtime_error("energies.txt contains no energy values.");
        if (nEvents <= 0)
            throw std::runtime_error("nEvents must be > 0.");
        if (useFixedHits && targetHits <= 0)
            throw std::runtime_error("targetHits must be > 0 when useFixedHits is enabled.");
        if (batchEvents <= 0)
            throw std::runtime_error("batchEvents must be > 0.");
    }
    // ---------------------------------------------------------------

    G4UIExecutive *ui = nullptr;

    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager;
    #else
        G4RunManager *runManager = new G4RunManager;
        G4cout << "[warning] Built without G4MULTITHREADED. Runs will use one CPU core." << G4endl;
    #endif

    //Physics list:
    runManager->SetUserInitialization(new PhysicsList());

    //Detector Construction
    runManager->SetUserInitialization(new DetectorConstruction());

    //Action Initialization
    runManager->SetUserInitialization(new ActionInitialization());

    G4VisManager *visManager = new G4VisExecutive();
    visManager->Initialise();

    G4UImanager *UImanager = G4UImanager::GetUIpointer();

    if (argc == 1)
    {
        // Interactive / visualisation mode
        ui = new G4UIExecutive(argc, argv);
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
    }
    else if (G4String(argv[1]) == "sweep")
    {
        // ----------------------------------------------------------
        // Energy sweep mode: ./ibm sweep
        // Runs nEvents for each energy in sweepEnergies and appends
        // (energy_eV, efficiency, hits, total) to efficiency.txt
        // ----------------------------------------------------------
        UImanager->ApplyCommand("/run/numberOfThreads " + std::to_string(nThreads));
        UImanager->ApplyCommand("/run/initialize");

        // Write header (overwrites any previous file)
        {
            std::ofstream outFile("efficiency.txt");
            outFile << "# energy_eV\tefficiency\tevents_with_hits\ttotal_events\n";
        }

        for (G4double energy : sweepEnergies)
        {
            RunAction::SetCurrentEnergy(energy);

            std::ostringstream cmd;
            cmd << std::setprecision(10) << "/gun/energy " << energy / eV << " eV";
            UImanager->ApplyCommand(cmd.str());

            if (!useFixedHits)
            {
                RunAction::SetWriteEfficiencyEnabled(true);
                UImanager->ApplyCommand("/run/beamOn " + std::to_string(nEvents));
            }
            else
            {
                RunAction::SetWriteEfficiencyEnabled(false);

                G4int cumulativeHits = 0;
                G4int cumulativeEvents = 0;
                while (cumulativeHits < targetHits)
                {
                    UImanager->ApplyCommand("/run/beamOn " + std::to_string(batchEvents));
                    cumulativeHits += RunAction::GetLastRunHits();
                    cumulativeEvents += RunAction::GetLastRunTotalEvents();

                    if (RunAction::GetLastRunTotalEvents() <= 0)
                    {
                        throw std::runtime_error("A beamOn chunk produced zero events; aborting fixed-hits loop.");
                    }
                }

                const G4double eff = (cumulativeEvents > 0)
                    ? static_cast<G4double>(cumulativeHits) / static_cast<G4double>(cumulativeEvents)
                    : 0.0;

                G4cout << "[fixed-hits] Energy " << energy / eV << " eV"
                       << " | hits=" << cumulativeHits
                       << " | events=" << cumulativeEvents
                       << " | efficiency=" << eff << G4endl;

                std::ofstream outFile("efficiency.txt", std::ios::app);
                outFile << std::scientific << std::setprecision(6)
                        << energy / eV      << "\t"
                        << eff              << "\t"
                        << cumulativeHits   << "\t"
                        << cumulativeEvents << "\n";
            }
        }

        RunAction::SetWriteEfficiencyEnabled(true);
    }
    else
    {
        // Single-macro batch mode (unchanged): ./ibm run.mac
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        UImanager->ApplyCommand(command + fileName);
    }

    return 0;
}
