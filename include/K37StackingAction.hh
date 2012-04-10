
#ifndef K37StackingAction_h
#define K37StackingAction_h 1

#include "G4UserStackingAction.hh"
#include "globals.hh"
//#include "K37ScintillatorMinusZHit.hh"
#include "K37ScintillatorHit.hh"
//#include "K37StripDetectorMinusZHit.hh"
#include "K37StripDetectorHit.hh"

class G4Track;

//-----------------------------------------------

class K37StackingAction : public G4UserStackingAction
{
  public:

    K37StackingAction();
   ~K37StackingAction();

    G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* );
    void NewStage();
    void PrepareNewEvent();

 private:
    K37ScintillatorHitsCollection* scintillatorPlusZHits;
    K37ScintillatorHitsCollection* scintillatorMinusZHits;
    K37StripDetectorHitsCollection* stripDetectorPlusZHits;
    K37StripDetectorHitsCollection* stripDetectorMinusZHits;
    G4VHitsCollection* GetCollection(G4String colName);

    G4bool GetPointersToHitCollections();
    
};

#endif
