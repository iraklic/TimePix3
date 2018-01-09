#ifndef DATAPROCESS_H
#define DATAPROCESS_H

#include <deque>

#include "TSysEvtHandler.h"

#include "TObjString.h"
#include "TString.h"
#include "TStopwatch.h"

#include "TTree.h"
#include "TH1.h"
#include "TH2.h"

#define MAXHITS 65536       // minimal value for sorting 65536

using namespace std;

enum ProcType
{
    procDat     = 0,
    procRoot    = 1,
    procAll     = 2
};

enum DataType
{
    dtDat  = 0,
    dtTpx  = 1,
    dtCent = 2,

    dtStandard = 3
};

class DataProcess
{
public:
    DataProcess();
    ~DataProcess();
    void setName(TString fileNameInput);
    void setName(TObjString *fileNameInput, Int_t size);
    void setProcess(ProcType process);
    void setNEntries(UInt_t nEntries);
    Int_t setOptions(Bool_t bCol = kTRUE,
                      Bool_t bRow = kTRUE,
                      Bool_t bToT = kTRUE,
                      Bool_t bToA = kTRUE,
                      Bool_t bTrig = kTRUE,
                      Bool_t bTrigTime = kTRUE,
                      Bool_t bTrigToA = kTRUE,
                      Bool_t bCentroid = kFALSE,
                      Int_t gapPixel = 0,
                      Float_t gapTime = 1.0,
                      Bool_t bNoTrigWindow = kFALSE,
                      Float_t timeWindow = 40,
                      Float_t timeStart = 0,
                      Bool_t singleFile = kFALSE,
                      Int_t  linesPerFile = 100000);
    Int_t process();
    void StopLoop();

private:
    Int_t processFileNames();
    void finishMsg(TString fileName, TString operation, UInt_t events, Int_t fileCounter = 1);
    ULong64_t roundToNs(ULong64_t number);

    Int_t openDat(Int_t fileCounter = 0);
    Int_t openCsv(DataType type = dtStandard, TString fileCounter = "");
    Int_t openRoot();

    void  findCluster(UInt_t index, UInt_t stop, deque<UInt_t >* cols, deque<UInt_t >* rows, deque<ULong64_t >* toas, deque<UInt_t> *tots, deque<Bool_t >* centered, deque<UInt_t >* indices);
    Int_t processDat();
    Int_t skipHeader();

    Int_t processRoot();
    void  plotStandardData();

    void closeDat();
    void closeCsv();
    void closeRoot();

private:
    //
    // global variables
    ProcType    m_process;
    UInt_t      m_maxEntries;
    TStopwatch  m_time;
    UInt_t      m_pixelCounter;
    TSignalHandler* m_sigHandler;
    Bool_t      m_bFirstTrig;
    Bool_t      m_bDevID;

    //
    // options of GUI
    Bool_t m_bCol;
    Bool_t m_bRow;
    Bool_t m_bToT;
    Bool_t m_bToA;
    Bool_t m_bTrig;
    Bool_t m_bTrigTime;
    Bool_t m_bTrigToA;
    Bool_t m_bCentroid;
    Bool_t m_bNoTrigWindow;
    Bool_t m_bSingleFile;

    Float_t  m_timeWindow;
    Float_t  m_timeStart;

    UInt_t  m_linesPerFile;

    //
    // centroiding parameters
    ULong64_t      m_gapTime;
    UInt_t         m_gapPix;

    //
    // file names
    DataType        m_type;
    Int_t           m_numInputs;
    deque<TString > m_fileNameInput;
    TString         m_fileNamePath;
    deque<TString > m_fileNameDat;
    TString         m_fileNameRoot;
    TString         m_fileNamePdf;
    TString         m_fileNameCsv;
    TString         m_fileNameCentCsv;

    //
    // files
    deque<FILE* >   m_filesDat;
    TFile*          m_fileRoot;
    deque<FILE* >   m_filesCsv;
    deque<FILE* >   m_filesCentCsv;

    //
    // ROOT trees
    TTree* m_rawTree;
    TTree* m_timeTree;
    TTree* m_procTree;

    UInt_t m_nRaw;
    UInt_t m_nCent;
    UInt_t m_nTime;

    //
    // ROOT histograms
    TH2I* m_pixelMap;
    TH2F* m_pixelMapToT;
    TH2F* m_pixelMapToA;

    TH1I* m_histToT;
    TH1I* m_histToA;

    TH2I* m_pixelCentMap;
    TH2F* m_pixelCentMapToT;
    TH2F* m_pixelCentMapToA;

    TH1I* m_histCentToT;
    TH1I* m_histCentToA;

    TH2F* m_histCentToTvsToA;

    TH1I* m_histTrigger;

    TH1F* m_histSpectrum;
    TH2F* m_ToTvsToF;

    TH1F* m_histCentSpectrum;
    TH2F* m_centToTvsToF;

    //
    // single pixel data
    UInt_t      m_row;
    UInt_t      m_col;
    UInt_t      m_ToT;
    ULong64_t   m_ToA;

    UInt_t      m_Rows[MAXHITS];
    UInt_t      m_Cols[MAXHITS];
    UInt_t      m_Size;
    UInt_t      m_ToTs[MAXHITS];
    ULong64_t   m_ToAs[MAXHITS];

    //
    // trigger data
    UInt_t      m_trigCnt;
    ULong64_t   m_trigTime;
};

#endif // DATAPROCESS_H
