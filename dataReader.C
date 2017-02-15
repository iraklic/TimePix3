#include <iostream>
#include "TFile.h"
#include "TH1.h"
#include "TTree.h"

void dataReader(char * inFile = "") {
	if (inFile == "") {
		cout << "Please provide fule to reduce : root -l 'dataReader(\"<filename.root>\")'" << endl;
		return;
	}
	cout << inFile << endl;
	char outFile[100];
	sprintf(outFile, "%s.csv", inFile);

	FILE * myFile;
	myFile = fopen(outFile, "w");
	fprintf(myFile, "TrigId,Col,Row,ToA,ToT,GlobalTimeFine\n");

	TFile * f = new TFile(inFile, "READ");
	TTree * timetree = (TTree * ) f->Get("timetree");
	TTree * rawtree = (TTree * ) f->Get("rawtree");
	const int Ntime = timetree->GetEntries();
	const int Nraw = rawtree->GetEntries();

	ULong64_t TrigTime;
       	timetree->SetBranchAddress("TrigTimeGlobal", &TrigTime);

	Int_t nhits;
	Int_t Rows[65536];
	Int_t Cols[65536];
	Int_t ToTs[65536];
	Int_t ToAs[65536];
	Int_t FToAs[65536];
	ULong64_t GlobalTimesFine[65536];

	rawtree->SetBranchAddress("Nhits", &nhits);
	rawtree->SetBranchAddress("Col", Cols);
	rawtree->SetBranchAddress("Row", Rows);
	rawtree->SetBranchAddress("ToA", ToAs);
	rawtree->SetBranchAddress("FToA", FToAs);
	rawtree->SetBranchAddress("ToT", ToTs);
	rawtree->SetBranchAddress("GlobalTimeFine", GlobalTimesFine);

	int currChunk = 0;
	int myCounter = 0;
	ULong64_t lastTrig = 0;
	for (int i = 0; i < Ntime; i++) {
		if (i % 500 == 0) cout << i  << " out of " << Ntime << " done!" << endl;
		timetree->GetEntry(i);
		if (i) {
			ULong64_t diff = 6.1*(TrigTime - lastTrig);
			lastTrig = TrigTime;
		}
		for (int j = currChunk; j < Nraw; j++) {
			rawtree->GetEntry(j);
			if (GlobalTimesFine[0] < TrigTime || nhits < 2) {
				currChunk = j;
				continue;
			}
			if (GlobalTimesFine[0] > (TrigTime + 40/6.1*1000000)) {
				currChunk = j;
				break;
			}
			else {
				for (int k = 0; k < nhits; k++) {
					fprintf(myFile, "%d,%d,%d,%d,%d,%llu\n", i,  Cols[k], Rows[k], ToAs[k], ToTs[k], GlobalTimesFine[k]);
				}
				myCounter++;
			}
		}
	}
	cout << myCounter << " events from total of " << Nraw << " selected!"  << endl;
	fclose(myFile);
}
