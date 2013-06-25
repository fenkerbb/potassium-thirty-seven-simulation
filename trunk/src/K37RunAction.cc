// Authors: Spencer Behling and Benjamin Fenker 2013

#include <iomanip>

#include "K37HistogramManager.hh"
#include "K37RunAction.hh"

#include "K37EventAction.hh"
#include "K37PrimaryGeneratorAction.hh"
#include "K37DetectorConstruction.hh"
#include "K37ListOfVolumeNames.hh"
#include "K37AnnihilationPosition.hh"
#include "K37AllPossibleEventInformation.hh"
// Just need for testing
#include "JTW_Event.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VVisManager.hh"
#include "G4ios.hh"
#include "G4EmCalculator.hh"

#include "K37Analysis.hh"

using AGG::Aggregator;
using std::string;

G4bool fillEvGenData = true;
G4bool fillAllSDData = true;


//----------------------------------

K37RunAction::K37RunAction(K37DetectorConstruction* det,
                           K37PrimaryGeneratorAction* kin,
                           K37ListOfVolumeNames* list,
                           K37AnnihilationPosition* anhilP,
                           K37AllPossibleEventInformation* APEI,
                           K37HistogramManager * his) :
  detector(det), kinematic(kin), listOfEnteredVolumes(list), runMessenger(0),
  annihilationPosition(anhilP), AllEventInformation(APEI), histograms(his) {
  recordAnnihilationPosition = false;
  // recordAnnihilationPosition = true;
  recordVolumeNames = false;
  // recordVolumeNames = true;
  recordEventInformation = true;
  // recordEventInformation = false;
  NbofEvents = 0;
  accepted = 0;
  accepted_bs = 0;
  bs = 0;
  // secondaryInSiLi = 0;
  // secondaryInDedx = 0;
  // secondaryInBoth = 0;
  falsePositive = 0;
  falsePositiveWithSili = 0;
  falsePositiveWithDedx = 0;
  falsePositiveWithBoth = 0;

  acceptedWithSili2 = 0;

  acceptedWithDedx2 = 0;
  /*acceptedWithDedx2SecondaryFiredDedx=0;
    acceptedWithDedx2SecondaryFiredSili=0;
    acceptedWithDedx2SecondaryFiredDedx2=0;
    acceptedWithDedx2SecondaryFiredSiliAndDedx2=0;
    acceptedWithDedx2SecondaryFiredDedxAndSili=0;
    acceptedWithDedx2SecondaryFiredDedxAndDedx2=0;
    acceptedWithDedx2SPrimaryFiredAll=0;
    acceptedWithDedx2SecondaryFiredAll=0;
  */

  plusZHits_vc = 0.0;
  minusZHits_vc = 0.0;

  acceptedWithNone = 0;
  acceptedWithNoneSecondaryFiredDedx = 0;
  acceptedWithNoneSecondaryFiredSili = 0;
  acceptedWithNonePrimaryFiredBoth = 0;
  acceptedWithNoneSecondaryFiredBoth = 0;

  falsePositiveWithNone = 0;
  falsePositiveWithNoneSecondaryFiredDedx = 0;
  falsePositiveWithNoneSecondaryFiredSili = 0;
  falsePositiveWithNonePrimaryFiredBoth = 0;
  falsePositiveWithNoneSecondaryFiredBoth = 0;

  vetoed = 0;
  nonVetoed = 0;

  acceptedPrimaryScatteredOffHoops = 0;

  snprintf(configuration_filename_, sizeof(configuration_filename_),
           "IOconfiguration.mac");
  snprintf(output_directory_, sizeof(output_directory_),
           "/home/bfenker/geant4_workdir/K37Build");

  outFileName = "old_analysis_manager";
  // manager automatically
  runMessenger = new K37RunMessenger(this);

  // Detectors
  qdc_upper_pmt_ = new Generic_Channel("QDC_UpperPMT", 1, "/D");
  qdc_lower_pmt_ = new Generic_Channel("QDC_LowerPMT", 2, "/D");
  dl_x_pos_      = new TDC_Channel("DL_X_Pos"    , 3, "/v", false);
  dl_z_pos_      = new TDC_Channel("DL_Z_Pos"    , 4, "/v", false);
  char name[200];
  for (G4int i = 0; i < 40; i++) {
    snprintf(name, sizeof(name), "STRIP_UX_%02d", i);
    strip_detector_upper_x_[i] = new Generic_Channel(name, i+5, "/D");
    snprintf(name, sizeof(name), "STRIP_UY_%02d", i);
    strip_detector_upper_y_[i] = new Generic_Channel(name, i+45, "/D");
    snprintf(name, sizeof(name), "STRIP_LX_%02d", i);
    strip_detector_lower_x_[i] = new Generic_Channel(name, i+85, "/D");
    snprintf(name, sizeof(name), "STRIP_LY_%02d", i);
    strip_detector_lower_y_[i] = new Generic_Channel(name, i+125, "/D");
  }

  // TDCs
  tdc_scint_top_     = new TDC_Channel("TDC_SCINT_TOP"    , 201, "/v", false);
  tdc_scint_bottom_  = new TDC_Channel("TDC_SCINT_BOTTOM" , 202, "/v", false);
  tdc_ion_mcp_       = new TDC_Channel("TDC_ION_MCP"      , 203, "/v", false);
  tdc_electron_mcp_  = new TDC_Channel("TDC_ELECTRON_MCP" , 203, "/v", false);
  tdc_photo_diode_   = new TDC_Channel("TDC_PHOTO_DIODE"  , 204, "/v", false);
  tdc_long_electron_ = new TDC_Channel("TDC_LONG_ELECTRON", 205, "/v", false);
  tdc_nimio32_trigger_ = new TDC_Channel("TDC_NIMIO32_TRIGGER", 205, "/v",
                                         false);
  tdc_tdc_stop_= new TDC_Channel("TDC_TDC_STOP", 205, "/v", false);

  // Event generator and other Geant-only data
  electron_kinetic_energy_generated_ = new Generic_Channel("T_GEN_ELE", 301,
                                                           "/D");
  electron_mu_generated_ = new Generic_Channel("MU_GEN_ELE", 302, "/D");
  recoil_mu_generated_   = new Generic_Channel("MU_GEN_RECOIL", 302, "/D");
  recoil_mcp_particle_   = new Generic_Channel("ION_MCP_PARTICLE_PDG",
                                               303, "/D");

  // Information to match analyzer (not really simulated)
  run_action_       = new Generic_Channel("Run_Number"      , 401, "/D");
  tnim_op_beam_     = new Generic_Channel("TNIM_OP_Beam"    , 402, "/l");
  ttlbit_sigmaplus_ = new Generic_Channel("TTLBit_SigmaPlus", 403, "/D");
  the_aggregator_ = 0;
}

//----------------------------------

K37RunAction::~K37RunAction() {
  delete runMessenger;
  delete qdc_upper_pmt_;
  delete qdc_lower_pmt_;
  delete dl_x_pos_;
  delete dl_z_pos_;
  for (G4int i = 0; i < 40; i++) {
    delete strip_detector_upper_x_[i];
    delete strip_detector_upper_y_[i];
    delete strip_detector_lower_x_[i];
    delete strip_detector_lower_y_[i];
  }
  delete tdc_scint_top_;
  delete tdc_scint_bottom_;
  delete tdc_ion_mcp_;
  delete tdc_electron_mcp_;
  delete tdc_photo_diode_;
  delete tdc_long_electron_;
  delete tdc_nimio32_trigger_;
  delete tdc_tdc_stop_;
  delete electron_kinetic_energy_generated_;
  delete electron_mu_generated_;
  delete recoil_mu_generated_;
  delete run_action_;
  delete recoil_mcp_particle_;
  delete tnim_op_beam_;
  delete ttlbit_sigmaplus_;
}

//----------------------------------

void K37RunAction::BeginOfRunAction(const G4Run* aRun) {
  histograms->book();

  NbofEvents = 0;
  if (recordVolumeNames == true) {
    listOfEnteredVolumes->touchFile();
    listOfEnteredVolumes->clearList();
    listOfEnteredVolumes->setShouldVolumeNamesBeRecorded();
  } else {
    listOfEnteredVolumes->deleteFile();
  }

  if (recordAnnihilationPosition == true) {
    annihilationPosition->touchFile();
    annihilationPosition->clearList();
    annihilationPosition->setShouldAnnihilationBeRecorded();
  } else {
    annihilationPosition->deleteFile();
  }
  if (recordEventInformation== true) {
    AllEventInformation->touchFile();
    AllEventInformation->clearEventInformation();
    AllEventInformation->setShouldEventInformationBeRecorded();
  } else {
    AllEventInformation->deleteFile();
  }
  accepted = 0;
  accepted_bs = 0;
  bs = 0;
  // secondaryInSiLi = 0;
  // secondaryInDedx = 0;
  // secondaryInBoth = 0;
  falsePositive = 0;
  falsePositiveWithSili = 0;
  falsePositiveWithDedx = 0;
  falsePositiveWithBoth = 0;

  acceptedWithSili2 = 0;
  acceptedWithDedx2 = 0;
  /*acceptedWithDedx2SecondaryFiredDedx=0;
    acceptedWithDedx2SecondaryFiredSili=0;
    acceptedWithDedx2SecondaryFiredDedx2=0;
    acceptedWithDedx2SecondaryFiredSiliAndDedx2=0;
    acceptedWithDedx2SecondaryFiredDedxAndSili=0;
    acceptedWithDedx2SecondaryFiredDedxAndDedx2=0;
    acceptedWithDedx2SPrimaryFiredAll=0;
    acceptedWithDedx2SecondaryFiredAll=0;
  */
  acceptedWithNone = 0;
  acceptedWithNoneSecondaryFiredDedx = 0;
  acceptedWithNoneSecondaryFiredSili = 0;
  acceptedWithNonePrimaryFiredBoth = 0;
  acceptedWithNoneSecondaryFiredBoth = 0;

  falsePositiveWithNone = 0;
  falsePositiveWithNoneSecondaryFiredDedx = 0;
  falsePositiveWithNoneSecondaryFiredSili = 0;
  falsePositiveWithNonePrimaryFiredBoth = 0;
  falsePositiveWithNoneSecondaryFiredBoth = 0;

  acceptedPrimaryScatteredOffHoops  =  0;

  vetoed = 0;
  nonVetoed = 0;

  G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;
  G4RunManager::GetRunManager()->SetRandomNumberStore(true);

  /* outfile << "This sentence is appended to the file content\n";
     outfile<< "Event Number: "<<G4endl;
     outfile<< "energyDedx / keV" << "\t"<< "energySiLi / keV" << "\t"<< "(energyTotal)/ keV" << G4endl;
     outfile<< "Position and time since start of e+ exit from volume"<<G4endl;
     outfile<< "1= dedx 2=Sili"<< "\t"<<"time(ns)" << "\t"<<"X"<< "\t"<<"Y"<< "\t"<<"Z" <<G4endl;
     outfile<<G4endl;
     outfile<< "---------------------"<<G4endl;
  */

  // std::ofstream zpos;
  // zpos.open ("alive.txt", std::ofstream::out | std::ofstream::trunc);
  // zpos.close();

  /*
    std::ofstream projectionFront;
    projectionFront.open ("projectionFront.txt", std::ofstream::out | std::ofstream::trunc);
    projectionFront.close();

    std::ofstream projectionBack;
    projectionBack.open ("projectionBack.txt", std::ofstream::out | std::ofstream::trunc);
    projectionBack.close();
  */


  // std::ofstream runstat;
  // runstat.open ("runstat.txt", std::ofstream::out | std::ofstream::trunc);
  // runstat.close();

  std::ofstream detectorADA;
  detectorADA.open("detectorADA.txt",
                   std::ofstream::out | std::ofstream::trunc);
  detectorADA.close();

  std::ofstream detectorODA;
  detectorODA.open("detectorODA.txt",
                   std::ofstream::out | std::ofstream::trunc);
  detectorODA.close();


  // std::ofstream theta;
  // theta.open ("theta.txt", std::ofstream::out | std::ofstream::trunc);
  // theta.close();


  ///////////////////////// My new stuff //////////////////
  // Create analysis manager
  // The choice of analysis technology is done via selectin of a namespace
  // in K37Analysis.hh
  G4AnalysisManager *anMan = G4AnalysisManager::Instance();

  // Open an output file
  //
  // G4String fileName = "K37";
  FILE *io_file;
  char data_type[80], out_file[80];
  io_file = fopen(GetConfigurationFileName(), "r");
  fscanf(io_file, "%s %s", data_type, out_file);

  G4cout << "Opening file: " << out_file << G4endl;
  anMan->OpenFile(outFileName);
  // anMan->SetFirstHistoId(1);
  // anMan->SetFirstNtupleId(1);
  // Creating histograms
  //
  anMan->CreateH1("ADA Detector Scintillator", "ADA Detector Scintillator",
                  200, 0., 10000);
  anMan->CreateH1("ADA Detector Strip Detector", "ADA Detector Strip Detector",
                  1000, 0., 10000);
  anMan->CreateH1("ODA Detector Scintillator", "ODA Detector Scintillator",
                  200, 0., 10000);
  anMan->CreateH1("ODA Detector Strip Detector", "ODA Detector Strip Detector",
                  1000, 0., 10000);
  //  Histogram to get the cos(theta_e) from event generator
  anMan->CreateH1("Electron_angle_to_pol_generated",
                  "Electron_angle_to_pol_generated",
                  100, -1.1, 1.1);
  anMan->CreateH1("Electron energy (event generator)",
                  "Electron energy(event generator", 1000, 0.0, 10.0);
  anMan->CreateH1("v_over_c", "v_over_c", 100, 0.0, 1.0);

  // The order with which the ntuple columns are created MUST stay the same.
  // They are accessed simply by a number that is incremented each time one is
  // created.  These numbers are stored in K37AnalysisNumbering.  Add to the
  // end, but DO NOT add to the beginning and DO NOT add to the middle and DO
  // NOT rearrange these (without rearrning the numbering).  If you must
  // rearrange things, make sure to update the numbering!

  // One ntuple per accepted event will contain all the information of the event
  // By "all," I mean what we will get from the detectors.  Energy deposited.
  // Some of these also give information straight out of the event generator.
  anMan->CreateNtuple("ntuple", "Event data");
  anMan -> CreateNtupleDColumn("T_e_generated");
  anMan->CreateNtupleDColumn("v_over_c_generated");
  anMan->CreateNtupleDColumn("theta_e_generated");
  anMan -> CreateNtupleDColumn("omega");
  anMan->CreateNtupleIColumn("detector_hit");
  anMan->CreateNtupleDColumn("v_over_c");
  char ntupName[15];
  // Each strip detector reads energy left in each of its 80 channels
  // 2-dimensions * 40 strips/dimension * 2 detectors = 160 columns
  for (G4int i = 1; i <= 40; i++) {
    //    snprintf(ntupName, sizeof(ntupName), "SDplusZX%02d", i);
    snprintf(ntupName, sizeof(ntupName), "STRIP_UX_%02d", i);
    anMan->CreateNtupleDColumn(ntupName);
  }
  for (G4int i = 1; i <= 40; i++) {
    snprintf(ntupName, sizeof(ntupName), "STRIP_UY_%02d", i);
    anMan->CreateNtupleDColumn(ntupName);
  }
  for (G4int i = 1; i <= 40; i++) {
    snprintf(ntupName, sizeof(ntupName), "STRIP_LX_%02d", i);
    anMan->CreateNtupleDColumn(ntupName);
  }
  for (G4int i = 1; i <= 40; i++) {
    snprintf(ntupName, sizeof(ntupName), "STRIP_LY_%02d", i);
    anMan->CreateNtupleDColumn(ntupName);
  }
  // First level of accepted events:  energy in +z(-z) scintillator and strip
  // detector and no energy in the -z(+z) detector.  Neglects low-energy
  // back-scattered eventsxo.
  anMan -> CreateNtupleIColumn("accepted");
  anMan -> CreateNtupleDColumn("T_e_generated");
  anMan -> FinishNtuple();
  anMan -> CreateNtupleDColumn("TDC_SCINT_TOP");
  anMan -> CreateNtupleDColumn("TDC_SCINT_BOTTOM");
  anMan -> CreateNtupleDColumn("QDC_UpperPMT");
  anMan -> CreateNtupleDColumn("QDC_LowerPMT");
  anMan -> CreateNtupleDColumn("DL_X_Pos");
  anMan -> CreateNtupleDColumn("DL_Z_Pos");
  anMan -> CreateNtupleDColumn("TDC_ION_MCP");
  anMan -> CreateNtupleDColumn("TDC_ELECTRON_MCP");
  anMan -> CreateNtupleDColumn("recoil_mu_generated");
  // End making ntuples

  // Analysis Version 3! Spencer's Aggregator---should be IDENTICAL to what we
  // get from the analyzer.
  // the_aggregator_ = new Aggregator();

  RegisterChannel(qdc_upper_pmt_);
  RegisterChannel(qdc_lower_pmt_);
  RegisterChannel(dl_x_pos_);
  RegisterChannel(dl_z_pos_);
  for (G4int i = 0; i < 40; i++) {
    RegisterChannel(strip_detector_upper_x_[i]);
    RegisterChannel(strip_detector_upper_y_[i]);
    RegisterChannel(strip_detector_lower_x_[i]);
    RegisterChannel(strip_detector_lower_y_[i]);
  }
  RegisterChannel(tdc_scint_top_);
  RegisterChannel(tdc_scint_bottom_);
  RegisterChannel(tdc_ion_mcp_);
  RegisterChannel(tdc_electron_mcp_);
  RegisterChannel(tdc_photo_diode_);
  RegisterChannel(electron_kinetic_energy_generated_);
  RegisterChannel(electron_mu_generated_);
  RegisterChannel(recoil_mu_generated_);
  RegisterChannel(run_action_);
  RegisterChannel(tdc_long_electron_);
  RegisterChannel(tdc_nimio32_trigger_);
  RegisterChannel(tdc_tdc_stop_);
  RegisterChannel(recoil_mcp_particle_);
  RegisterChannel(tnim_op_beam_);
  RegisterChannel(ttlbit_sigmaplus_);
  the_aggregator_ -> RegisterIOMethod(configuration_filename_);
  // the_aggregator_ -> RegisterIOMethod("ScreenIO.mac");
  the_aggregator_ -> BeginRun();

  // test_channel_ -> InsertData(10.0);
  // the_aggregator_ -> EndEvent();

  // test_channel_ -> InsertData(15.0);
  // the_aggregator_ -> EndEvent();

  // test_channel_ -> InsertData(13.0);
  // the_aggregator_ -> EndEvent();
}

void K37RunAction::EndOfRunAction(const G4Run* aRun) {
  histograms->save();

  NbofEvents = aRun->GetNumberOfEvent();

  this->PrintResultsToScreen();
  this->PrintResultsToRunstat();
  if (recordAnnihilationPosition == true) {
    annihilationPosition->checkIfPrintIsNeeded(true);
  }
  if (recordVolumeNames == true) {
    listOfEnteredVolumes->checkIfPrintIsNeeded(true);
  }

  // JTW_Event *mytester = new JTW_Event();
  // FILE *f = fopen("RecheckA_beta.dat", "a");
  // if (!f) G4cout << "COULDN'T OPEN FILE" << G4endl;
  // G4int numIter = 25000;
  // for (G4int j = 0; j < numIter; j++) {
  //   G4int power = 2;
  //   for (G4int i = 0; i < 2*pow(10, power); i++) {
  //     mytester -> MakeEvent();
  //         if (i %  static_cast<int>(pow(10, power-1)) == 0) {
  //           // G4cout << "Gen " << i << G4endl;
  //         }
  //   }
  //   // G4cout << "Writing to file..." << G4endl;
  //   // G4cout << mytester -> GetNumPlus() << "\t" << mytester -> GetNumMins()
  //   //        << G4endl;
  //   fprintf(f, "%d\t%d\n", mytester -> GetNumPlus(),
  //           mytester -> GetNumMins());
  //   mytester -> ResetGeneratedCounters();
  //   if (j % 1000 == 0) {
  //     G4cout << j << G4endl;
  //   }
  // }

  // fclose(f);
  // G4EmCalculator emCalculator;
  // emCalculator.SetVerbose(1);
  // G4ParticleDefinition* particle = kinematic -> GetParticleGun() ->
  //   GetParticleDefinition();

  G4AnalysisManager *anMan = G4AnalysisManager::Instance();
  // save histograms
  //
  G4cout << "Closing histograms........." << G4endl;
  anMan->Write();
  anMan->CloseFile();
  G4cout << "Closed histograms." << G4endl;
  G4cout << "Total v/c N+ = " << plusZHits_vc << G4endl;
  G4cout << "Total v/c N- = " << minusZHits_vc << G4endl;
  delete G4AnalysisManager::Instance();

  the_aggregator_ -> EndRun();

  G4cout << "Last line of EORA" << G4endl;
}

void K37RunAction::PrintEnergyLossTable(G4String materialChoice) {
  // get the pointer to the material table
  const G4MaterialTable* theMaterialTable = G4Material::GetMaterialTable();
  G4bool materialIsOnTheList = false;
  // search the material by its name
  G4Material*materialSelectedFromTable;
  for (size_t J = 0 ; J < theMaterialTable -> size(); J++) {
    materialSelectedFromTable = (*theMaterialTable)[J];
    if (materialSelectedFromTable ->GetName() == materialChoice) {
      G4cout << materialSelectedFromTable -> GetName()
             << " is on the list of availble materials" << G4endl;
      G4cout << "The energy loss table of e+ in "
             << materialSelectedFromTable -> GetName()
             << " will be calculated" << G4endl;
      materialIsOnTheList = true;
      break;
    }
  }

  if (materialIsOnTheList == false) {
    G4cerr << "Material was not on list" << G4endl;
    return;
  }

  G4EmCalculator emCalculator;
  G4double test_energies[800];
  for (G4int i = 0; i < 800; i++) {
    test_energies[i]=G4double(i)*0.005;
  }

  std::ofstream energyLossTableFile;
  energyLossTableFile.open("energyLossTableFile.txt",
                           std::ofstream::out | std::ofstream::trunc);

  energyLossTableFile << "Energy(MeV)" << "    " << "DEDX(MeV/cm)" << G4endl;
  for (G4int i = 0; i < 800; i++) {
    energyLossTableFile << std::setw(15) << std::left << (test_energies[i])/MeV
                        << std::setw(15)
                        <<(emCalculator.GetDEDX((test_energies[i])*MeV,
                                                "e+",
                                                materialSelectedFromTable ->
                                                GetName(),
                                                "DefaultRegionForTheWorld"))/
      (MeV/cm) << G4endl;
  }
  energyLossTableFile.close();
  G4cout<< "energyLossTableFile.txt written." << G4endl;
}


void K37RunAction::PrintResultsToScreen() {
  G4cout << "------------------------------------------------------" << G4endl;
  G4cout << "  # Events ___________: " << NbofEvents << G4endl;
  G4cout << "  # Accepted _________: " << accepted<< G4endl;
  G4cout << "------------------------------------------------------" << G4endl;

  // G4cout << "  # ABSoffHoop _______: "
  //        << acceptedPrimaryScatteredOffHoops<< G4endl;
  // G4cout << "  # Acc/Eve __________: "
  //        << (G4double(acceptedWithNone)/G4double(NbofEvents)) << G4endl;
  // G4cout << "  # NV _______________: " << nonVetoed<< G4endl;
  // G4cout << "  # NV/Eve ___________: "
  //        << (G4double(nonVetoed)/G4double(NbofEvents)) << G4endl;
  // G4cout << "  # FalsePositive ____: " << falsePositiveWithNone << G4endl;
  // G4cout << "  # Acc With Sili2 ___: " << acceptedWithSili2 << G4endl;
  // G4cout << "  # Acc With Dedx2 ___: " << acceptedWithDedx2 << G4endl;
  // G4cout << "  # Sec Fired Dedx__: " << secondaryInDedx << G4endl;
  // G4cout << "  # Sec Fired Both__: " << secondaryInBoth << G4endl;
  // G4cout << "  # FP With Sili _____: " << falsePositiveWithSili << G4endl;
  // G4cout << "  # FP With Dedx _____: " << falsePositiveWithDedx << G4endl;
  // G4cout << "  # Fired All 1 & 2 __: " << falsePositiveWithBoth << G4endl;
  // G4cout << "  # BackScatter ______: " << bs << G4endl;
  // G4cout << "  # NV BS ____________: " << accepted_bs << G4endl;
  // G4cout << "  # FP With None ___: " << falsePositiveWithNone << G4endl;
  // G4cout << "  # BS/Eve _________: " << (G4double(bs)/G4double(NbofEvents))
  //        << G4endl;
  // G4cout << "  # ABS/Eve ________: "
  //        << (G4double(accepted_bs)/G4double(NbofEvents)) << G4endl;
  // G4cout << "  # ABS/BS _________: " << (G4double(accepted_bs)/G4double(bs))
  //        << G4endl;

  // G4cout << "----------------------------------------------------" << G4endl;
  // G4cout << "  # Accepted With No 2 _______________________: "
  //        << acceptedWithNone<< G4endl;
  // G4cout << "  # Accepted With No 2 Vetoed_________________: "
  //        << vetoed<< G4endl;
  // G4cout << "  # Accepted With No 2 Non-Vetoed_____________: "
  //        << nonVetoed<< G4endl;
  // G4cout << "  # Accepted With No 2 Secondary Fired Dedx __: "
  //        << acceptedWithNoneSecondaryFiredDedx<< G4endl;
  // G4cout << "  # Accepted With No 2 Secondary Fired Sili __: "
  //        << acceptedWithNoneSecondaryFiredSili<< G4endl;
  // G4cout << "  # Accepted With No 2 Secondary Fired Both __: "
  //        << acceptedWithNoneSecondaryFiredBoth<< G4endl;
  // G4cout << "  # Accepted With No 2 Primary Fired Both ____: "
  //        << acceptedWithNonePrimaryFiredBoth<< G4endl;
  // G4cout << "----------------------------------------------------" << G4endl;
  // G4cout << "  # FP With No 1 ________________________: "
  //        << falsePositiveWithNone<< G4endl;
  // G4cout << "  # FP With No 1 Secondary Fired Dedx2 __: "
  //        << falsePositiveWithNoneSecondaryFiredDedx<< G4endl;
  // G4cout << "  # FP With No 1 Secondary Fired Sili2 __: "
  //        << falsePositiveWithNoneSecondaryFiredSili<< G4endl;
  // G4cout << "  # FP With No 1 Secondary Fired Both2 __: "
  //        << falsePositiveWithNonePrimaryFiredBoth<< G4endl;
  // G4cout << "  # FP With No 1 Primary Fired Both2 ____: "
  //        << falsePositiveWithNoneSecondaryFiredBoth<< G4endl;
  // G4cout << "----------------------------------------------------" << G4endl;
  // G4cout << "  # Accepted With Dedx2 _________________________________: "
  //        << acceptedWithDedx2 << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Dedx ____________: "
  //        << acceptedWithDedx2SecondaryFiredDedx << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Sili ____________: "
  //        << acceptedWithDedx2SecondaryFiredSili << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Dedx2 ___________: "
  //        << acceptedWithDedx2SecondaryFiredDedx2 << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Sili And Dedx2 __: "
  //        << acceptedWithDedx2SecondaryFiredSiliAndDedx2 << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Dedx And Sili ___: "
  //        << acceptedWithDedx2SecondaryFiredDedxAndSili << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired Dedx And Dedx2 __: "
  //        << acceptedWithDedx2SecondaryFiredDedxAndDedx2 << G4endl;
  // G4cout << "  # Accepted With Dedx2 Secondary Fired All _____________: "
  //        << acceptedWithDedx2SecondaryFiredAll << G4endl;
  // G4cout << "  # Accepted With Dedx2 Primary Fired All _______________: "
  //        << acceptedWithDedx2SPrimaryFiredAll << G4endl;
  // G4cout << "-------------------------------" << G4endl;
}

void K37RunAction::RegisterChannel(K37_Data* channel) {
  (*active_channels_)[channel->GetName()] = channel;
  the_aggregator_ -> RegisterData(channel);
}

void K37RunAction::PrintResultsToRunstat() {
  std::ofstream runstat;
  runstat.open("runstat.txt", std::ofstream::out | std::ofstream::app);
  runstat << "--------------------------------------------------------"
          << G4endl;
  runstat << "  # Events ___________: " << NbofEvents  << G4endl;
  runstat << "  # Accepted _________: " << accepted << G4endl;
  runstat << "--------------------------------------------------------"
          << G4endl;
  runstat.close();


  // runstat << "  # ABSoffHoop _______: " << acceptedPrimaryScatteredOffHoops
  //         << G4endl;
  // runstat << "  # Events _______: " << NbofEvents << G4endl;
  // runstat << "  # Accepted _____: " << accepted << G4endl;
  // runstat << "  # Acc/Eve ______: "
  //         << (G4double(accepted)/G4double(NbofEvents)) << G4endl;
  // runstat << "  # Sec Fired SiLi: " << secondaryInSiLi << G4endl;
  // runstat << "  # Sec Fired Dedx: " << secondaryInDedx << G4endl;
  // runstat << "  # Sec Fired Both: " << secondaryInBoth << G4endl;
  // runstat << "  # BackScatter __: " << bs << G4endl;
  // runstat << "  # FalsePositive_: " << falsePositive  << G4endl;
  // runstat << "  # BS/Eve _______: " << (G4double(bs)/G4double(NbofEvents))
  //         << G4endl;
  // runstat << "  # Accepted BS __: " << accepted_bs << G4endl;
  // runstat << "  # ABS/Eve ______: "
  //         << (G4double(accepted_bs)/G4double(NbofEvents)) << G4endl;
  // runstat << "  # ABS/BS _______: " << (G4double(accepted_bs)/G4double(bs))
  //         << G4endl;
  // runstat << "  # Acc/Eve __________: "
  //         << (G4double(acceptedWithNone)/G4double(NbofEvents)) << G4endl;
  // runstat << "  # NV _______________: " << nonVetoed << G4endl;
  // runstat << "  # NV/Eve ___________: "
  //         << (G4double(nonVetoed)/G4double(NbofEvents))  << G4endl;
  // runstat << "  # FalsePositive ____: " << falsePositiveWithNone  << G4endl;
  // runstat << "  # Acc With Sili2 ___: " << acceptedWithSili2  << G4endl;
  // runstat << "  # Acc With Dedx2 ___: " << acceptedWithDedx2  << G4endl;
  // runstat << "  # FP With Sili _____: " << falsePositiveWithSili  << G4endl;
  // runstat << "  # FP With Dedx _____: " << falsePositiveWithDedx  << G4endl;
  // runstat << "  # Fired All 1 & 2 __: " << falsePositiveWithBoth  << G4endl;
  // runstat << "  # BackScatter ______: " << bs  << G4endl;
  // runstat << "  # NV BS ____________: " << accepted_bs  << G4endl;
  // runstat << "--------------------------------------------------------"
  //         << G4endl;
  // runstat << "  # Accepted With No 2 _______________________: "
  //         << acceptedWithNone << G4endl;
  // runstat << "  # Accepted With No 2 Vetoed_________________: " << vetoed
  //         << G4endl;
  // runstat << "  # Accepted With No 2 Non-Vetoed_____________: " << nonVetoed
  //         << G4endl;
  // runstat << "  # Accepted With No 2 Secondary Fired Dedx __: "
  //         << acceptedWithNoneSecondaryFiredDedx << G4endl;
  // runstat << "  # Accepted With No 2 Secondary Fired Sili __: "
  //         << acceptedWithNoneSecondaryFiredSili << G4endl;
  // runstat << "  # Accepted With No 2 Secondary Fired Both __: "
  //         << acceptedWithNoneSecondaryFiredBoth << G4endl;
  // runstat << "  # Accepted With No 2 Primary Fired Both ____: "
  //         << acceptedWithNonePrimaryFiredBoth << G4endl;
  // runstat << "--------------------------------------------------------"
  //         << G4endl;
  // runstat << "  # FP With No 1 ________________________: "
  //         << falsePositiveWithNone << G4endl;
  // runstat << "  # FP With No 1 Secondary Fired Dedx2 __: "
  //         << falsePositiveWithNoneSecondaryFiredDedx << G4endl;
  // runstat << "  # FP With No 1 Secondary Fired Sili2 __: "
  //         << falsePositiveWithNoneSecondaryFiredSili << G4endl;
  // runstat << "  # FP With No 1 Secondary Fired Both2 __: "
  //         << falsePositiveWithNonePrimaryFiredBoth << G4endl;
  // runstat << "  # FP With No 1 Primary Fired Both2 ____: "
  //         << falsePositiveWithNoneSecondaryFiredBoth << G4endl;
  // runstat << "--------------------------------------------------------"
  //         << G4endl;
}
