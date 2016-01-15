const int numPanels =32;

TH1F *hRawQDC[numPanels];
void findThresh(string Input = ""){

	TFile f("builtVeto_DebugList.root");

	stringstream ss;


	std::string histprefix = "hRawQDC";

	Int_t nbins[numPanels];
	Float_t binwidth[numPanels];
	Int_t pedestalbin[numPanels];
	Int_t threshbin[numPanels];
	Int_t thresh[numPanels];		
	Float_t pedestalval;
	Float_t nextbinval;
	
	Char_t hname[50];
	for (Int_t i=0; i<numPanels; i++){
			sprintf(hname,"hRawQDC%d",i);
			hRawQDC[i] = (TH1F*) f.Get(hname);
	}
	//find threshhold for each panel
	for (Int_t i=0; i<numPanels; i++){
		ss.str("");
		ss.clear();
		ss << i;
		string histname = histprefix + ss.str();
		
		pedestalbin[i] = 0;
		nbins[i]=hRawQDC[i]->GetNbinsX();
		binwidth[i] = hRawQDC[i]->GetBinWidth(1);	
		for (Int_t j=0; j<nbins[i]; j++){
			pedestalval = hRawQDC[i]->GetBinContent(pedestalbin[i]);
			nextbinval = hRawQDC[i]->GetBinContent(j);
			if (nextbinval > pedestalval) {
				pedestalbin[i] = j;	
			}
		
//		printf("\n pedestal bin = %i | pedestal value = %i | next bin # = %i | next bin value = %i. \n", pedestalbin[i], pedestalval, j, nextbinval);
		}
		
		thresh[i] = ceil((pedestalbin[i] + 3)*binwidth[i]);
		threshbin[i] = hRawQDC[i]->FindBin(thresh[i]); 
 
		printf("\n Panel Thresholds:\n  Panel # = %i  | threshold = %i | pedestal bin = %i | bin width = %i.\n", i, thresh[i], pedestalbin[i], binwidth[i] );
	}	

	cout << "found threshold." << endl;
}
