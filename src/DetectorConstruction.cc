#include "DetectorConstruction.hh"
#include "G4Isotope.hh"
#include "G4Element.hh"

DetectorConstruction::DetectorConstruction()
{
}

DetectorConstruction::~DetectorConstruction()
{
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
    G4bool checkOverlaps = true;


    // Create B10 isotope
    G4Isotope* B10 = new G4Isotope("B10", 5, 10, 10.0129*g/mole); // name, Z, A, mass
    G4Element* elB10 = new G4Element("Boron10", "B10", 1);
    elB10->AddIsotope(B10, 100.*perCent); // 100% B10
    G4Material* M_B10 = new G4Material("B10_solid", 2.34*g/cm3, 1); // density, number of elements
    M_B10->AddElement(elB10, 1); // just the one element

    //Declare the materials
    G4NistManager *nist = G4NistManager::Instance();
    G4Material *M_vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material *M_Al = nist->FindOrBuildMaterial("G4_Al");
    G4Material *M_Gas = nist->FindOrBuildMaterial("G4_Ar");
    G4Material *M_B = nist->FindOrBuildMaterial("G4_B");
    //G4Material *M_B = M_B10;

    // Dimension Parameters
    G4double xWorld = 20*cm;
    G4double yWorld = 20*cm;
    G4double zWorld = 20*cm;

    G4double yDetector = 57*mm;
    G4double zDetector = 57*mm;

    G4double gasWidth = 2*mm;
    G4double plateWidth = 0.1*mm;
    G4double boronWidth = 100*nm;

    G4double center_plate_x = 0;
    G4double left_plate_x = -1*(plateWidth+gasWidth);
    G4double right_plate_x = plateWidth+gasWidth+boronWidth;
    G4double left_gas_x = -1*(plateWidth/2 +gasWidth/2);
    G4double right_gas_x = (plateWidth/2 +gasWidth/2);
    G4double boron_plate_x = plateWidth/2 +gasWidth +boronWidth/2;




    //Define the world volume
    G4Box *solidWorld = new G4Box("solidWorld", xWorld/2, yWorld/2, zWorld/2);
    G4LogicalVolume *logicalWorld = new G4LogicalVolume(solidWorld, M_vacuum,"logicalWorld");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, {0,0,0}, logicalWorld, "physWorld", 0, false, 0, checkOverlaps);

    //Define the center aluminum plate
    G4Box *solid_center_plate = new G4Box("solid_center_plate", plateWidth/2, yDetector/2, zDetector/2);
    G4LogicalVolume *logical_center_plate = new G4LogicalVolume(solid_center_plate, M_Al,"logical_center_plate");
    G4VPhysicalVolume *phys_center_plate = new G4PVPlacement(0, {center_plate_x,0,0}, logical_center_plate, "phys_center_plate", logicalWorld, false, 0, checkOverlaps);

    //define the left aluminum plate
    G4Box *solid_left_plate = new G4Box("solid_left_plate", plateWidth/2, yDetector/2, zDetector/2);
    G4LogicalVolume *logical_left_plate = new G4LogicalVolume(solid_left_plate, M_Al,"logical_left_plate");
    G4VPhysicalVolume *phys_left_plate = new G4PVPlacement(0, {left_plate_x,0,0}, logical_left_plate, "phys_left_plate", logicalWorld, false, 0, checkOverlaps);

    //define the right aluminum plate
    G4Box *solid_right_plate = new G4Box("solid_right_plate", plateWidth/2, yDetector/2, zDetector/2);
    G4LogicalVolume *logical_right_plate = new G4LogicalVolume(solid_right_plate, M_Al,"logical_right_plate");
    G4VPhysicalVolume *phys_right_plate = new G4PVPlacement(0, {right_plate_x,0,0}, logical_right_plate, "phys_right_plate", logicalWorld, false, 0, checkOverlaps);

    //define the boron plate
    G4Box *solid_boron_plate = new G4Box("solid_boron_plate", boronWidth/2, yDetector/2, zDetector/2);
    G4LogicalVolume *logical_boron_plate = new G4LogicalVolume(solid_boron_plate, M_B,"logical_boron_plate");
    G4VPhysicalVolume *phys_boron_plate = new G4PVPlacement(0, {boron_plate_x,0,0}, logical_boron_plate, "phys_boron_plate", logicalWorld, false, 0, checkOverlaps);

    //define the left gas volume
    G4Box *solid_left_gas = new G4Box("solid_left_gas", gasWidth/2, yDetector/2, zDetector/2);
    G4LogicalVolume *logical_left_gas = new G4LogicalVolume(solid_left_gas, M_Gas,"logical_left_gas");
    G4VPhysicalVolume *phys_left_gas = new G4PVPlacement(0, {left_gas_x,0,0}, logical_left_gas, "phys_left_gas", logicalWorld, false, 0, checkOverlaps);

    //define the right gas volume
    G4Box *solid_right_gas = new G4Box("solid_right_gas", gasWidth/2, yDetector/2, zDetector/2);
    logical_right_gas = new G4LogicalVolume(solid_right_gas, M_Gas,"logical_right_gas");
    G4VPhysicalVolume *phys_right_gas = new G4PVPlacement(0, {right_gas_x,0,0}, logical_right_gas, "phys_right_gas", logicalWorld, false, 0, checkOverlaps);


    //Setting up the colors

    //Make the visual attribute for the materials
    G4VisAttributes *Al_Vis_Att = new G4VisAttributes(G4Color(200./255,200./255,200./255,0.5));
    Al_Vis_Att->SetForceSolid(true);

    G4VisAttributes *B_Vis_Att = new G4VisAttributes(G4Color(160./255, 32./255, 240./255,0.99));
    B_Vis_Att->SetForceSolid(true);

    G4VisAttributes *Gas_Vis_Att = new G4VisAttributes(G4Color(255./255,255./255,150./255,0.3));
    Gas_Vis_Att->SetForceSolid(true);
    
    //Assign the visual attributes to the logical volumes
    logical_center_plate->SetVisAttributes(Al_Vis_Att);
    logical_left_plate->SetVisAttributes(Al_Vis_Att);
    logical_right_plate->SetVisAttributes(Al_Vis_Att);

    logical_boron_plate->SetVisAttributes(B_Vis_Att);

    logical_left_gas->SetVisAttributes(Gas_Vis_Att);
    logical_right_gas->SetVisAttributes(Gas_Vis_Att);



    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    SensitiveDetector *sensDet = new SensitiveDetector("sensDet");
    logical_right_gas->SetSensitiveDetector(sensDet);
    G4SDManager::GetSDMpointer()->AddNewDetector(sensDet);

}