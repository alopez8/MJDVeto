/*
	panelmaptest.C
	Andrew Lopez, UTK/Majorana
	January 2016.

	this program is designed to test whether the function PanelMap, which will determine the directionality of an event flagged as a muon, is working properly.
	
	Usage:
	CINT: root[0] .X builtVeto.C ("Filename_list_of_run_numbers")  <--- NO .TXT extension.
	bash: root -b -q -l builtVeto.C ("The_filename_without_extension")
*/

#ifndef __CINT__
//#include <vector>
#include <iostream>
//#include <fstream>

using namespace std;
#include <new>
//using namespace CLHEP;
#endif

//Panel Map function
int PanelMap(int i){

	int y =0;
	// 0 = lower-bottom, 1 = upper-bottom, 2 = Top, 3 = inner north, 4 = outer north, 5 = inner east, 6 = outer east, 7 = inner south, 8 = outer south, 9 = inner west, 10 = outer west

	if 	(i+1 == 1)  y=0;	//lower-bottom
	else if (i+1 == 2)  y=0;	//lower-bottom
	else if (i+1 == 3)  y=0;	//lower-bottom
	else if (i+1 == 4)  y=0;	//lower-bottom
	else if (i+1 == 5)  y=0;	//lower-bottom
	else if (i+1 == 6)  y=0;	//lower-bottom

	else if (i+1 == 7)  y=1;	//upper-bottom
	else if (i+1 == 8)  y=1;	//upper-bottom
	else if (i+1 == 9)  y=1;	//upper-bottom
	else if (i+1 == 10)  y=1;	//upper-bottom
	else if (i+1 == 11) y=1;	//upper-bottom
	else if (i+1 == 12) y=1;	//upper-bottom


	else if (i+1 == 18) y=2;	//top
	else if (i+1 == 19) y=2;	//top
	else if (i+1 == 21) y=2;	//top
	else if (i+1 == 22) y=2;	//top


	else if (i+1 == 20) y=3;	//inner north
	else if (i+1 == 24) y=3;	//inner north

	else if (i+1 == 16) y=4;	//outer north
	else if (i+1 == 17) y=4;	//outer north


	else if (i+1 == 29) y=5;	//inner east
	else if (i+1 == 31) y=5;	//inner east

	else if (i+1 == 30) y=6;	//outer east
	else if (i+1 == 32) y=6;	//outer east


	else if (i+1 == 25) y=7;	//inner south
	else if (i+1 == 27) y=7;	//inner south

	else if (i+1 == 26) y=8;	//outer south
	else if (i+1 == 28) y=8;	//outer south


	else if (i+1 == 13) y=9;	//inner west
	else if (i+1 == 14) y=9;	//inner west

	else if (i+1 == 15) y=10;	//outer west
	else if (i+1 == 23) y=10;	//outer west


	else y=-1;
	
	return y;
}

const int numPanels = 32;

void panelmaptest(string Input = ""){

		Int_t numpanelshit = 0;
		Int_t a = 0;
		
		//muon (high) qdc threshold values
		Int_t muonthresh[numPanels] = {0};
		
		//set Test QDC values
		Int_t QDC[numPanels] = {0};
		
		cout << "Enter how many panels will trigger." << endl;
		cin >> numpanelshit;
		
		for (Int_t i=0; i<numpanelshit; i++){
			READ:cout << "Enter panel number, 0-31, which will be hit." << endl;
			cin >> a;
			if (a < 0 || a > 31) goto READ;
			QDC[a] = 700;
		}	

		for (Int_t k=0; k<numPanels; k++){
			muonthresh[k] = 500;
		}

	
	//=== Global counters / variables / plots ===
	
		Int_t tbonly = 0;
		Int_t tcount = 0;
		Int_t bcount = 0;
		Int_t ncount = 0;
		Int_t wcount = 0;
		Int_t ecount = 0;
		Int_t scount = 0;


			

				Bool_t isTopMuon = false;	//if true, Muon event has fire on AT LEAST 1 top panel
				Bool_t isBotMuon = false;
				Bool_t isNorthMuon = false;
				Bool_t isSouthMuon = false;
				Bool_t isWestMuon = false;
				Bool_t isEastMuon = false;

				// multiplicity of panels above threshold
		       	for (int k=0; k<numPanels; k++) { 
						if (QDC[k]>muonthresh[k]){
							cout << "isBotMuon " << isBotMuon << endl;
							cout << "isTopMuon " << isTopMuon << endl;
							cout << "PanelMap of " << k << " = " << PanelMap(k) << endl;
							//determine which panels where hit (top,bottom,west,east,north,south) | must be done before writing muonn qdcs to top/bot only graphs
							if (PanelMap(k)== 0 || PanelMap(k) == 1) {
								isBotMuon = true;
								cout << "Bot:PanelMap = " << PanelMap(k) << endl;
							}	
							else if (PanelMap(k)== 2){
								isTopMuon = true;
								cout << "Top:PanelMap = " << PanelMap(k) << endl;
							}
							else if (PanelMap(k)== 3 || PanelMap(k) == 4) {
								isNorthMuon = true;
								cout << "North:PanelMap = " << PanelMap(k) << endl;
							}
							else if (PanelMap(k)== 5 || PanelMap(k) == 6){
								isEastMuon = true;
								cout << "East:PanelMap = " << PanelMap(k) << endl;
							}
							else if (PanelMap(k)== 7 || PanelMap(k) == 8){
								isSouthMuon = true;
								cout << "South:PanelMap = " << PanelMap(k) << endl;
							}
							else if (PanelMap(k)== 9 || PanelMap(k) == 10){
								isWestMuon = true;
								cout << "West:PanelMap = " << PanelMap(k) << endl;
							}
						}	
			
				}
				cout << "isBotMuon " << isBotMuon << endl;
				cout << "isTopMuon " << isTopMuon << endl;
					if (isTopMuon) tcount++;
					if (isBotMuon) bcount++;
					if (isNorthMuon) ncount++;
					if (isSouthMuon) scount++;
					if (isEastMuon) ecount++;
					if (isWestMuon) wcount++;
					if (isTopMuon && isBotMuon && !isEastMuon && !isWestMuon && !isNorthMuon && !isSouthMuon) tbonly++;
					
					
	cout << "number of TB only muons: " << tbonly << endl;
	cout << "numer of top muons: " << tcount << endl;
	cout << "numer of bottom muons: " << bcount << endl;
	cout << "numer of west muons: " << wcount << endl;
	cout << "numer of north muons: " << ncount << endl;
	cout << "number of east muons: " << ecount << endl;
	cout << "number of south muons: " << scount << endl;

}


