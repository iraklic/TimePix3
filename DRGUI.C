//==================================
// Author: IRAKLI CHAKABERIA   =====
// Date: 2017-02-14            =====
//==================================

#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TG3DLine.h>
#include <TApplication.h>
#include "TGTextEntry.h"

const char * fileTypes[] = {
	"ROOT files", "*.root",
	0, 0
};

class TextMargin : public TGHorizontalFrame {
	protected:
	TGNumberEntry *fEntry;

	public:
	TextMargin(const TGWindow *p, const char *name) : TGHorizontalFrame(p) {
		fEntry = new TGNumberEntry(this, 0, 6, -1, TGNumberFormat::kNESInteger);
		AddFrame(fEntry, new TGLayoutHints(kLHintsLeft));
		TGLabel *label = new TGLabel(this, name);
		AddFrame(label, new TGLayoutHints(kLHintsLeft, 10));
	}
	TGTextEntry * GetEntry() const { return fEntry->GetNumberEntry(); }
	void SetEntry(int entry) { fEntry->SetNumber(entry); }

	ClassDef(TextMargin, 0)
};

class DRGui : public TGMainFrame {
	protected:
	TGTextButton * fButton;
	TGTextButton * bButton;
	TGTextEntry * rootFile;

	public:
	bool bnHits;
	bool bCol;
	bool bRow;
	bool bToA;
	bool bToT;
	bool bGTF;

	int nHitsCut;
	int timeWindow;
	int linesPerFile;

	DRGui();
	void RunReducer();
	void ApplyCutsHits(char *);
	void ApplyCutsTime(char *);
	void ApplyCutsLines(char *);

	void SelectCol(Bool_t);
	void SelectRow(Bool_t);
	void SelectToA(Bool_t);
	void SelectToT(Bool_t);
	void SelectGTF(Bool_t);

	void Browse();

	ClassDef(DRGui, 0)
};

DRGui::DRGui() : TGMainFrame(gClient->GetRoot(), 10, 10, kHorizontalFrame) {
	SetCleanup(kDeepCleanup);

//	All variables are selected by default
	bCol = true;
	bRow = true;
	bToA = true;
	bToT = true;
	bGTF = true;

	linesPerFile = 50000;
	nHitsCut = 1;
	timeWindow = 40;

//	Controls on right
	TGVerticalFrame * controls = new TGVerticalFrame(this);
	AddFrame(controls, new TGLayoutHints(kLHintsRight | kLHintsExpandY, 5, 5, 5, 5));

//	VARIABLE ON LEFT
	TGVerticalFrame * variables = new TGVerticalFrame(this);
	AddFrame(variables, new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 5, 5, 5, 5));

//	Separator
	TGVertical3DLine *separator = new TGVertical3DLine(this);
	AddFrame(separator, new TGLayoutHints(kLHintsRight | kLHintsExpandY));

	TGVertical3DLine *separator1 = new TGVertical3DLine(this);
	AddFrame(separator1, new TGLayoutHints(kLHintsExpandY));

//	Contents 
	TGVerticalFrame * contents = new TGVerticalFrame(this);
	AddFrame(contents, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));

//	The button for test
	fButton = new TGTextButton(contents, "&RUN THE DATA REDUCER");
//	fButton->Resize(200, 200);
//	fButton->ChangeOptions(fButton->GetOptions() | kFixedSize);
	fButton->SetToolTipText("Run the reducer macro", 200);

	fButton->Connect("Pressed()", "DRGui", this, "RunReducer()");

//	BROWS FILE
	rootFile = new TGTextEntry(contents, new TGTextBuffer(50));
	rootFile->SetText("Select File");
	bButton = new TGTextButton(contents, "Browse Files");
	contents->AddFrame(rootFile, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 5, 5));
	contents->AddFrame(bButton, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 5, 5));
	contents->AddFrame(fButton, new TGLayoutHints(kLHintsBottom | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));

	bButton->Connect("Clicked()", "DRGui", this, "Browse()");

//	VARIABLES
	TGGroupFrame * varsGroup = new TGGroupFrame(variables, "Variables");
	varsGroup->SetTitlePos(TGGroupFrame::kCenter);
	TGCheckButton * col = new TGCheckButton(varsGroup, "Col");
	TGCheckButton * row = new TGCheckButton(varsGroup, "Row");
	TGCheckButton * toa = new TGCheckButton(varsGroup, "ToA");
	TGCheckButton * tot = new TGCheckButton(varsGroup, "ToT");
	TGCheckButton * gtf = new TGCheckButton(varsGroup, "GlobalTimeFine");

	col->SetOn();
	row->SetOn();
	toa->SetOn();
	tot->SetOn();
	gtf->SetOn();

	varsGroup->AddFrame(col, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
	varsGroup->AddFrame(row, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
	varsGroup->AddFrame(toa, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
	varsGroup->AddFrame(tot, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
	varsGroup->AddFrame(gtf, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

	col->Connect("Toggled(Bool_t)", "DRGui", this, "SelectCol(Bool_t)");
	row->Connect("Toggled(Bool_t)", "DRGui", this, "SelectRow(Bool_t)");
	toa->Connect("Toggled(Bool_t)", "DRGui", this, "SelectToA(Bool_t)");
	tot->Connect("Toggled(Bool_t)", "DRGui", this, "SelectToT(Bool_t)");
	gtf->Connect("Toggled(Bool_t)", "DRGui", this, "SelectGTF(Bool_t)");

	variables->AddFrame(varsGroup, new TGLayoutHints(kLHintsExpandX));

//	CUTS
	TGGroupFrame * cuts = new TGGroupFrame(controls, "Cuts");
	cuts->SetTitlePos(TGGroupFrame::kCenter);

	TextMargin * nhits = new TextMargin(cuts, "nhits");
	TextMargin * timeW = new TextMargin(cuts, "Time Window (micro s)");
	timeW->SetEntry(40);

	cuts->AddFrame(nhits, new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));
	cuts->AddFrame(timeW, new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));

	nhits->GetEntry()->Connect("TextChanged(char*)", "DRGui", this, "ApplyCutsHits(char*)");
	timeW->GetEntry()->Connect("TextChanged(char*)", "DRGui", this, "ApplyCutsTime(char*)");

	controls->AddFrame(cuts, new TGLayoutHints(kLHintsExpandX));

//	OUTPUT CONTROL
	TGGroupFrame * outControl = new TGGroupFrame(controls, "Output File");
	outControl->SetTitlePos(TGGroupFrame::kCenter);

	TextMargin * nLines = new TextMargin(outControl, "Lines per File");
	outControl->AddFrame(nLines, new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));
	nLines->SetEntry(100000);
	nLines->GetEntry()->Connect("TextChanged(char*)", "DRGui", this, "ApplyCutsLines(char*)");

	controls->AddFrame(outControl, new TGLayoutHints(kLHintsExpandX));

//	QUIT
	TGTextButton *quit = new TGTextButton(controls, "Quit");
	controls->AddFrame(quit, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 0, 5));
	quit->Connect("Pressed()", "TApplication", gApplication, "Terminate()");

	Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
	DontCallClose();

	MapSubwindows();
	Resize();

	SetWMSizeHints(GetDefaultWidth(), GetDefaultHeight(), 1000, 1000, 0 ,0);
	SetWindowName("RRGui");
	MapRaised();
}

void DRGui::RunReducer() {
	int err = 0;
	char cmd[500];
	sprintf(cmd,".x dataReader.C(\"%s\", %d, %d, %d, %d, %d, %d, %d, %d)",
		       	rootFile->GetText(),
			bCol,
			bRow,
			bToA,
			bToT,
			bGTF,
			nHitsCut,
			timeWindow,
			linesPerFile
			);

	cout << cmd << endl;
	gROOT->ProcessLine(cmd, &err);
}

void DRGui::ApplyCutsHits(char * val) { nHitsCut = atoi(val); }
void DRGui::ApplyCutsTime(char * val) { timeWindow = atoi(val); }
void DRGui::ApplyCutsLines(char * val) { linesPerFile = atoi(val); }

void DRGui::SelectCol(Bool_t check) { bCol = check; }
void DRGui::SelectRow(Bool_t check) { bRow = check; }
void DRGui::SelectToA(Bool_t check) { bToA = check; }
void DRGui::SelectToT(Bool_t check) { bToT = check; }
void DRGui::SelectGTF(Bool_t check) { bGTF = check; }

void DRGui::Browse() {
	static TString dir(".");
	TGFileInfo fileInfo;
	fileInfo.fFileTypes = fileTypes;
	fileInfo.fIniDir = StrDup(dir);
	new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fileInfo);
	if (fileInfo.fFilename) {
		rootFile->SetText(fileInfo.fFilename);
	}
	dir = fileInfo.fIniDir;
}

void DRGUI() {
	new DRGui();
}
