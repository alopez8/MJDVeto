// For analysis of the new MJD built data format.
// Goal is to give an example of accessing every data member.
// Run in compiled mode with .x vetoReader.C++
// 
// Clint Wiseman, University of South Carolina
// Andrew Lopez, UTK/Majorana
// 11/16/2015

/*
	File components:
	1. headerXML
	2. ProcessID
	3. builderID0
	4. MGTree
	5. MGGarbageTree
	6. ChannelMap
	7. ChannelSettings
	8. VetoTree
*/

#ifndef __CINT__
//#include <vector>
#include <iostream>
//#include <fstream>
//#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
//#include "TChain.h"
//#include "TEntryList.h"
//#include "TBranch.h"
//#include "TH1.h"
//#include "CLHEP/Units/SystemOfUnits.h"
//#include "GATMultiplicityProcessor.hh"
#include "GATDataSet.hh"
//#include "MGTWaveform.hh"
#include "MJTRun.hh"
#include "MJTVetoData.hh"
#include "MGTBasicEvent.hh"
using namespace std;
//using namespace CLHEP;
#endif

Char_t TheFile[200];
const char *run_dir = "P3KJR"; //string name of run directory
Int_t Run_to_investigate = 9729;
const int numPanels = 32;
TH1F *hRawQDC[numPanels];  
TH1F *hLEDCutQDC[numPanels];
TH1F *hMuonCutQDC[numPanels];

void builtVetoanalysis(){

//define global variables
	Int_t ledcount = 0;
	Int_t muoncount = 0;
	Float_t duration = 0;
	Int_t run = 0;


//led (low) qdc threshold values from findThresh.C
		Int_t ledthresh[numPanels] = {136, 129, 115, 108, 172, 129, 129, 122, 129, 108, 122, 115, 108, 115, 108, 186, 65, 165, 100, 136, 93, 100, 143, 79, 136, 115, 93, 122, 158, 172, 129, 93};
		
//muon (high) qdc threshold values
		Int_t muonthresh[numPanels] = {0};

		for (Int_t k=0; k<numPanels; k++){
			muonthresh[k] = 500;
		}
		
		Char_t InputName[200] = "builtVetoanalysis";
		
		// Set up output file(s)
		Char_t OutputFile[200];
		sprintf(OutputFile,"%s.root",InputName);
		TFile *RootFile = new TFile(OutputFile, "RECREATE");	
		TH1::AddDirectory(kFALSE); // Global flag: "When a (root) file is closed, all histograms in memory associated with this file are automatically deleted."
	
		ofstream vastats;
		vastats.open ("va_stats.txt");
		
		Char_t hname[50];
		
		const Int_t nqdc_bins=1400;  // this gives 3 qdc / bin
		const Float_t ll_qdc=0.;
		const Float_t ul_qdc=4300.;
		
		for (Int_t i=0; i<numPanels; i++){
				sprintf(hname,"hRawQDC%d",i);
				hRawQDC[i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
				sprintf(hname,"hLEDCutQDC%d",i);
				hLEDCutQDC[i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
				sprintf(hname,"hMuonCutQDC%d",i);
				hMuonCutQDC[i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
		}

	// Initialize with standard ROOT methods	
	sprintf(TheFile,"/global/project/projectdirs/majorana/data/mjd/surfmjd/data/built/%s/OR_run%u.root",run_dir,Run_to_investigate);
	TFile *f = new TFile(TheFile);

	TTree *v = (TTree*)f->Get("VetoTree");
	TTree *b = (TTree*)f->Get("MGTree");
	Long64_t nentries = v->GetEntries();
	cout << "Found " << nentries << " entries." << endl;
	//v->Show(nentries-1);	// for comparison
	//b->Show(0);	// for comparison

	// Initialize by GATDataSet
	// Requires GetVetoChain, which has not been merged into GAT yet.
	/*
	GATDataSet ds(6947);
	TChain *b = ds.GetBuiltChain();
	TChain *v = ds.GetVetoChain();	// Clint's super-fancy addition
	Long64_t nentries = v->GetEntries();
	cout << "Found " << nentries << " veto entries." << endl;
	//v->Show(nentries-1);	// for comparison
	//b->Show(0); // for comparison

	// Must initialize with GATDataSet to use these functions
	// Does not seem to contain veto data.
	// It would be nice to see the HV map information ...
	MJTChannelMap *map = ds.GetChannelMap();
	//map->DumpChannelMap();
	MJTChannelSettings *set = ds.GetChannelSettings();
	//set->DumpSettings();
	//set->DumpEnabledIDs();
	vector<uint32_t> en = set->GetEnabledIDList(); 	// save a vector with the enabled Ge detector list
	*/

	cout << "=================================================================" << endl;

	MJTRun *VetoRun = new MJTRun();
	MGTBasicEvent *vetoEvent = new MGTBasicEvent(); 
	UInt_t mVeto = 0;
	uint32_t vBits = 0;

	
	v->SetBranchAddress("run",&VetoRun);
	v->SetBranchAddress("mVeto",&mVeto);
	v->SetBranchAddress("vetoEvent",&vetoEvent);
	VetoTree->SetBranchAddress("vetoBits",&vBits);
	
	//define vBit counters
	Int_t kmccount = 0;

	
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//loop over VetoTree entries	
for (int z = 0; z < nentries; z++){
	
	//define single entry variables
	Int_t lednumPanelsHit = 0;
	Int_t muonnumPanelsHit = 0;
	Bool_t isLED = false; //to mark whether an event is an LED or not
	

	
	v->GetEntry(z);
	
	//veto error bits
//	if (MJBits::GetBit(vBits, MJVetoBits::kMissingChannels())) kmccount++;
		
	//cout << "z = " << z << endl;
	//v->GetEntry(nentries-1);

	// =========================================================================
	// 1. "run"
	// Both MGTree and VetoTree now contain a MJTRun object. (MJTRun.hh)
	
	// This is "packed", returns e.g. 2576980377.
	// Shouldn't need to unpack it, since we can do the test below.
	uint32_t runbits = VetoRun->GetRunBits(); 
	
	// Prints a list of enabled run bits with names to cout.
	cout << "Enabled run bits: " << endl;
	VetoRun->ListRunBits();	

	// Other bits are listed in $MGDODIR/Majorana/MJTypes.hh 
	// Returns a bool (0 or 1)
	cout << "Test for a run bit - Shop Activity: " << VetoRun->GetRunBit(MJRunBits::kMachineShop) << endl; 

	// Unsigned int from MGTypes.hh -- kData=0, kTest=1, kCalibration=2, kMC=3, kUndefined=4																	  
	printf("Run Type: %u\n",VetoRun->GetRunType());  
	printf("Run Number: %i\n",VetoRun->GetRunNumber());
	run = VetoRun->GetRunNumber();

	// Set in /MJOR/MOVetoDataLoader.cc:    fRun->SetRunDescription("Orca run");
	// Returns a string.
	cout << "Run Description: " << VetoRun->GetRunDescription() << endl;  
	
	// Returns time_t values of unix timestamps.
	// Built-in function uses mktime, so would have been set to 
	//   whatever the local unix time was when the built file was created.
	// Need to compare against e.g. the run database to trust these as 
	//   absolute start/stop times in a known time zone.
	printf("Start time:%li  Stop time: %li\n",VetoRun->GetStartTime(),VetoRun->GetStopTime()); 
	
	duration = VetoRun->GetStopTime() - VetoRun->GetStartTime();
	printf("Duration: %li seconds\n",duration);

	// Both of these return strings.
	// "undefined" is the default in MGRun.cc
	cout << "Parent DAQ label: " << VetoRun->GetParentDAQLabel() << endl;
	cout << "MGDO Conversion Version: " << VetoRun->GetMGDOConversionVersion() << endl;

	// =========================================================================

	// 2. "mVeto"
	// Built-in multiplicity is calculated in MOVetoDataLoader.cc:
	// if(!((MJTVetoData*)(fVetoEvent->GetDetectorData(i)))->IsUnderThreshold())
	// So it's dependent on the IsUnderThreshold software tag set in ORCA.
	//   which is QDC = 500.
	printf("mVeto: %u\n",mVeto);

	// =========================================================================

	// 3. "vetoEvent"
	// Veto data is stored in an MGTBasicEvent filled with MJTVetoData objects.
	
	// From adding all QDC amplitudes in MOVetoDataLoader.cc
	// It looks like it would add the QDC pedestal values as well, which are unphysical
	printf("Total energy? (QDC+pedestal): %.1f \n",vetoEvent->GetETotal());

	// Returns a double: time since the run start (in ns?)
	printf("Time (ns?): %.1f \n",vetoEvent->GetTime());

	// Set in MGTypes.hh -- kReal=0, kPulser=1, kMC=2, kUndefined=3
	printf("Veto event type: %u\n",vetoEvent->GetEventType());

	// Make sure NDetectorData == 32.
	// Returns a size_t.
	printf("NDetectorData: %lu\n",vetoEvent->GetNDetectorData());
	if (vetoEvent->GetNDetectorData() != 32) 
		cout << "Warning! Detector Data is not 32, it's " << vetoEvent->GetNDetectorData() << endl;
	
	// Access the MJTVetoData objects "vd"
	MJTVetoData *vd[numPanels];
	for (int i=0; i<numPanels; i++) { vd[i] = dynamic_cast<MJTVetoData*>(vetoEvent->GetDetectorData()->At(i)); }

	// These values should be the same for every MJTVetoData object in the array
	// Check: If EventCount doesn't match ScalerCount, something has gone wrong.
	printf("Crate: %i  EventCount:%i  ScalerCount: %i\n",
		vd[0]->GetCrate(),vd[0]->GetEventCount(),vd[0]->GetScalerCount());
	printf("ScalerID: %i  ScalerIndex:%llu  TimeStamp: %llu  IsBadTs: %i \n",
		vd[0]->GetScalerID(),vd[0]->GetScalerIndex(),vd[0]->GetTimeStamp(),vd[0]->IsBadTS());
	
	if (vd[0]->GetEventCount() != vd[0]->GetScalerCount()) 
		printf("Warning!  EventCount and ScalerCount don't match!\n");
	
	// The scaler clock is 100MHz = 1E8 counts / sec
	printf("Scaler Time (sec): %.8f\n",vd[0]->GetTimeStamp()/1E8);
/*
	// Display raw data (out of order)
	//   Member functions from MJTVetoData.hh and MGDetectorData.hh (MJTVetoData's base class)
	//   fIndex (ORCA packet number, useful for consistency checks)
	//   fID ("packed" channel number, just like Ge detectors)
	printf("Unsorted MJTVetoData:\n");
	for (int j = 0; j<32; j++)	{
		if (vd[j]){
			printf("Ca: %-3i  Ch: %-3i  Amp: %-5.0f  UTh: %-3i  OF: %-3i  ID: %-5i  Index: %-5llu\n",
				vd[j]->GetCard(),vd[j]->GetChannel(),vd[j]->GetAmplitude(),vd[j]->IsUnderThreshold(),
				vd[j]->IsOverflow(),vd[j]->GetID(),vd[j]->GetIndex());
		}
	}
*/	
	cout << endl;
//********************************************************************
//sort data into arrays
	// Sort raw data into arrays and then display.
	// This may not be totally necessary, but makes hit pattern analysis easier
	//   to match to the physical veto panel locations, and Yuri's wiring diagrams.
	// Most things are cast to int's.  
	// Original types can be found in MJTVetoData.hh and MGDetectorData.hh if necessary.
	const int card1 = 13;
	const int card2 = 18;
	int Card[numPanels] = {0};
	int QDC[numPanels] = {0};
	int IsUnderThreshold[numPanels] = {0};
	int IsOverflow[numPanels] = {0};
	int ID[numPanels] = {0};
	long Index[numPanels] = {0};
	int k = 0;
	for (int j = 0; j<numPanels; j++)	{
		if (vd[j]){
			k = vd[j]->GetChannel();	// goes from 0 to 15
			if (vd[j]->GetCard() == card1) {
				Card[k] = vd[j]->GetCard();
				QDC[k] = (int)vd[j]->GetAmplitude();
				IsUnderThreshold[k] = (int)vd[j]->IsUnderThreshold();
				IsOverflow[k] = (int)vd[j]->IsOverflow();
				ID[k] = vd[j]->GetID();
				Index[k] = (Long_t)vd[j]->GetIndex();
			}
			else if (vd[j]->GetCard() == card2) {
				Card[16+k] = vd[j]->GetCard();
				QDC[16+k] = (int)vd[j]->GetAmplitude();
				IsUnderThreshold[16+k] = (int)vd[j]->IsUnderThreshold();
				IsOverflow[16+k] = (int)vd[j]->IsOverflow();	
				ID[16+k] = vd[j]->GetID();
				Index[16+k] = (Long_t)vd[j]->GetIndex();
			}
		}
	}
/*
	printf("Sorted MJTVetoData:\n");
	for (int j = 0; j < 32; j++) {
		printf("Ca: %-3i  Panel: %-3i  Amp: %-5i  UTh: %-3i  OF: %-3i  ID:%-5i  Index:%-5li\n",
			Card[j],j,QDC[j],IsUnderThreshold[j],IsOverflow[j],ID[j],Index[j]);
	}

	// Display ORCA packet numbers: Scaler, QDC Card 1, QDC Card 2
	printf("ORCA Packet Numbers - Scaler: %llu  QDC 1 (Ch.%i): %li  QDC 2 (Ch.%i): %li\n",
		vd[0]->GetScalerIndex(),Card[0],Index[0],Card[31],Index[31]);
*/	
	// Check: QDC index should be no more than 2 greater than scaler index.
	// The event builder has supposedly already done this check.
	if ((Long_t) vd[0]->GetScalerIndex() > Index[0] || (Long_t)vd[0]->GetScalerIndex() > Index[31])
		printf("Warning! Scaler index is larger than QDC index!\n");
	

	// =========================================================================
//*******************************************************8

//begin analysis	
	for (int k = 0; k<numPanels; k++){
		
		hRawQDC[k]->Fill(QDC[k]);

				
		if (QDC[k] > ledthresh[k]) lednumPanelsHit++;
		if (QDC[k] > muonthresh[k]) muonnumPanelsHit++;
	}
		cout << "lednumPanelsHit = " << lednumPanelsHit << endl;
		cout << "muonnumPanelsHit = " << muonnumPanelsHit << endl;
		
	if (lednumPanelsHit >= 20){
		isLED = true; // if true, marks the signal as an LED
		ledcount += 1;
		for (int k=0; k<numPanels; k++){
			if (QDC[k]>ledthresh[k]){
				hLEDCutQDC[k]->Fill(QDC[k]);								
			}
		}
	}	
	
	if (!isLED && muonnumPanelsHit > 2 && muonnumPanelsHit < 20){
		muoncount += 1;
		for (Int_t k=0; k<numPanels; k++){
			if(QDC[k]>muonthresh[k]){
				hMuonCutQDC[k]->Fill(QDC[k]);
			}

		}			
	}
/*	
	if (lednumPanelsHit >24) {
		vastats << "nentry " << z << " hit " << lednumPanelsHit << " panels." << endl;
		for (int k=0; k<numPanels; k++){
			vastats << "Panel k: " << k << " | QDC[k] = " << QDC[k] << endl;
		}
	}
	
	if (z == 171) {
		vastats << "nentry " << z << " hit " << lednumPanelsHit << " panels." << endl;
		for (int k=0; k<numPanels; k++){
			vastats << "Panel k: " << k << " | QDC[k] = " << QDC[k] << endl;
		}
	}
*/	
} // End loop over VetoTree entries.
	cout << "ledcount = " << ledcount << endl;
	cout << "muoncount = " << muoncount << endl;
	
//output

	TDirectory *rawqdc = RootFile->mkdir("RawQDC");
	TDirectory *ledcutqdc = RootFile->mkdir("LEDCutQDC");
	TDirectory *muoncutqdc = RootFile->mkdir("MuonCutQDC");

		for (Int_t i=0; i<numPanels; i++){
		
			RootFile->cd("RawQDC");
			hRawQDC[i]->Write();		// write the raw QDC without fitting

			
			RootFile->cd("LEDCutQDC");
			hLEDCutQDC[i]->Write();		
		
			RootFile->cd("MuonCutQDC");
			hMuonCutQDC[i]->Write();	
		}
	
	RootFile->cd();

	// ==========================

	RootFile->Close();

	cout << "Wrote ROOT file." << endl;
	
	vastats << "run # = " << run << " seconds" << endl;
	vastats << "ledcount = " << ledcount << endl;
	vastats << "muoncount = " << muoncount << endl;
	vastats << "duration = " << duration << endl;
	vastats << "nentries = " << nentries << endl;
	vastats << "# of entries that flip kMissingChannels bit = " << kmccount << endl;
	vastats.close();
	
//	ofstream lowdt;
//	lowdt.open ("great.txt");
//	lowdt << "It worked!" << endl;
//	lowdt.close();
	
} //end of program