#include <iostream>
#include "TFile.h"
#include "TH1.h"
#include "TTree.h"

void dataReader(char * inFile = "", 
		bool bCol = true, 
		bool bRow = true, 
		bool bToA = true, 
		bool bToT = true, 
		bool bGTF = true, 
		int nHitsCut = 0, 
		int windowCut = 40, 
		int linesPerFile = 50000) {
	if (inFile == "") {
		cout << "Please provide fule to reduce : root -l 'dataReader(\"<filename.root>\")'" << endl;
		return;
	}
	if (windowCut > 500) {
		cout << " ========================================== " << endl;
		cout << "=== " << windowCut << " ms is very large winwdow!!!" << endl;
		cout << " ========================================== " << endl;
		return;
	}
	cout << "Working with : " << inFile << " file" << endl;
	char outFile[200];
	sprintf(outFile, "%s.csv", inFile);

	FILE * myFile;
	myFile = fopen(outFile, "w");
	if (bCol) fprintf(myFile, "Col");
	if (bRow) fprintf(myFile, ",Row");
	if (bToA) fprintf(myFile, ",ToA");
	if (bToT) fprintf(myFile, ",ToT");
	if (bGTF) fprintf(myFile, ",GlobalTimeFine");
	fprintf(myFile, "\n");

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
			if (GlobalTimesFine[0] < TrigTime || nhits < nHitsCut) {
				currChunk = j;
				continue;
			}
			if (GlobalTimesFine[0] > (TrigTime + windowCut/6.1*1000000)) {
				currChunk = j;
				break;
			}
			else {
				for (int k = 0; k < nhits; k++) {
					fprintf(myFile, "%d", i);
					if (bCol) fprintf(myFile, ",%d", Cols[k]);
					if (bRow) fprintf(myFile, ",%d", Rows[k]);
					if (bToA) fprintf(myFile, ",%d", ToAs[k]);
					if (bToT) fprintf(myFile, ",%d", ToTs[k]);
					if (bGTF) fprintf(myFile, ",%llu", GlobalTimesFine[k]);
					fprintf(myFile, "\n");
				}
				myCounter++;
			}
		}
	}
	cout << myCounter << " events from total of " << Nraw << " selected!"  << endl;
	fclose(myFile);
}
