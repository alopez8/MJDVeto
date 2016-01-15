/*
	builtVeto.C
	Clint Wiseman, USC/Majorana
	Andrew Lopez, UTK/Majorana
	June 2015.
	
 => This code can be run on PDSF.  It takes a .txt file of run numbers
	as an input argument, and uses the name of the text file to generate output
	in the folder ./output

 => Recommended: When scanning a new input file of run numbers on PDSF, run CheckFiles.C 
 	to make sure files exist and have not been blinded.  
 	This code has a tendency to quit unexpectedly when it encounters a "bad" file.
 
 => builtVeto uses the same sorting method developed in builtVetoSimple.  
 	It is then mainly used for plotting scaler corruption and multiplicity of events.
	Generates ROOT files of histograms, allowing one to look at run-by-run
	scaler corruption in time, and run-by-run multiplicity to look for 
	changes in the system.

 => builtVetoCal.C is a bit more advanced, and (among other things) uses
	custom threshold values for each veto panel.

	Usage:
	CINT: root[0] .X builtVeto.C ("Filename_list_of_run_numbers")  <--- NO .TXT extension.
	bash: root -b -q -l builtVeto.C ("The_filename_without_extension")
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
#include <new>
//using namespace CLHEP;
#endif

const int numPanels = 32;
const int numFiles = 1629;
const int runmin = 3137; //first run number in debug list
const int runmax = 7274; //last run number in debug list
const int runspace = runmax - runmin + 1; //how many runs between runmax and runmin


// global pointers for qdc histograms.
TH1F *hRawQDC[numFiles][numPanels];  
TH1F *hLEDCutQDC[numFiles][numPanels];
TH1F *hMuonCutQDC[numFiles][numPanels];
TH1F *ledTime[numFiles];
TH1F *hMuonMult[numPanels];
TH1F *hMuonManyQDC[numPanels];
TH1F *hLEDAvgQDCPanel[numPanels];
TH1F *hLEDrmsQDCPanel[numPanels];
TGraph *ledtimestamp[numFiles];

void builtVetoMult2reader(string Input = ""){

	int card1 = 13;
	int card2 = 18;
	Bool_t useThresh = true; // if true, also enables fitting LED peaks
	
	
	//led (low) qdc threshold values from findThresh.C
		Int_t ledthresh[numPanels] = {136, 129, 115, 108, 172, 129, 129, 122, 129, 108, 122, 115, 108, 115, 108, 186, 65, 165, 100, 136, 93, 100, 143, 79, 136, 115, 93, 122, 158, 172, 129, 93};
	
		//muon (high) qdc threshold values
		Int_t muonthresh[numPanels] = {0};

		for (Int_t k=0; k<numPanels; k++){
			muonthresh[k] = 500;
		}

	
	// Input a list of run numbers
	if (Input == "") Char_t InputName[200] = "builtVeto_DebugList";
	else Char_t InputName[200] = Input.c_str();
	Char_t InputFile[200];
	sprintf(InputFile,"%s.txt",InputName);
	ifstream InputList;
	InputList.open(InputFile);
	Char_t TheFile[200];
	
	// Set up output file(s)
	Char_t OutputFile[200];
	sprintf(OutputFile,"%s.root",InputName);
	TFile *RootFile = new TFile(OutputFile, "RECREATE");	
  	TH1::AddDirectory(kFALSE); // Global flag: "When a (root) file is closed, all histograms in memory associated with this file are automatically deleted."
	
	ofstream lowdt;
	lowdt.open ("zerodeltat.txt");

	//=== Global counters / variables / plots ===

		Int_t run = 0;
		Float_t duration = 0;

		const Int_t nqdc_bins=1400;  // this gives 3 qdc / bin
		const Float_t ll_qdc=0.;
		const Float_t ul_qdc=4300.;

		Float_t Tnew = 0;
		Float_t Told = 0;
		Float_t deltaT = 0;

		Float_t totdur = 0;
		Int_t badtimecount = 0;
		Int_t baddurcount = 0;

		Float_t ledcount[numFiles] = {0};
		Float_t ledcountmax[numFiles] = {0};
		Float_t ledtimemax[numFiles] = {0}; 
		Float_t totledcount = 0;

		Float_t muoncount[numFiles] = {0};
		Float_t totmuoncount = 0;
		Int_t omitteddur[numFiles] = {0};


		Float_t ledQDCcount[numFiles][numPanels] = {0};
		Float_t totledQDC[numFiles][numPanels] = {0};
		Float_t quadtotledQDC[numFiles][numPanels] = {0};
		Float_t AvgledQDC[numFiles][numPanels] = {0};
		Float_t rmsledQDC[numFiles][numPanels] = {0};
		Float_t AvgFileLEDQDC[numFiles] = {0};
		Float_t totFileLEDQDC[numFiles] = {0};
		Float_t totFileledQDCcount[numFiles] = {0};
		Float_t slope[numFiles] = {0}; //slope of # leds vs duration
		Int_t runnum[numFiles] = {0}; //array of run numbers
		
		// get number of files in dataset for the TGraph
		Int_t filesToScan = 0;
		Int_t filesScanned = 0;
	  	while(true) { 
			InputList >> run; 
			if (InputList.eof()) 		break;
			filesToScan++; 
		}
	  	cout << "Scanning " << numFiles << " files." << endl;	  	
	  	InputList.close();
	  	InputList.open(InputFile);
	  	run=0;
	 	TGraph *SCorruption = new TGraph(filesToScan);
	 	Int_t BadTSTotal = 0;
		if (filesToScan != numFiles){
			cout << " Error: number of files in builtVetoMult.C does not equal number of files in builtVeto_DebugList.txt" << endl;
			break;
		}

	 	TH1D *TotalCorruptionInTime = new TH1D("TotalCorruptionInTime","corrupted entries during run (entry method)",(Int_t)3600/5,0,3600);
	 	TotalCorruptionInTime->GetXaxis()->SetTitle("time (5 sec / bin)");
	 	Bool_t PlotCorruptedEntries = true; // flag for plotting corrupted entries in time for EACH RUN

	 	TH1D *TotalMultiplicity = new TH1D("TotalMultiplicity","Events over threshold",32,0,32);
	 	TotalMultiplicity->GetXaxis()->SetTitle("number of panels hit");
	 	Bool_t PlotMultiplicity = true;	// flag to plot multiplicity for EACH RUN

		Int_t ledcut = 20;
		TH1D *hLEDCutMult = new TH1D("hLEDCutMult","LED Events over threshold (ledcut = 20) ",32,0,32);
	 	hLEDCutMult->GetXaxis()->SetTitle("number of panels hit");

		TH1D *hLEDCutMulttest = new TH1D("hLEDCutMult","LED Events over threshold (ledcut = 2) ",32,0,32);
	 	hLEDCutMulttest->GetXaxis()->SetTitle("number of panels hit");		

		TH1D *hLEDCutDT = new TH1D("hLEDCutDT","Delta T between LED Events over threshold (ledcut = 20) ",20000,0,20);
	 	hLEDCutDT->GetXaxis()->SetTitle("time (seconds)");
		
		TH1D *hLEDCutDTFocus = new TH1D("hLEDCutDTFocus","Delta T between LED Events over threshold (ledcut = 20)",100000,7,8); //graph DT peak with high resolution
		hLEDCutDTFocus->GetXaxis()->SetTitle("time (seconds)");

		TH1D *hLEDpertime = new TH1D("hLEDpertime","LED events vs run duration (ledcut = 10) ",3600,0,3600);
	 	hLEDpertime->GetXaxis()->SetTitle("run duration");
		hLEDpertime->GetYaxis()->SetTitle("# of LED events");

		TH1D *hAvgMuonQDC = new TH1D("hAvgMuonQDC","Muon Multiplicity vs Muon Avg QDC value",1000,0,5000);
	 	hAvgMuonQDC->GetXaxis()->SetTitle("Muon Avg QDC value");
		hAvgMuonQDC->GetYaxis()->SetTitle("Multiplicity");

	
		Char_t hname[50];
/*		
		for (Int_t j=0; j<numFiles; j++){
			
			for (Int_t i=0; i<numPanels; i++){
				sprintf(hname,"File%dhRawQDC%d",j,i);
				hRawQDC[j][i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
				sprintf(hname,"File%dhLEDCutQDC%d",j,i);
				hLEDCutQDC[j][i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
				sprintf(hname,"File%dhMuonCutQDC%d",j,i);
				hMuonCutQDC[j][i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);
			}
		}
*/
		for (Int_t i=0; i<numFiles; i++){
			sprintf(hname,"ledTime_run%d",i);
			ledTime[i] = new TH1F(hname,hname,nqdc_bins,ll_qdc,ul_qdc);

		}

		for (Int_t i=0; i<numPanels; i++){
			sprintf(hname,"MuonMultiplicityGT%d",i);
			hMuonMult[i] = new TH1F(hname,hname,32,0,32);
			sprintf(hname,"MuonManyTrigQDCPanel%d",i);
			hMuonManyQDC[i] = new TH1F(hname,hname,3600,0,3600);
			sprintf(hname,"hLEDAvgQDCPanel%d",i);
			hLEDAvgQDCPanel[i] = new TH1F(hname,hname,runspace,runmin,runmax);
			sprintf(hname,"hLEDrmsQDCPanel%d",i);
			hLEDrmsQDCPanel[i] = new TH1F(hname,hname,runspace,runmin,runmax);
		}

	//=== End ===


	// Loop over files
	while(true){

		// initialize 
		InputList >> run;
		if (InputList.eof()) 		break;
		sprintf(TheFile,"/global/project/projectdirs/majorana/data/mjd/surfmjd/data/built/P3JDY/OR_run%u.root",run); 	
		TChain *VetoTree = new TChain("VetoTree");
		VetoTree->AddFile(TheFile);
		Long64_t nentries = VetoTree->GetEntries();
		
		MJTRun *VetoRun = new MJTRun();
		MGTBasicEvent *vetoEvent = new MGTBasicEvent();
		UInt_t mVeto = 0;
		
		//set branch addresses
		VetoTree->SetBranchAddress("run",&VetoRun);
		VetoTree->SetBranchAddress("mVeto",&mVeto);
		VetoTree->SetBranchAddress("vetoEvent",&vetoEvent);
	
	

		// Unsigned int from MGTypes.hh -- kData=0, kTest=1, kCalibration=2, kMC=3, kUndefined=4																	  
		printf("Run Type: %u\n",VetoRun->GetRunType());  
		
//-------------

    	//=== Single-file counters / variables / plots

			Bool_t IsEmpty = false;

			Int_t BadTSInFile = 0;
			Float_t corruption = 0;
			if (PlotCorruptedEntries) {
				TH1D *CorruptionInTime = new TH1D("CorruptionInTime","corrupted entries during run (entry method)",(Int_t)duration/5,0,(Int_t)duration);
				CorruptionInTime->GetXaxis()->SetTitle("time (5 sec / bin)");
			}
			
			Int_t lednumPanelsHit = 0;
			Int_t muonnumPanelsHit = 0;
			runnum[filesScanned] = run;
			
			Float_t MuonQDCtot = 0;
			Float_t AvgMuonQDCvalue = 0;
			
			sprintf(hname,"ledtimestamp_run%d",run);
			ledtimestamp[filesScanned] = new TGraph(nentries);
			ledtimestamp[filesScanned]->SetName(hname);
			
			//check if runmin and runmax are correct
			if (filesScanned == 0 && runmin != run){
				cout << "Runmin is not set correctly (should be first run number in debug list" << endl;
				break;
			}	
			
			if (filesScanned == numFiles -1 && runmax != run){
				cout << "Runmax is not set correctly (should be last run number in debug list" << endl;
				break;
			}
			
			if (PlotMultiplicity) {
				TH1D *OneRunMultiplicity = new TH1D("multiplicity","multiplicity of veto entries",32,0,32);
  				OneRunMultiplicity->GetXaxis()->SetTitle("number of panels hit");
  			}
			
			TGraph *AvgFileLEDQDCgraph = new TGraph(numFiles);
			AvgFileLEDQDCgraph->SetTitle("Average LED QDC value per file");
			AvgFileLEDQDCgraph->GetXaxis()->SetTitle("File Number");
			AvgFileLEDQDCgraph->GetYaxis()->SetTitle("Avg QDC value");

			TGraph *ledFiletime = new TGraph(numFiles);
			ledFiletime->SetTitle("# of LEDs vs run number");
			ledFiletime->GetXaxis()->SetTitle("Run number");
			ledFiletime->GetYaxis()->SetTitle("Total # of LEDs");
			
			TGraph *ledfitslope = new TGraph(numFiles);
			ledfitslope->SetTitle("(# of led vs run duration) linear fit slope vs run number");
			ledfitslope->GetXaxis()->SetTitle("Run number");
			ledfitslope->GetYaxis()->SetTitle("slope (corresponds to frequency)");

		//=== End ===

		// Loop over VetoTree entries
		printf("Now scanning run %i: %lli entries, %.2f sec.  \n",run,nentries,duration);
		if (nentries == 0) IsEmpty = true;
		for (int z = 0; z < nentries; z++) {
				VetoTree->GetEntry(z);
				
				//Access run duration
				duration = VetoRun->GetStopTime() - VetoRun->GetStartTime();
				
				// Access the MJTVetoData objects "vd"
				MJTVetoData *vd[numPanels];
				for (int i=0; i<numPanels; i++) { vd[i] = dynamic_cast<MJTVetoData*>(vetoEvent->GetDetectorData()->At(i)); }
				
				if (vd[0]->GetEventCount() != vd[0]->GetScalerCount()) 
				printf("Warning!  EventCount and ScalerCount don't match!\n");
		
				Bool_t isBadTS = vd[0]->IsBadTS();
				double TimeStamp = vd[0]->GetTimeStamp()/1E8; //scaler timestamp in seconds
					
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
				
		

				//=====================BEGIN ACTUAL GODDAMMED ANALYSIS=================
				//change Bool_t isLED to isIdentified? (0=unidentified, 1 = LED, 2 = muon)?


				Bool_t isLED = false; 		//if true, event is LED (first assume event is not an LED)
		
				if (isBadTS) { 
					BadTSInFile++;
					TotalCorruptionInTime->Fill(TimeStamp);
					if (PlotCorruptedEntries) CorruptionInTime->Fill(TimeStamp);
				}

				// multiplicity of panels above threshold
		       		for (int k=0; k<numPanels; k++) { 
				//	hRawQDC[filesScanned][k]->Fill(QDC[k]);
				
				

				// count lednumPanelsHit and muonnumPanelsHit
				if (useThresh) { 
					if (QDC[k]>ledthresh[k]) lednumPanelsHit++; 
					if (QDC[k]>muonthresh[k]) muonnumPanelsHit++;
				}
				else { if (!IsUnderThreshold[k]) lednumPanelsHit++; }
				
				}
		      		TotalMultiplicity->Fill(lednumPanelsHit);			
		       		if (PlotMultiplicity) OneRunMultiplicity->Fill(lednumPanelsHit);

				if (lednumPanelsHit >= 2){
					hLEDCutMulttest->Fill(lednumPanelsHit);
				}

				//include LED cut
				if (lednumPanelsHit >= ledcut){
					isLED = true; // if true, marks the signal as an LED
					hLEDCutMult->Fill(lednumPanelsHit);
					ledcount[filesScanned] +=1;
/*					if(filesScanned == 0) {
						cout << "led count for file 0 is: " << ledcount[filesScanned] << endl;
					}
*/
					//fill ledtimestamp
					ledtimestamp[filesScanned]->SetPoint(ledcount[filesScanned], TimeStamp, ledcount[filesScanned]);
	
					hLEDpertime->Fill(TimeStamp,ledcount[filesScanned]);
					ledTime[filesScanned]->Fill(TimeStamp,ledcount[filesScanned]);
					
					if (ledcountmax[filesScanned] < ledcount[filesScanned]) ledcountmax[filesScanned] = ledcount[filesScanned];

					if (ledtimemax[filesScanned] < TimeStamp) ledtimemax[filesScanned] = TimeStamp;
					
					for (int k=0; k<numPanels; ++k) {
						if (useThresh) { 
							if (QDC[k]>ledthresh[k]){
							//	hLEDCutQDC[filesScanned][k]->Fill(QDC[k]);
								ledQDCcount[filesScanned][k] += 1;
								totledQDC[filesScanned][k] += QDC[k];
								quadtotledQDC[filesScanned][k] += QDC[k]*QDC[k];
								
							}
						}
						else {
						//	if (!IsUnderThreshold[k]) hLEDCutQDC[filesScanned][k]->Fill(QDC[k]); 
						}	
						
					}
					//find deltat
					Tnew = TimeStamp;
					//see if 0 in deltat is from the first event
					if (ledcount[filesScanned] == 1 | ledcount[filesScanned] == 2){
//						cout << "led count = " << ledcount[filesScanned] << " | Timestamp = " << Tnew << endl;
					}					
					deltaT = Tnew - Told;
					if (deltaT < 0.01 || deltaT > 10000){
					lowdt << "delta T = " << deltaT << " | led #'s = " << ledcount[filesScanned]-1 << " and " << ledcount[filesScanned] << "run # = " << run << endl;

					}
					Told = Tnew;
					hLEDCutDT->Fill(deltaT);
					hLEDCutDTFocus->Fill(deltaT);
					if (TimeStamp > 10000){
						badtimecount++;
					}
	
				}
			
				//count multiplicity for muon cut
				if (!isLED && muonnumPanelsHit > 2 && muonnumPanelsHit < ledcut){
					muoncount[filesScanned] += 1;
										
					for (Int_t k=0; k<numPanels; k++){
						if (muonnumPanelsHit > k) hMuonMult[k]->Fill(muonnumPanelsHit);
						if(QDC[k]>muonthresh[k]){
							//hMuonCutQDC[filesScanned][k]->Fill(QDC[k]);
							hMuonManyQDC[k]->Fill(QDC[k]);
							MuonQDCtot += QDC[k];
						}
					}			
					AvgMuonQDCvalue = MuonQDCtot/((double) muonnumPanelsHit);
					hAvgMuonQDC->Fill(AvgMuonQDCvalue);
					MuonQDCtot = 0;
					
				}
				
				//=====================END ACTUAL GODDAMMED ANALYSIS===================
			
			lednumPanelsHit=0;
			muonnumPanelsHit=0;

		}	// End loop over VetoTree entries.

		// === END OF FILE Output & Plotting ===
		if (filesScanned == 0){
			TDirectory *runmultiplicity = RootFile->mkdir("RunMultiplicity");	
			TDirectory *corruptionintime = RootFile->mkdir("CorruptionInTime");
		}
		
		corruption = ((Float_t)BadTSInFile/nentries)*100;
		printf(" Corrupted scaler entries: %i of %lli, %.3f %%.\n",BadTSInFile,nentries,corruption);
		if(run>45000000) SCorruption->SetPoint(filesScanned,run-45000000,corruption);
		else SCorruption->SetPoint(filesScanned,run,corruption);

		if (PlotCorruptedEntries) {
			RootFile->cd("CorruptionInTime");
			char outfile1[200];	
			sprintf(outfile1,"CorruptionInTime_Run%i",run);
			CorruptionInTime->Write(outfile1,TObject::kOverwrite);
			RootFile->cd();
		}
		if (PlotMultiplicity) {
			RootFile->cd("RunMultiplicity");
			char outfile2[200];
			sprintf(outfile2,"Multiplicity_Run%i",run);
			OneRunMultiplicity->Write(outfile2,TObject::kOverwrite);
			RootFile->cd();
		}

		if (!IsEmpty && ledcount[filesScanned] > 2){
			ledTime[filesScanned]->Fit("pol1");
			slope[filesScanned] = ledTime[filesScanned]->GetFunction("pol1")->GetParameter(1);
		}
		
		for (int j=0; j<numFiles; ++j){
			ledFiletime->SetPoint(j, runnum[j], ledcount[j]);
		}

		// ==========================

		delete VetoTree;
		cout << "files Scanned: " << filesScanned << endl;
		if (duration >= 0){
			totdur += duration;
		}
		if (duration < 0){
			omitteddur[baddurcount]=run;
			baddurcount +=1;
		}
		cout << "totdur = " << totdur << " | duration = " << duration << endl;
		filesScanned++;
	} // End loop over files.

	//calculate/fill qdc mean and rms
	for (int k=0; k<numPanels; ++k){
		for (int j=0; j<numFiles; ++j){
			if(k == 15){
				AvgledQDC[j][k] = 0;
				rmsledQDC[j][k] = 0;
			}
			else{
				AvgledQDC[j][k] = totledQDC[j][k]/ledQDCcount[j][k];
				rmsledQDC[j][k] = sqrt(quadtotledQDC[j][k]/ledQDCcount[j][k]);
			}
			hLEDAvgQDCPanel[k]->Fill(runnum[j],AvgledQDC[j][k]);
			hLEDrmsQDCPanel[k]->Fill(runnum[j],rmsledQDC[j][k]);
//		cout << "panel number: " << k << " | mean value " << AvgQDC[k] << " | rms value " << rmsQDC[k] << endl;
		}
	} 

	//calculate qdc mean for each file
	for (int j=0; j<numFiles; ++j){		
		for (int k=0; k<numPanels; ++k){
			totFileLEDQDC[j] += totledQDC[j][k];
			totFileledQDCcount[j] += ledQDCcount[j][k];
		}
		AvgFileLEDQDC[j] = totFileLEDQDC[j]/totFileledQDCcount[j];
		AvgFileLEDQDCgraph->SetPoint(j, runnum[j], AvgFileLEDQDC[j]);
		if (!IsEmpty){
			ledfitslope->SetPoint(j,runnum[j],slope[j]);
		}
	}
	
	for (int i=0; i<numFiles; ++i){
		printf("led count for file %i: %i. \n", i,ledcount[i]);	
		totledcount += ledcount[i];

		printf("muon count for file %i: %i. \n", i,muoncount[i]);	
		totmuoncount += muoncount[i];
	}

	// === END OF SCAN Output & Plotting ===
	printf("Finished loop over files.\n");
	printf("Total # of Files: %i. \n",numFiles);
	printf("Total Duration: %f seconds. \n",totdur);
	printf("Total LED Count: %f. \n", totledcount);
	printf("Total Muon Count: %f. \n", totmuoncount);	
	printf("Number of Bad Durations: %f. \n", baddurcount);
	

//	TCanvas *c1 = new TCanvas("c1", "Bob Ross's Canvas",600,600);
//	c1->SetGrid();
	SCorruption->SetMarkerColor(4);
	SCorruption->SetMarkerStyle(21);
	SCorruption->SetMarkerSize(0.5);
	SCorruption->SetTitle("Corruption in scaler card");
	SCorruption->GetXaxis()->SetTitle("Run");
	SCorruption->GetYaxis()->SetTitle("% corrupted events");
//	SCorruption->Draw("ALP");	
	
	SCorruption->Write("ScalerCorruption",TObject::kOverwrite);

	TotalCorruptionInTime->Write("TotalCorruptionInTime",TObject::kOverwrite);

	TotalMultiplicity->Write("TotalMultiplicity",TObject::kOverwrite);


//	TCanvas *c2 = new TCanvas("c2", "LEDCutMult",600,600);
//	c2->SetGrid();
//	c2->SetLogy();
//	hLEDCutMult->Draw();
	hLEDCutMult->Write();

//	TCanvas *c3 = new TCanvas("c3", "LEDCutMulttest",600,600);
//	c3->SetGrid();
//	c3->SetLogy();
//	hLEDCutMulttest->Draw();
	hLEDCutMulttest->Write();

//	TCanvas *c4 = new TCanvas("c4", "LEDCutDT",600,600);
//	c4->SetGrid();
//	hLEDCutDT->Draw();
	hLEDCutDT->Write();

//	TCanvas *c5 = new TCanvas("c5", "AvgFileLEDQDCgraph",600,600);
//	c5->SetGrid();
	AvgFileLEDQDCgraph->SetMarkerColor(4);
	AvgFileLEDQDCgraph->SetMarkerStyle(21);
	AvgFileLEDQDCgraph->SetMarkerSize(0.5);
//	AvgFileLEDQDCgraph->Draw("AP");	
	AvgFileLEDQDCgraph->Write("AvgFileLEDQDCgraph",TObject::kOverwrite);


//	TCanvas *c6 = new TCanvas("c6", "ledFiletime",600,600);
//	c6->SetGrid();
	ledFiletime->SetMarkerColor(4);
	ledFiletime->SetMarkerStyle(21);
	ledFiletime->SetMarkerSize(0.5);
//	ledFiletime->Draw("ALP");	
	ledFiletime->Write("LEDFileTime",TObject::kOverwrite);


//	TCanvas *c7 = new TCanvas("c7", "LEDpertime",600,600);
//	c7->SetGrid();
//	hLEDpertime->Draw();
//	hLEDpertime->Write();

//	TCanvas *c9 = new TCanvas("c9", "slope vs file number", 600,600);
//	c9->SetGrid();
	ledfitslope->SetMarkerColor(4);
	ledfitslope->SetMarkerStyle(21);
	ledfitslope->SetMarkerSize(0.5);
//	ledfitslope->Draw("ALP");	
	ledfitslope->Write("LEDFitSlope",TObject::kOverwrite);
	
//	TCanvas *c10 = new TCanvas("c10", "avg muon qdc value", 600,600);
//	c10->SetGrid();
//	hAvgMuonQDC->Draw();
	hAvgMuonQDC->Write();
	
//	TCanvas *c11 = new TCanvas("c11", "LEDCutDTFocus",600,600);
//	c11->SetGrid();
//	hLEDCutDTFocus->Draw();
	hLEDCutDTFocus->Write();
	
//	TCanvas *vcan0 = new TCanvas("vcan0","raw veto QDC, panels 1-32",0,0,1600,900);
//	vcan0->Divide(8,4,0,0);
	
//	TCanvas *vcan1 = new TCanvas("vcan1","LED Cut veto QDC (ledcut = 20), panels 1-32",0,0,1600,900);
//	vcan1->Divide(8,4,0,0);

//	TCanvas *vcan2 = new TCanvas("vcan2","Muon Cut veto QDC (all qdc over muon thresh), panels 1-32",0,0,1600,900);
//	vcan2->Divide(8,4,0,0);

//	TCanvas *vcan9 = new TCanvas("vcan9","Muon Cut Multiplicity (muon cut = 0-31)", 0,0,1600,900);
//	vcan9->Divide(8,4,0,0);	
//	TCanvas *vcan10 = new TCanvas("vcan10","LED event vs time (ledcut = 20), files 1-2",0,0,800,600);
//	vcan10->Divide(2);
	
//	TCanvas *vcan11 = new TCanvas("vcan11","Muon QDC numPanelsHit > 2 && < 20", 0,0,1600,900);
//	vcan11->Divide(8,4,0,0);

//	TCanvas *vcan12 = new TCanvas("vcan12","Avg LED QDC", 0,0, 1600,900);
//	vcan12->Divide(8,4,0,0);

//	TCanvas *vcan13 = new TCanvas("vcan13","rms LED QDC", 0,0,1600,900);
//	vcan13->Divide(8,4,0,0);

//	TCanvas *vcan14 = new TCanvas("vcan14","ledtimestamps",0,0,1600,900);
	
	
	Char_t buffer[2000];
	printf("\n Calibration Table:\n  Panel / Mean,error / Sigma,error / Chi-square/NDF (~1?) / LED Peak Pos.\n");

	
	RootFile->Write();

	TDirectory *ledtime = RootFile->mkdir("LEDTime");
	for (Int_t i=0; i<filesToScan; i++){

		RootFile->cd("LEDTime");
//		vcan10->cd(i+1);
//		TVirtualPad *vpad10 = vcan10->cd(i+1);
//		ledTime[i]->Draw();
		ledTime[i]->Write();

	}
	
	TDirectory *rawqdc = RootFile->mkdir("RawQDC");
	TDirectory *ledcutqdc = RootFile->mkdir("LEDCutQDC");
	TDirectory *muoncutqdc = RootFile->mkdir("MuonCutQDC");
	TDirectory *muonmult = RootFile->mkdir("MuonMult");
	TDirectory *muonmanyqdc = RootFile->mkdir("MuonManyQDC");
	TDirectory *avgledqdc = RootFile->mkdir("AvgLEDQDC");
	TDirectory *rmsledqdc = RootFile->mkdir("rmsLEDQDC");
	TDirectory *ledtimestamps = RootFile->mkdir("LEDTimestamps");

	
	for (Int_t j=0; j<numFiles; j++){


		for (Int_t i=0; i<numPanels; i++){
		
			RootFile->cd("RawQDC");
//			vcan0->cd(i+1);
//			TVirtualPad *vpad0 = vcan0->cd(i+1); vpad0->SetLogy();
//			hRawQDC[j][i]->Draw();
//			hRawQDC[j][i]->Write();		// write the raw QDC without fitting

			
			RootFile->cd("LEDCutQDC");
//			vcan1->cd(i+1);
//			TVirtualPad *vpad1 = vcan1->cd(i+1); vpad1->SetLogy();
//			hLEDCutQDC[j][i]->Draw();
//			hLEDCutQDC[j][i]->Write();		
		
			RootFile->cd("MuonCutQDC");
//			vcan2->cd(i+1);
//			TVirtualPad *vpad2 = vcan2->cd(i+1); vpad2->SetLogy();
//			hMuonCutQDC[j][i]->Draw();
//			hMuonCutQDC[j][i]->Write();	

		}
		RootFile->cd("LEDTimestamps");
//		vcan14->cd(1);
//		vcan14->SetLogy();
		ledtimestamp[j]->SetMarkerColor(4);
		ledtimestamp[j]->SetMarkerStyle(21);
		ledtimestamp[j]->SetMarkerSize(0.5);
		ledtimestamp[j]->SetTitle("ledcount vs timestamp");
		ledtimestamp[j]->GetXaxis()->SetTitle("timestamp");
		ledtimestamp[j]->GetYaxis()->SetTitle("ledcount");
//		ledtimestamp[j]->Draw("ALP");	
		ledtimestamp[j]->Write();
		
	}
	
	RootFile->cd("MuonManyQDC");
	for (Int_t i=0; i<numPanels; i++){
//		vcan11->cd(i+1);
//		TVirtualPad *vpad11 = vcan11->cd(i+1); vpad11->SetLogy();
		hMuonManyQDC[i]->GetYaxis()->SetTitle("Multiplicity");
		hMuonManyQDC[i]->GetXaxis()->SetTitle("QDC Values");
//		hMuonManyQDC[i]->Draw();
		hMuonManyQDC[i]->Write();
	
	}
	RootFile->cd("MuonMult");
	for (Int_t i=0; i<numPanels; i++){
//		vcan9->cd(i+1);
//		TVirtualPad *vpad9 = vcan9->cd(i+1); vpad9->SetLogy();
		hMuonMult[i]->GetYaxis()->SetTitle("Multiplicity");
		hMuonMult[i]->GetXaxis()->SetTitle("# of panels hit");
//		hMuonMult[i]->Draw();
		hMuonMult[i]->Write();
	}

	RootFile->cd("AvgLEDQDC");
	for (Int_t i=0; i<numPanels; i++){
//		vcan12->cd(i+1);
//		TVirtualPad *vpad12 = vcan12->cd(i+1); vpad12->SetLogy();
		hLEDAvgQDCPanel[i]->GetYaxis()->SetTitle("Avg QDC value");
		hLEDAvgQDCPanel[i]->GetXaxis()->SetTitle("Run Number");
//		hLEDAvgQDCPanel[i]->Draw();
		hLEDAvgQDCPanel[i]->Write();
	}

	RootFile->cd("rmsLEDQDC");
	for (Int_t i=0; i<numPanels; i++){
//		vcan13->cd(i+1);
//		TVirtualPad *vpad13 = vcan13->cd(i+1); vpad13->SetLogy();
		hLEDAvgQDCPanel[i]->GetYaxis()->SetTitle("rms QDC value");
		hLEDAvgQDCPanel[i]->GetXaxis()->SetTitle("Run Number");
//		hLEDrmsQDCPanel[i]->Draw();
		hLEDrmsQDCPanel[i]->Write();
	}

	RootFile->cd();

	// ==========================

	RootFile->Close();

	cout << "badtimecount = " << badtimecount << endl;
	cout << "Wrote ROOT file." << endl;
	
	lowdt.close();

}


