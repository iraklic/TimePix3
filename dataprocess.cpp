#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <cmath>

#include "dataprocess.h"

#include "TSystem.h"

#include "TFile.h"
#include "TCanvas.h"

using namespace std;

DataProcess::DataProcess()
{
    Init();
}

DataProcess::~DataProcess()
{
    delete m_sigHandler;
}

void DataProcess::Init()
{
    // set predefined options in case user definition is missing
    setNEntries(0xFFFFFFFF);
    setCorrection(CorrType::corrNew);
    setProcess(procAll);
    setOptions();

    m_sigHandler = new TSignalHandler(kSigInterrupt);
    m_sigHandler->Add();
    m_sigHandler->Connect("Notified()", "DataProcess", this, "StopLoop()");

    m_numInputs = 1;
    m_trigCnt = 0;
    m_maxEntries = 0;
    m_bFirstTrig = kFALSE;
    m_bDevID = kFALSE;
}

void DataProcess::setName(TString fileNameInput)
{
    m_numInputs = 1;
    m_fileNameInput.push_back(fileNameInput);
    std::cout << "file name input " << m_fileNameInput.back() << std::endl;
}

void DataProcess::setName(TObjString* fileNameInput, Int_t size)
{
    m_numInputs = size;

    for (Int_t input = 0; input < m_numInputs; input++)
    {
        m_fileNameInput.push_back( fileNameInput[input].GetString()) ;
        std::cout << "file name input " << m_fileNameInput.back() << " number " << input << std::endl;
    }
}

void DataProcess::setProcess(ProcType process)
{
    m_process = process;
}

void DataProcess::setCorrection(CorrType correction, TString fileNameCorrection)
{
    m_correction = correction;
    switch (m_correction)
    {
        case corrNew:
        {
            m_correctionName = fileNameCorrection;
            std::cout << "Creating new correction" << std::endl;
            if (fileNameCorrection.Sizeof() > 1 )
            {
                std::cout << " - with file" << m_correctionName << std::endl;
                openCorr(kTRUE);
                m_bCorrCsv = kTRUE;
            }
            else
            {
                m_bCorrCsv = kFALSE;
            }
            break;
        }
        case corrUse:
        {
            m_correctionName = fileNameCorrection;
            std::cout << "Using correction name " << m_correctionName << std::endl;
            openCorr(kFALSE);
            m_bCorrCsv = kTRUE;
            break;
        }
        case corrOff:
        {
            std::cout << "Ignoring correction" << std::endl;
            m_bCorrCsv = kFALSE;
            break;
        }
        default:
            break;
    }
}

void DataProcess::setNEntries(UInt_t nEntries)
{
    m_maxEntries = nEntries;
}

Int_t DataProcess::setOptions(Bool_t bCol,
                              Bool_t bRow,
                              Bool_t bToT,
                              Bool_t bToA,
                              Bool_t bTrig,
                              Bool_t bTrigTime,
                              Bool_t bTrigToA,
                              Bool_t bProcTree,
                              Bool_t bCsv,
                              Bool_t bCentroid,
                              Int_t  gapPixel,
                              Float_t gapTime,
                              Bool_t bNoTrigWindow,
                              Float_t timeWindow,
                              Float_t timeStart,
                              Bool_t singleFile,
                              Int_t linesPerFile)
{
    m_bCol          = bCol;
    m_bRow          = bRow;
    m_bToT          = bToT;
    m_bToA          = bToA;
    m_bTrig         = bTrig;
    m_bTrigTime     = bTrigTime;
    m_bTrigToA      = bTrigToA;
    m_bProcTree     = bProcTree;
    m_bCsv          = bCsv;
    m_bCentroid     = bCentroid;
    m_gapTime       = (ULong64_t) (gapTime * 163840); // conversion from us ToA size
    m_gapPix        = gapPixel;
    m_bNoTrigWindow = bNoTrigWindow;
    m_bSingleFile   = singleFile;
    m_timeWindow    = timeWindow;
    m_timeStart     = timeStart;
    m_linesPerFile  = linesPerFile;

    if (timeWindow/1000 > 500)
    {
            std::cout << " ========================================== " << std::endl;
            std::cout << " === " << timeWindow << " us is very large winwdow!!!" << std::endl;
            std::cout << " ========================================== " << std::endl;
            return -1;
    }

    return 0;
}

Int_t DataProcess::process()
{
    m_time.Start();
    m_pixelCounter = 0;

    //
    // create filenames
    if ( processFileNames() != 0) return -1;

    //
    // switch to decide what to do with the input
    // either open dat file, root file or both
    // close files after finished with them
    // at closing of the root file, create plots
    switch (m_process)
    {
        case procDat:
            if ( openDat()) return -1;
            if ( openRoot()) return -1;
            processDat();
            closeDat();

            break;
        case procRoot:
            if ( openRoot()) return -1;
            processRoot();

            break;
        case procAll:
            if ( openDat()) return -1;
            if ( openRoot()) return -1;
            processDat();
            processRoot();
            closeDat();

            break;
        default :
            std::cout << " ========================================== " << std::endl;
            std::cout << " ====== COULD NOT DECIDE WHAT TO DO ======= " << std::endl;
            std::cout << " ========================================== " << std::endl;

            return -2;
    }

    if (m_correction != corrOff)
        closeCorr();

    closeRoot();
    closeCsv();

    //
    // print time of the code execution to the console
    m_time.Stop();
    m_time.Print();

    return 0;
}

void DataProcess::plotStandardData()
{
    TCanvas* canvas;
    if (m_trigCnt != 0)
    {
        canvas = new TCanvas("canvas", m_fileNamePdf, 1200, 1200);
        canvas->Divide(3,3);
    }
    else
    {
        canvas = new TCanvas("canvas", m_fileNamePdf, 1200, 800);
        canvas->Divide(3,2);
    }

    canvas->cd(1);
    gPad->SetLogz();
    gPad->SetRightMargin(0.15);
    m_pixelMap->SetStats(kFALSE);
    m_pixelMap->Draw("colz");

    canvas->cd(2);
    gPad->SetLogz();
    gPad->SetRightMargin(0.15);
    m_pixelMapToT->SetStats(kFALSE);
    m_pixelMapToT->Draw("colz");

    if (m_bCentroid)
    {
        canvas->cd(3);
        gPad->SetLogz();
        gPad->SetRightMargin(0.15);
        m_pixelCentMap->SetStats(kFALSE);
        m_pixelCentMap->Draw("colz");
    }

    canvas->cd(5);
    gPad->SetRightMargin(0.15);
    m_histToT->Draw();

    canvas->cd(6);
    gPad->SetRightMargin(0.15);
    m_histToA->Draw();

    if (m_trigCnt != 0)
    {
        canvas->cd(4);
        gPad->SetRightMargin(0.15);
        m_histTrigger->Draw();

        if ( m_bTrigToA )
        {
            canvas->cd(7);
            gPad->SetRightMargin(0.15);
            m_histSpectrum->Draw();

            if (m_bCentroid)
            {
                canvas->cd(8);
                gPad->SetRightMargin(0.15);
                m_histCentSpectrum->Draw();
            }
        }
    }

    canvas->Print(m_fileNamePath + m_fileNamePdf);
    canvas->Close();
}

Int_t DataProcess::processFileNames()
{
    deque<Int_t > inputNum;
    TString tmpString;

    for (Int_t input = 0; input < m_numInputs; input++)
    {
        if (m_fileNameInput[input].Sizeof()==1) return -1;

        UInt_t dotPos  = m_fileNameInput[input].Last('.');
        UInt_t dashPos = m_fileNameInput[input].Last('-');
        tmpString = m_fileNameInput[input];
        TString tmpNum ( tmpString(dashPos+1,dotPos-dashPos-1) );
        inputNum.push_back( tmpNum.Atoi() );
    }

    for (Int_t input = 0; input < m_numInputs; input++)
    {
        UInt_t j = input;
        while (j > 0 && inputNum[j-1] > inputNum[j])
        {
            swap(inputNum[j-1],inputNum[j]);
            swap(m_fileNameInput[j-1],m_fileNameInput[j]);

            j--;
        }
    }

    tmpString = m_fileNameInput[0];
    if (tmpString.EndsWith(".dat"))
        m_type = dtDat;
    else if (tmpString.EndsWith(".tpx3"))
        m_type = dtTpx;

    UInt_t slash = tmpString.Last('/');
    m_fileNamePath = tmpString(0,slash+1);

    for (Int_t input = 0; input < m_numInputs; input++)
    {
        m_fileNameInput[input].Remove(0,slash+1);
        m_fileNameDat.push_back( m_fileNameInput[input] );

        //
        // if one file only, keep number
        UInt_t repPos;
        if (m_numInputs == 1)   repPos = m_fileNameInput[input].Last('.');
        else                    repPos = m_fileNameInput[input].Last('-');

        m_fileNameRoot      = m_fileNameInput[input].Replace(repPos, 200, ".root");
        m_fileNamePdf       = m_fileNameInput[input].Replace(repPos, 200, ".pdf");
        m_fileNameCsv       = m_fileNameInput[input].Replace(repPos, 200, ".csv");
        m_fileNameCentCsv   = m_fileNameInput[input].Replace(repPos, 200, "_cent.csv");
    }

    return 0;
}

Int_t DataProcess::openCorr(Bool_t create)
{
    if (create)
        m_fileCorr = fopen(m_correctionName, "w+");
    else
    {
        m_fileCorr = fopen(m_correctionName, "r");
        loadCorrection();
    }

    if (m_fileCorr == NULL)
    {
        std::cout << " ========================================== " << std::endl;
        std::cout << " == COULD NOT OPEN DAT, PLEASE CHECK IT === " << std::endl;
        std::cout << " ========================================== " << std::endl;
        return -1;
    }

    return 0;
}

Int_t DataProcess::openDat(Int_t fileCounter)
{
    FILE* fileDat = fopen(m_fileNamePath + m_fileNameDat[fileCounter], "r");
    std::cout << m_fileNameDat[fileCounter] << " at " << m_fileNamePath << std::endl;

    if (fileDat == NULL)
    {
        std::cout << " ========================================== " << std::endl;
        std::cout << " == COULD NOT OPEN DAT, PLEASE CHECK IT === " << std::endl;
        std::cout << " ========================================== " << std::endl;
        return -1;
    }

    m_filesDat.push_back(fileDat);

    return 0;
}

Int_t DataProcess::openCsv(DataType type, TString fileCounter)
{
    TString fileNameTmp;
    if (type == dtCent)
    {
        fileNameTmp = m_fileNameCentCsv;
    }
    else
    {
        fileNameTmp = m_fileNameCsv;
    }
    UInt_t dotPos = fileNameTmp.Last('.');

    if (fileCounter.Sizeof() != 1)
    {
        fileNameTmp.Replace(dotPos, 200, "-" + fileCounter + ".csv");
    }

    FILE* fileCsv = fopen(m_fileNamePath + fileNameTmp, "w");
    std::cout << fileNameTmp << " at " << m_fileNamePath << std::endl;

    if (fileCsv == NULL)
    {
        std::cout << " ========================================== " << std::endl;
        std::cout << " = COULD NOT OPEN CSV, "<< fileCounter << " PLEASE CHECK IT == " << std::endl;
        std::cout << " ========================================== " << std::endl;
        return -1;
    }

    deque<FILE* >* files;
    if (type == dtCent)
    {
        files = &m_filesCentCsv;
    }
    else
    {
        files = &m_filesCsv;
    }
    files->push_back(fileCsv);

    if (m_bTrig)    fprintf(files->back(), "#TrigId,");
    if (m_bTrigTime)fprintf(files->back(), "#TrigTime,");
    if (m_bCol)     fprintf(files->back(), "#Col,");
    if (m_bRow)     fprintf(files->back(), "#Row,");
    if (m_bToA)     fprintf(files->back(), "#ToA,");
    if (m_bToT)     fprintf(files->back(), "#ToT[arb],");
    if (m_bTrigToA) fprintf(files->back(), "#Trig-ToA[arb],");
    if (type == dtCent)
        if (m_bCentroid)fprintf(files->back(), "#Centroid,");
    if (m_correction != corrOff && m_bTrigToA) fprintf(files->back(), "#cTrig-ToA[us],");

    fprintf(files->back(), "\n");

    return 0;
}

Int_t DataProcess::openRoot()
{
    //
    // Open file
    if ( m_process == procDat || m_process == procAll)
    {
        m_fileRoot = new TFile(m_fileNamePath + m_fileNameRoot, "RECREATE");
        std::cout << m_fileNameRoot << " at " << m_fileNamePath << std::endl;

        //
        // check if file opened correctly
        if (m_fileRoot == NULL)
        {
            std::cout << " ========================================== " << std::endl;
            std::cout << " == COULD NOT OPEN ROOT, PLEASE CHECK IT == " << std::endl;
            std::cout << " ========================================== " << std::endl;
            return -1;
        }

        //
        // create trees
        m_rawTree = new TTree("rawtree", "Tpx3 camera, November 2017");
        m_rawTree->Branch("Size", &m_Size, "Size/i");
        m_rawTree->Branch("Col",   m_Cols, "Col[Size]/i");
        m_rawTree->Branch("Row",   m_Rows, "Row[Size]/i");
        m_rawTree->Branch("ToT",   m_ToTs, "ToT[Size]/i");
        m_rawTree->Branch("ToA",   m_ToAs, "ToA[Size]/l");    // l for long

        m_timeTree = new TTree("timetree", "Tpx3 camera, March 2017");
        m_timeTree->Branch("TrigCntr", &m_trigCnt, "TrigCntr/i");
        m_timeTree->Branch("TrigTime", &m_trigTime,"TrigTime/l");

        //
        // create plots
        m_pixelMap      = new TH2I("pixelMap", "Col:Row", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToT   = new TH2F("pixelMapToT", "Col:Row:{ToT}", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToA   = new TH2F("pixelMapToA", "Col:Row:{ToA}", 256, -0.5, 255.5, 256, -0.5, 255.5);

        m_histToT       = new TH1I("histToT", "ToT", 200, 0, 5000);
        m_histToA       = new TH1I("histToA", "ToA", 1000, 0, 0);

        m_pixelCentMap      = new TH2I("pixelCentMap", "Col:Row", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelCentMapToT   = new TH2F("pixelCentMapToT", "Col:Row:{ToT}", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelCentMapToA   = new TH2F("pixelCentMapToA", "Col:Row:{ToA}", 256, -0.5, 255.5, 256, -0.5, 255.5);

        m_histCentToT       = new TH1I("histCentToT", "ToT", 200, 0, 5000);
        m_histCentToA       = new TH1I("histCentToA", "ToA", 1000, 0, 0);

        m_scanDir = m_fileRoot->mkdir("ToTvsToA_scan");
        m_scanDir->cd();
        for (int totCounter = 0; totCounter < 1023; totCounter++)
        {
            m_histCentScan[totCounter] = new TH2F(Form("ToTvsToA_%d",totCounter*25),Form("ToTvsToA_%d",totCounter*25), 300, -234.375, 234.375, 400, 0, 10000);
        }
        m_fileRoot->cd();
        m_mapCorr = new TH2F("cToTvsToT", "cToT:ToT", 400, 0, 10000, 400, 0, 10000);
        m_mapCorrErr = new TH2F("cToTvsToTErr", "cToT:ToT:{Err}", 400, 0, 10000, 400, 0, 10000);

        m_histCentToTvsToA  = new TH2F("CentroidToTvsToA", "ToA:ToT", 300, -234.375, 234.375, 400, 0, 10000);
        m_histCorrToTvsToA  = new TH2F("CorrectedToTvsToA", "ToA:ToT", 300, -234.375, 234.375, 400, 0, 10000);

        m_histTrigger   = new TH1I("histTrigger", "Trigger", 1000, 0, 0);
    }
    else if (m_process == procRoot)
    {
        m_fileRoot = new TFile(m_fileNamePath + m_fileNameRoot, "UPDATE");
        std::cout << m_fileNameRoot << " at " << m_fileNamePath << std::endl;

        //
        // check if file opened correctly
        if (m_fileRoot == NULL)
        {
            std::cout << " ========================================== " << std::endl;
            std::cout << " == COULD NOT OPEN ROOT, PLEASE CHECK IT == " << std::endl;
            std::cout << " ========================================== " << std::endl;
            return -1;
        }

        //
        // read trees
        m_rawTree = (TTree * ) m_fileRoot->Get("rawtree");
        m_rawTree->SetBranchAddress("Size", &m_Size);
        m_rawTree->SetBranchAddress("Col",  m_Cols);
        m_rawTree->SetBranchAddress("Row",  m_Rows);
        m_rawTree->SetBranchAddress("ToT",  m_ToTs);
        m_rawTree->SetBranchAddress("ToA",  m_ToAs);

        m_timeTree = (TTree * ) m_fileRoot->Get("timetree");
        m_timeTree->SetBranchAddress("TrigCntr", &m_trigCnt);
        m_timeTree->SetBranchAddress("TrigTime", &m_trigTime);

        //
        // read plots
        m_pixelMap      = (TH2I *) m_fileRoot->Get("pixelMap");
        m_pixelMapToT   = (TH2F *) m_fileRoot->Get("pixelMapToT");
        m_pixelMapToA   = (TH2F *) m_fileRoot->Get("pixelMapToA");

        m_histToT       = (TH1I *) m_fileRoot->Get("histToT");
        m_histToA       = (TH1I *) m_fileRoot->Get("histToA");

        m_pixelCentMap      = (TH2I *) m_fileRoot->Get("pixelCentMap");
        m_pixelCentMapToT   = (TH2F *) m_fileRoot->Get("pixelCentMapToT");
        m_pixelCentMapToA   = (TH2F *) m_fileRoot->Get("pixelCentMapToA");

        m_histCentToT       = (TH1I *) m_fileRoot->Get("histCentToT");
        m_histCentToA       = (TH1I *) m_fileRoot->Get("histCentToA");

        m_histCentToTvsToA  = (TH2F *) m_fileRoot->Get("CentroidToTvsToA");
        m_histCorrToTvsToA  = (TH2F *) m_fileRoot->Get("CorrectedToTvsToA");

        m_histTrigger   = (TH1I *) m_fileRoot->Get("histTrigger");
    }

    m_fileRoot->cd();

    return 0;
}

void DataProcess::closeCorr()
{
    if (m_bCorrCsv)
        fclose(m_fileCorr);

    while (m_lookupTable.size() != 0)
    {
        m_lookupTable.pop_front();
    }
}

void DataProcess::closeDat()
{
    while (m_filesDat.size() != 0)
//    for (UInt_t size = 0; size < m_filesDat.size(); size++)
    {
        fclose(m_filesDat.front());
        m_filesDat.pop_front();
        m_fileNameInput.pop_front();
        m_fileNameDat.pop_front();
    }
}

void DataProcess::closeCsv()
{
    while (m_filesCsv.size() != 0)
    {
        fclose(m_filesCsv.front());
        m_filesCsv.pop_front();

    }
    while (m_bCentroid && m_filesCentCsv.size() != 0)
    {
        fclose(m_filesCentCsv.front());
        m_filesCentCsv.pop_front();
    }
}

void DataProcess::closeRoot()
{
    m_nCent = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();

    if (m_nCent != 0 || m_nTime != 0)
        plotStandardData();

    m_fileRoot->cd();
    m_fileRoot->Write();
    m_fileRoot->Close();
}

Int_t DataProcess::skipHeader()
{
    TNamed *deviceID = new TNamed("deviceID", "device ID not set");
    Int_t retVal;

    UInt_t sphdr_id;
    UInt_t sphdr_size;
    retVal = fread( &sphdr_id, sizeof(UInt_t), 1, m_filesDat.back());
    retVal = fread( &sphdr_size, sizeof(UInt_t), 1, m_filesDat.back());
    std::cout << hex << sphdr_id << dec << std::endl;
    std::cout << "header size " << sphdr_size << std::endl;
    if (sphdr_size > 66304) sphdr_size = 66304;
    UInt_t *fullheader = new UInt_t[sphdr_size/sizeof(UInt_t)];
    if (fullheader == 0) { std::cout << "failed to allocate memory for header " << std::endl; return -1; }

    retVal = fread ( fullheader+2, sizeof(UInt_t), sphdr_size/sizeof(UInt_t) -2, m_filesDat.back());
    fullheader[0] = sphdr_id;
    fullheader[1] = sphdr_size;

    if (!m_bDevID)
    {
        // todo read in header via structure
        // for now just use fixed offset to find deviceID
        UInt_t waferno = (fullheader[132] >> 8) & 0xFFF;
        UInt_t id_y = (fullheader[132] >> 4) & 0xF;
        UInt_t id_x = (fullheader[132] >> 0) & 0xF;
        Char_t devid[16];
        sprintf(devid,"W%04d_%c%02d", waferno, (Char_t)id_x+64, id_y);  // make readable device identifier
        deviceID->SetTitle(devid);
        deviceID->Write();   // write deviceID to root file
        m_bDevID = kTRUE;
    }

    return 0;
}

// find clusters recursively
void DataProcess::findCluster(UInt_t index, UInt_t stop, deque<UInt_t >* cols, deque<UInt_t >* rows, deque<ULong64_t >* toas, deque<UInt_t >* tots, deque<Bool_t >* centered, deque<UInt_t >* indices)
{
    Int_t colInd, rowInd, totInd;
    Int_t colK, rowK, totK;
    ULong64_t toaInd, toaK, toa, toaDiff;

    Int_t gap = 1 + (Int_t) m_gapPix;

    colInd = (Int_t) cols->at(index);
    rowInd = (Int_t) rows->at(index);
    toaInd = toas->at(index);
    totInd = tots->at(index);
    toa = 81920;

    for (UInt_t k = 0; k < stop; k++)
    {
        colK = (Int_t) cols->at(k);
        rowK = (Int_t) rows->at(k);
        toaK = toas->at(k);
        totK = tots->at(k);

        if (toaK > toaInd)
        {
            toaDiff = toaK - toaInd;
        }
        else
        {
            toaDiff = toaInd - toaK;
        }

        if (!(centered->at(k)) && ((std::abs(colInd - colK) <= gap) && (std::abs(rowInd - rowK) <= gap )) && toaDiff < toa)
        {
            centered->at(k) = kTRUE;
            indices->push_back(k);
            findCluster(k,stop,cols,rows,toas,tots,centered,indices);
        }
    }
}


Int_t DataProcess::processDat()
{
    //
    // define all locally needed variables
    ULong64_t headdata = 0;
    ULong64_t pixdata;
    ULong64_t longtime = 0;
    ULong64_t longtime_lsb = 0;
    ULong64_t longtime_msb = 0;
    ULong64_t globaltime = 0;
    Int_t pixelbits;
    Int_t longtimebits;
    Int_t diff;
    UInt_t data;
    UInt_t ToA_coarse;
    UInt_t FToA;
    UInt_t dcol, col;
    UInt_t spix, row;
    UInt_t ToT, ToA;
    UInt_t pix;
    UInt_t spidr_time;   // additional timestamp added by SPIDR
    UInt_t header;
    UInt_t subheader;
    UInt_t trigtime_coarse;
    UInt_t prev_trigtime_coarse = 0 ;
    UInt_t trigtime_fine;
    ULong64_t trigtime_global_ext = 0;
    ULong64_t tmpfine;

    Int_t retVal;

    //
    // deque used for sorting and writing data
    deque<UInt_t>      Rows;
    deque<UInt_t>      Cols;
    deque<UInt_t>      ToTs;
    deque<ULong64_t>   ToAs;

    UInt_t sortSize = MAXHITS;
    UInt_t sortThreshold = 2*sortSize;

    //
    // used for centroiding
    deque<Bool_t>   Centered;
    deque<UInt_t>   centeredIndices;
    UInt_t          centeredIndex;

    ULong64_t tmpToA;
    Float_t   tmpToT;
    UInt_t    posX, posY;
    Bool_t    indexFound;


    UInt_t backjumpcnt = 0;
    Int_t curInput = 0;

    //
    // skip main header and obtain device ID
    if (m_type == dtDat)
        skipHeader();

    //
    // Main loop over all entries
    // nentries is either 1000000000 or user defined
    while (!feof(m_filesDat.back()) && (m_pixelCounter < m_maxEntries || m_maxEntries == 0))
    {
        //
        // read entries, write out each 10^5
        retVal = fread( &pixdata, sizeof(ULong64_t), 1, m_filesDat.back());
        if (m_pixelCounter % 100000 == 0) std::cout << "File "<< curInput << "/" << m_numInputs <<" Count " << m_pixelCounter << std::endl;

        //
        // reading data and saving them to deques or timeTrees
        if (retVal == 1)
        {
            header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;

            //
            // finding header type (data part - frames/data driven)
            if (header == 0xA || header == 0xB)
            {
                m_pixelCounter++;

                //
                // calculate col and row
                dcol        = ((pixdata & 0x0FE0000000000000) >> 52); //(16+28+9-1)
                spix        = ((pixdata & 0x001F800000000000) >> 45); //(16+28+3-2)
                pix         = ((pixdata & 0x0000700000000000) >> 44); //(16+28)
                col         = (dcol + pix/4);
                row         = (spix + (pix & 0x3));

                //
                // calculate ToA
                data        = ((pixdata & 0x00000FFFFFFF0000) >> 16);
                spidr_time  = (pixdata & 0x000000000000FFFF);
                ToA         = ((data & 0x0FFFC000) >> 14 );
                ToA_coarse  = (spidr_time << 14) | ToA;
                FToA        = (data & 0xF);
                globaltime  = (longtime & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);

                //
                // poor ol' ToT is all alone :(
                ToT         = (data & 0x00003FF0) >> 4;

                //
                // calculate the global time
                pixelbits = ( ToA_coarse >> 28 ) & 0x3;   // units 25 ns
                longtimebits = ( longtime >> 28 ) & 0x3;  // units 25 ns;
                diff = longtimebits - pixelbits;
                if( diff == 1  || diff == -3)  globaltime = ( (longtime - 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
                if( diff == -1 || diff == 3 )  globaltime = ( (longtime + 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
                if ( ( ( globaltime >> 28 ) & 0x3 ) != ( (ULong64_t) pixelbits ) )
                {
                    std::cout << "Error, checking bits should match .. " << std::endl;
                    std::cout << hex << globaltime << " " << ToA_coarse << " " << dec << ( ( globaltime >> 28 ) & 0x3 ) << " " << pixelbits << " " << diff << std::endl;
                }

                //
                // fill deques
                Centered.push_back(kFALSE);
                Cols.push_back(col);
                Rows.push_back(row);
                ToTs.push_back(ToT*25); // save in ns
                // subtract fast ToA (FToA count until the first clock edge, so less counts means later arrival of the hit)
                ToAs.push_back((globaltime << 12) - (FToA << 8));
                // now correct for the column to column phase shift (todo: check header for number of clock phases)
                ToAs.back() += ( ( (col/2) %16 ) << 8 );
                if (((col/2)%16) == 0) ToAs.back() += ( 16 << 8 );
            }

            //
            // finding header type (time part - trigger/global time)
            // packets with time information, old and new format: 0x4F and 0x6F
            if ( header == 0x4  || header == 0x6 )
            {
                subheader = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;

                //
                // finding subheader type (F for trigger or 4,5 for time)
                if ( subheader == 0xF )         // trigger information
                {
                    m_trigCnt = ((pixdata & 0x00FFF00000000000) >> 44) & 0xFFF;
                    trigtime_coarse = ((pixdata & 0x00000FFFFFFFF000) >> 12) & 0xFFFFFFFF;
                    tmpfine = (pixdata >> 5 ) & 0xF;   // phases of 320 MHz clock in bits 5 to 8
                    tmpfine = ((tmpfine-1) << 9) / 12;
                    trigtime_fine = (pixdata & 0x0000000000000E00) | (tmpfine & 0x00000000000001FF);

                    //
                    // check if the first trigger number is 1
                    if (! m_bFirstTrig && m_trigCnt != 1)
                    {
                        std::cout << "first trigger number in file is not 1" << std::endl;
                    } else
                    {
                        if ( trigtime_coarse < prev_trigtime_coarse )   // 32 time counter wrapped
                        {
                            if ( trigtime_coarse < prev_trigtime_coarse-1000 )
                            {
                                trigtime_global_ext += 0x100000000;
                                std::cout << "coarse trigger time counter wrapped: " << m_trigCnt << " " << trigtime_coarse << " " << prev_trigtime_coarse << std::endl;
                            }
                            else std::cout << "small backward time jump in trigger packet " << m_trigCnt << " (jump of " << prev_trigtime_coarse-trigtime_coarse << ")" << std::endl;
                        }
                        m_bFirstTrig = kTRUE;
                        m_trigTime = ((trigtime_global_ext + (ULong64_t) trigtime_coarse) << 12) | (ULong64_t) trigtime_fine; // save in ns

                        //
                        // fill timeTree and trigger histogram
                        m_timeTree->Fill();
                        m_histTrigger->Fill(m_trigTime);

                        prev_trigtime_coarse = trigtime_coarse;
                    }
                } else if ( subheader == 0x4 )  // 32 lsb of timestamp
                {
                    longtime_lsb = (pixdata & 0x0000FFFFFFFF0000) >> 16;
                } else if ( subheader == 0x5 )  // 32 lsb of timestamp
                {
                    longtime_msb = (pixdata & 0x00000000FFFF0000) << 16;
                    ULong64_t tmplongtime = longtime_msb | longtime_lsb;

                    //
                    // now check for large forward jumps in time;
                    // 0x10000000 corresponds to about 6 seconds
                    if ( (tmplongtime > ( longtime + 0x10000000)) && (longtime > 0) )
                    {
                        std::cout << "Large forward time jump" << std::endl;
                        longtime = (longtime_msb - 0x10000000) | longtime_lsb;
                    }
                    else longtime = tmplongtime;
                }
            }
        }
        else if (++curInput < m_numInputs)
        {
            m_nRaw = m_rawTree->GetEntries();
            m_nTime = m_timeTree->GetEntries();
            finishMsg(m_fileNameDat[curInput-1], "processing data", m_pixelCounter, curInput);
            openDat(curInput);
            if (m_type == dtDat)
                skipHeader();
            continue;
        }

        //
        // sorting the data, wait for either finishing the file or reaching sorting count
        // list contain 2*sortSize elements
        if ( (m_pixelCounter >= sortThreshold) || (retVal <= 0) )
        {
            //
            // processing either sort size or hitlist size
            UInt_t actSortedSize;
            if (retVal <= 0) actSortedSize = Cols.size();
            else actSortedSize = sortSize;

            //
            // insert sort the doublechunk
            for (UInt_t i = 0; i < Cols.size() ; i++)
            {
                UInt_t j = i;
                while (j > 0 && ToAs[j-1] > ToAs[j])//(ToAs[j-1] & 0xFFFF) > (ToAs[j] & 0xFFFF))
                {
                    swap(Cols[j-1],Cols[j]);
                    swap(Rows[j-1],Rows[j]);
                    swap(ToTs[j-1],ToTs[j]);
                    swap(ToAs[j-1],ToAs[j]);

                    j--;
                }
            }

            //
            // saving half of the sorted data to root file
            // first do centroiding
            for (UInt_t i = 0; i < actSortedSize ; i++)
            {
                indexFound  = kFALSE;
                posX        = Cols.front();
                posY        = Rows.front();
                ToT         = ToTs.front();

                if (m_bCentroid)
                {
                    // get all hits in one time window
                    UInt_t j = 0;
                    while (!Centered.front() && j < Cols.size())
                    {
                        // reaching the window gap
                        if ((ToAs[j] - ToAs.front()) > m_gapTime)
                        {
                            break;
                        }

                        j++;
                    }

                    // find cluster obtaining index i (current pixel always in the cluster - if not chosen in the previous run)
                    if (!Centered.front())
                    {
                        centeredIndices.push_back(0);
                        Centered.front() = kTRUE;
                    }

                    findCluster(0, j, &Cols, &Rows, &ToAs, &ToTs, &Centered, &centeredIndices);

                    // get centroid
                    ToT       = 0;
                    posX      = 0;
                    posY      = 0;
                    for (UInt_t k = 0; k < centeredIndices.size(); k++)
                    {
                        ToT      += ToTs[centeredIndices[k]];
                        posX     += ToTs[centeredIndices[k]]*Cols[centeredIndices[k]];
                        posY     += ToTs[centeredIndices[k]]*Rows[centeredIndices[k]];
                    }

                    if (centeredIndices.size() > 0 )
                    {
                        posX = (UInt_t) ((((Float_t) posX) / ToT) + 0.5);
                        posY = (UInt_t) ((((Float_t) posY) / ToT) + 0.5);
                    }
                }
                else
                {
                    centeredIndices.push_back(0);
                }

                //
                // save data - centroid will have index 0, others following - not necessarilly time-sorted
                ToT = 0;
                centeredIndex = 0;
                m_Size = centeredIndices.size();

                for (UInt_t k = 0; k < m_Size; k++)
                {
                    if (!indexFound && Cols[centeredIndices.front()] == posX && Rows[centeredIndices.front()] == posY)
                    {
                        centeredIndex = centeredIndices.front();
                        indexFound = kTRUE;
                    }

                    // if index does not fit cluster, take as center pixel with highest ToT
                    if (!indexFound && ToT < ToTs[centeredIndices.front()])
                    {
                        ToT = ToTs[centeredIndices.front()];
                        centeredIndex = centeredIndices.front();
                    }

                    m_Cols[k] = Cols[centeredIndices.front()];
                    m_Rows[k] = Rows[centeredIndices.front()];
                    m_ToTs[k] = ToTs[centeredIndices.front()];
                    m_ToAs[k] = ToAs[centeredIndices.front()];

                    if (k != 0 && centeredIndex == centeredIndices.front())
                    {
                        swap(m_Cols[k],m_Cols[0]);
                        swap(m_Rows[k],m_Rows[0]);
                        swap(m_ToTs[k],m_ToTs[0]);
                        swap(m_ToAs[k],m_ToAs[0]);
                    }

                    centeredIndices.pop_front();
                }

                // save processed data to root and clear for next use
                tmpToA = m_ToAs[0];
                tmpToT = m_ToTs[0];

                for (UInt_t k = 0; k < m_Size; k++)
                {

                    if (k == 0)
                    {
                        m_rawTree->Fill();

                        if (m_bCentroid)
                        {
                            m_pixelCentMap   ->Fill(m_Cols[k],m_Rows[k]);
                            m_pixelCentMapToT->Fill(m_Cols[k],m_Rows[k],m_ToTs[k]);
                            m_pixelCentMapToA->Fill(m_Cols[k],m_Rows[k],m_ToAs[k]);

                            m_histCentToT->Fill(m_ToTs[k]);
                            m_histCentToA->Fill(m_ToAs[k]);

                        }
                    }

                    m_pixelMap   ->Fill(m_Cols[k],m_Rows[k]);
                    m_pixelMapToT->Fill(m_Cols[k],m_Rows[k],m_ToTs[k]);
                    m_pixelMapToA->Fill(m_Cols[k],m_Rows[k],m_ToAs[k]);

                    m_histToT->Fill(m_ToTs[k]);
                    m_histToA->Fill(m_ToAs[k]);

                    if (m_bCentroid && k != 0)
                    {
                        m_histCentToTvsToA->Fill((m_ToAs[k]-tmpToA)*(25.0/4096), ( m_ToTs[k]));
                        m_histCentScan[(UInt_t) (tmpToT/25)]->Fill((m_ToAs[k]-tmpToA)*(25.0/4096), ( m_ToTs[k]));
                    }

                    m_Cols[k] = 0;
                    m_Rows[k] = 0;
                    m_ToTs[k] = 0;
                    m_ToAs[k] = 0;
                }

                // dump raw data
                Cols.pop_front();
                Rows.pop_front();
                ToTs.pop_front();
                ToAs.pop_front();
                Centered.pop_front();
            }

            //
            // increase the threshold size
            sortThreshold = m_pixelCounter + sortSize;

            //
            // end of read data
            if (Cols.size() == 0)
            {
                m_nRaw = m_rawTree->GetEntries();
                m_nTime = m_timeTree->GetEntries();
                std::cout << "===" << backjumpcnt << " backward time jumps found ==" << std::endl;

                finishMsg(m_fileNameDat[curInput-1], "processing data", m_pixelCounter);

                if (m_bCentroid) m_histCentToTvsToA->Write();
                if (m_correction == corrNew)
                    createCorrection();
                return 0;
            }
        }
    }

    m_nCent = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();
    std::cout << "Not all pixel packets found" << std::endl;
    finishMsg(m_fileNameDat[curInput], "processing data", m_pixelCounter);

    if (m_bCentroid) m_histCentToTvsToA->Write();
    if (m_correction == corrNew)
        createCorrection();

    return 0;
}

Int_t DataProcess::processRoot()
{
    UInt_t lineCounter = 0;
    UInt_t lineCounterCent = 0;
    UInt_t currChunkRaw = 0;
    UInt_t currChunkCent = 0;
    UInt_t entryTime = 0;
    ULong64_t tmpTrigTime = 0;

    ULong64_t lfTimeWindow = (ULong64_t) ((m_timeWindow*4096000.0/25.0) + 0.5);
    ULong64_t lfTimeStart  = (ULong64_t) ((m_timeStart *4096000.0/25.0) + 0.5);

    Int_t binCount;
    Float_t binMin;
    Float_t binMax;

    UInt_t   ToT;
    Float_t dToA = 0;
    Float_t dCent = 0;
    Float_t ToF[MAXHITS];

    m_nCent = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();

    m_fileRoot->cd();

    if (m_bProcTree)
    {
        std::cout << "Creating procTree" << std::endl;

        m_procTree = new TTree("proctree", "Tpx3 camera, January 2018");
        m_procTree->Branch("Size", &m_Size, "Size/i");
        m_procTree->Branch("Col",   m_Cols, "Col[Size]/i");
        m_procTree->Branch("Row",   m_Rows, "Row[Size]/i");
        m_procTree->Branch("ToT",   m_ToTs, "ToT[Size]/i");
        m_procTree->Branch("ToA",   m_ToAs, "ToA[Size]/l");    // l for long unsigned
        m_procTree->Branch("ToF",      ToF, "ToF[Size]/F");    // F for float
        m_procTree->Branch("TrigCntr", &entryTime, "TrigCntr/i");
        m_procTree->Branch("TrigTime", &m_trigTime,"TrigTime/l");
    }

    if (!m_bCol && !m_bRow && !m_bToA && !m_bToT && !m_bTrigToA) m_nCent = 0;

    if (m_nTime == 0) m_bTrig = m_bTrigTime = m_bTrigToA = kFALSE;

    if (m_bTrigToA)
    {
        if (m_bNoTrigWindow)
        {
            binCount = 100000;
            binMin   = 0;
            binMax   = 0;
        }
        else
        {
            binCount = (UInt_t) (640 * m_timeWindow);
            binMin   = m_timeStart;
            binMax   = ((Float_t) binCount/16000)*25 + m_timeStart;
        }

        m_histSpectrum     = new TH1F("histSpectrum", "ToF (ToA - trigTime)", binCount, binMin, binMax);
        m_ToTvsToF         = new TH2F("mapToT_ToF", "ToT vs ToF", binCount, binMin, binMax, 400, 0.0, 10.0);
        m_histCorrSpectrum = new TH1F("histCorrSpectrum", "ToF corrected (ToA - trigTime)", binCount, binMin, binMax);
        m_corrToTvsToF     = new TH2F("mapCorrToT_ToF", "ToT vs ToF corrected", binCount, binMin, binMax, 400, 0.0, 10.0);

        if (m_bCentroid)
        {
            m_histCentSpectrum = new TH1F("histCentSpectrum", "centroid ToF (ToA - trigTime)", binCount, binMin, binMax);
            m_centToTvsToF     = new TH2F("mapCentToT_ToF", "centroid ToT vs ToF", binCount, binMin, binMax, 400, 0.0, 10.0);
        }
    }

    //
    // loop entries in timeTree
    while (entryTime < m_nTime || m_nTime == 0)
    {
        if (entryTime % 10000 == 0) std::cout << "Time entry " << entryTime << " out of " << m_nTime << " done!" << std::endl;

        if (m_nTime != 0)
        {
            if (m_bNoTrigWindow && ((entryTime + 1) < m_nTime))
            {
                m_timeTree->GetEntry(entryTime + 1);
                tmpTrigTime = m_trigTime;
                m_timeTree->GetEntry(entryTime);
                lfTimeWindow = tmpTrigTime - m_trigTime;
            }
            else
            {
                m_timeTree->GetEntry(entryTime);
            }

            if (m_bCsv && m_nCent == 0)
            {
                //
                // single file creation
                if (m_filesCsv.size() == 0)
                {
                    if (openCsv()) return -1;
                }
                //
                // write trigger data if rawtree is empty
                if (m_bTrig)    fprintf(m_filesCsv.back(), "%u, ",  m_trigCnt);
                if (m_bTrigTime)fprintf(m_filesCsv.back(), "%llu, ",m_trigTime);

                fprintf(m_filesCsv.back(), "\n");
            }
        }

        //
        // loop entries in centTree
        for (UInt_t entryCent = currChunkCent; entryCent < m_nCent; entryCent++)
        {
            m_rawTree->GetEntry(entryCent);
            if (entryCent % 100000 == 0 && m_nTime == 0) std::cout << "Entry " << entryCent << " of " << m_nCent << " done!" << std::endl;

            //
            // dump all useless data
            if ( m_nTime!= 0 && m_ToAs[0] < m_trigTime + lfTimeStart)
            {
                currChunkCent = entryCent;
                continue;
            }

            //
            // stop rawTree loop if further away then trigger+window
            if ( m_nTime!= 0 && m_ToAs[0] > (m_trigTime +  lfTimeStart + lfTimeWindow))
            {
                currChunkCent = entryCent;
                break;
            }
            else
            {
                //
                // write centroid data
                if (m_bCentroid)
                {
                    if (m_bCsv )
                    {
                        //
                        // single file creation
                        if (m_bSingleFile && m_filesCentCsv.size() == 0)
                        {
                            if (openCsv(dtCent)) return -1;
                        }

                        //
                        // multiple file creation
                        if ((lineCounterCent % m_linesPerFile == 0) && !m_bSingleFile)
                        {
                            if (openCsv(dtCent, TString::Format("%d", lineCounterCent / m_linesPerFile))) return -1;
                        }
                        //
                        // write centroid data
                        if (m_bTrig)    fprintf(m_filesCentCsv.back(), "%u,",  m_trigCnt);
                        if (m_bTrigTime)fprintf(m_filesCentCsv.back(), "%llu,",m_trigTime);
                        if (m_bCol)     fprintf(m_filesCentCsv.back(), "%u,",  m_Cols[0]);
                        if (m_bRow)     fprintf(m_filesCentCsv.back(), "%u,",  m_Rows[0]);
                        if (m_bToA)     fprintf(m_filesCentCsv.back(), "%llu,",m_ToAs[0]);
                        if (m_bToT)     fprintf(m_filesCentCsv.back(), "%u,",  m_ToTs[0]);
                        if (m_bTrigToA) fprintf(m_filesCentCsv.back(), "%llu,",m_ToAs[0] - m_trigTime);
                        if (m_bCentroid)fprintf(m_filesCentCsv.back(), "%u,",  m_Size);
                    }

                    if (m_correction != corrOff)
                    {
                        ToT = m_ToTs[0] / 25.0;
                        if (ToT < m_lookupTable.size())
                            dCent = m_lookupTable.at(ToT).dToA;
                        else
                            dCent = 0;
                    }

                    if (m_bTrigToA)
                    {
                        ToF[0] = (((Float_t) ( m_ToAs[0] - m_trigTime)*25.0)/4096000.0) + dCent;
                        m_histCentSpectrum->Fill( ToF[0] );
                        m_centToTvsToF->Fill(ToF[0], ((Float_t) m_ToTs[0])/1000.0 );

                        if (m_bCsv && m_correction != corrOff) fprintf(m_filesCentCsv.back(), "%f,",ToF[0]);
                    }

                    if (m_bCsv )
                        fprintf(m_filesCentCsv.back(), "\n");
                    lineCounterCent++;
                }

                if (m_bCsv)
                {
                    //
                    // writing raw data to a file

                    //
                    // single file creation
                    if (lineCounter % m_linesPerFile == 0 && !m_bSingleFile)
                    {
                        if (openCsv(dtStandard, TString::Format("%d", lineCounter / m_linesPerFile))) return -1;
                    }

                    //
                    // multiple file creation
                    if (m_bSingleFile && m_filesCsv.size() == 0)
                    {
                        if (openCsv(dtStandard)) return -1;
                    }
                }

                //
                // write raw data
                ULong64_t tmpToAs0 = m_ToAs[0];
                for (UInt_t entryRaw = 0; entryRaw < m_Size; entryRaw++)
                {
                    if (m_bCsv)
                    {
                        if (m_bTrig)    fprintf(m_filesCsv.back(), "%u,",  m_trigCnt);
                        if (m_bTrigTime)fprintf(m_filesCsv.back(), "%llu,",m_trigTime);
                        if (m_bCol)     fprintf(m_filesCsv.back(), "%u,",  m_Cols[entryRaw]);
                        if (m_bRow)     fprintf(m_filesCsv.back(), "%u,",  m_Rows[entryRaw]);
                        if (m_bToA)     fprintf(m_filesCsv.back(), "%llu,",m_ToAs[entryRaw]);
                        if (m_bToT)     fprintf(m_filesCsv.back(), "%u,",  m_ToTs[entryRaw]);
                        if (m_bTrigToA) fprintf(m_filesCsv.back(), "%llu,",m_ToAs[entryRaw] - m_trigTime);
                    }

                    if (m_correction != corrOff)
                    {
                        ToT = m_ToTs[entryRaw] / 25;

                        if (ToT < m_lookupTable.size())
                        {
                            dToA  = m_lookupTable.at(ToT).dToA;
                            if (entryRaw != 0)
                                dCent = m_lookupTable.at(m_ToTs[0] / 25).dToA;
                            else
                                dCent = 0;
                        }
                        else
                        {
                            dToA = 0;
                            dCent = 0;
                            std::cout << "Sth went wrong in choosing dToA: ToT counter == " << ToT << ", LT size == " << m_lookupTable.size() <<std::endl;
                        }

                        if (entryRaw != 0)
                            m_histCorrToTvsToA->Fill(((Float_t)(m_ToAs[entryRaw] - tmpToAs0)*(25.0/4096)) + (dToA * 1e3), m_ToTs[entryRaw]);
                    }

                    if (m_bTrigToA)
                    {
                        ToF[entryRaw] = (((Float_t) ( m_ToAs[entryRaw] - m_trigTime )* 25.0 )/4096000.0);
                        m_histSpectrum->Fill( ToF[entryRaw] );
                        m_ToTvsToF->Fill(ToF[entryRaw], ((Float_t) m_ToTs[entryRaw])/1000.0 );
                        ToF[entryRaw] += dToA + dCent;
                        m_histCorrSpectrum->Fill(ToF[entryRaw] );
                        m_corrToTvsToF->Fill(ToF[entryRaw], ((Float_t) m_ToTs[entryRaw])/1000.0 );

                        if (m_bCsv && m_correction != corrOff) fprintf(m_filesCsv.back(), "%f,",ToF[entryRaw]);
                    }

                    m_ToAs[entryRaw] += (ULong64_t) ((1 + dCent + dToA)*(4096000.0/25.0));

                    if (m_bCsv)
                        fprintf(m_filesCsv.back(), "\n");
                    lineCounter++;
                }

                if(m_bProcTree)
                {
                    m_procTree->Fill();
                }

                for (UInt_t entryRaw = 0; entryRaw < MAXHITS; entryRaw++)
                {
                    ToF[entryRaw] = 0;
                }

            }
        }

        //
        // stop cycle by incrementing nTime if no trigger entries found
        if (m_nTime == 0 ) m_nTime = 1;
        entryTime++;
    }

    finishMsg(m_fileNameRoot,"processing root raw", lineCounterCent, m_filesCsv.size());

    return 0;
}

void DataProcess::finishMsg(TString fileName, TString operation, UInt_t events, Int_t fileCounter)
{
    std::cout << "==============================================="  << std::endl;
    std::cout << fileName << std::endl;
    std::cout << "==============================================="  << std::endl;
    std::cout << "====== "<< operation << " IS DONE !!!  ==========="  << std::endl;
    std::cout << "==============================================="  << std::endl;
    std::cout << "====  " << events << " events from total of " << m_nCent << " selected!"  << std::endl;
    if (operation == "processing data")
    {
        std::cout << "=========== " << fileCounter << " FILE(S) PROCESSED!!! ============"  << std::endl;
    }
    else
    {
        std::cout << "========== " << fileCounter << " FILE(S) WRITTEN OUT!!! ==========="  << std::endl;
    }
    std::cout << "==============================================="  << std::endl;
}

ULong64_t DataProcess::roundToNs(ULong64_t number)
{
//    return number;
    return (ULong64_t) ((number * (25.0/4096)) + 0.5);
}

void DataProcess::StopLoop()
{
    std::cout << "==============================================="  << std::endl;
    std::cout << "======== LOOP SUCCESSFULLY STOPPED ============"  << std::endl;
    std::cout << "==============================================="  << std::endl;
    gSystem->ExitLoop();
}

void DataProcess::loadCorrection()
{
    std::cout << "==============================================="  << std::endl;
    std::cout << "======== LOADING OLD CORRECTION TABLE ========="  << std::endl;
    std::cout << "==============================================="  << std::endl;

    LookupTable tmpLookupTable;
    while ( fscanf(m_fileCorr,"%u, %f\n", &tmpLookupTable.ToT, &tmpLookupTable.dToA) == 2)
    {
        m_lookupTable.push_back(tmpLookupTable);
        std::cout << "ToT: "  << tmpLookupTable.ToT << std::endl;
        std::cout << "dToA: " << tmpLookupTable.dToA << std::endl;
    }
}

void DataProcess::createCorrection()
{
    std::cout << "==============================================="  << std::endl;
    std::cout << "======== CREATING NEW CORRECTION TABLE ========"  << std::endl;
    std::cout << "==============================================="  << std::endl;

    LookupTable tmpLookupTable;
    int cntToT = 1;
    Bool_t process = kTRUE;
    Int_t tmpBinContent;
    Int_t binCounter;

    tmpLookupTable.ToT  = 0;
    tmpLookupTable.dToA = 0;
    m_lookupTable.push_back(tmpLookupTable);

    std::cout << "==============================================="  << std::endl;

    while (process)
    {
        Bool_t inner = kTRUE;
        int cnt = 1;
        while (inner)
        {
            binCounter = 0;
            Float_t tmpToA = 0;

//            m_histCentScan[cntToT]->GetYaxis()->SetRange(cnt+1,cnt+1);
//            m_histCentScan[cntToT]->GetXaxis()->SetRange(m_histCentScan[cntToT]->GetXaxis()->GetFirst(),m_histCentScan[cntToT]->GetXaxis()->GetLast());

            for (Int_t binx = m_histCentScan[cntToT]->GetXaxis()->GetFirst(); binx < m_histCentScan[cntToT]->GetXaxis()->GetLast(); binx++)
            {
                tmpBinContent = m_histCentScan[cntToT]->GetBinContent(binx,cnt);
                if (tmpBinContent > 10)
                {
                    tmpToA += (((1.5625 * ((Float_t) binx)) - 234.375) * tmpBinContent);
                    binCounter += tmpBinContent;
                }
            }
            if (binCounter != 0)
            {
                tmpToA /= binCounter;
                m_mapCorrErr->Fill(cntToT*25,cnt*25,std::sqrt(binCounter));
            }
            else
            {
                tmpToA = 0;
                m_mapCorrErr->Fill(cntToT*25,cnt*25,0);
            }

            m_mapCorr->Fill(cntToT*25,cnt*25,tmpToA);

            if (++cnt > cntToT)
                inner = kFALSE;
        }
//        m_histCentScan[cntToT]->GetYaxis()->SetRange();
//        m_histCentScan[cntToT]->GetXaxis()->SetRange();

        m_histCentToTvsToA->GetYaxis()->SetRange(cntToT+1,cntToT+1);
        m_histCentToTvsToA->GetXaxis()->SetRange(m_histCentToTvsToA->GetXaxis()->GetFirst(),m_histCentToTvsToA->GetXaxis()->GetLast());

        tmpLookupTable.ToT = cntToT * 25;
        binCounter = 0;
        tmpLookupTable.dToA = 0;
//        tmpLookupTable.dToA = m_histCentToTvsToA->GetMean();
        for (Int_t binx = m_histCentToTvsToA->GetXaxis()->GetFirst(); binx < m_histCentToTvsToA->GetXaxis()->GetLast(); binx++)
        {
            tmpBinContent = m_histCentToTvsToA->GetBinContent(binx,cntToT+1);
            if (tmpBinContent > 10)
            {
                tmpLookupTable.dToA += (((1.5625 * ((Float_t) binx)) - 234.375) * tmpBinContent);
                binCounter += tmpBinContent;
            }
        }
        if (binCounter != 0)
            tmpLookupTable.dToA /= binCounter;

//        Int_t binx, biny, binz;
//        m_histCentToTvsToA->GetMaximumBin(binx, biny, binz);
//        tmpLookupTable.dToA = (1.5625 * ((Float_t) binx)) - 15.625;

//        m_histCentToTvsToA->GetXaxis()->SetRange(11,m_histCentToTvsToA->GetXaxis()->GetLast());
//        m_histCentToTvsToA->GetMaximumBin(binx, biny, binz);
//        tmpLookupTable.dCent = (1.5625 * ((Float_t) binx)) - 15.625;

        m_lookupTable.push_back(tmpLookupTable);

        if (++cntToT > 1023)
            process = kFALSE;
    }

    Float_t shift = 0;//m_lookupTable.at(60).dToA;
    for (UInt_t index = 0; index < m_lookupTable.size(); index++)
    {
        // correcting to middle value
        m_lookupTable.at(index).dToA = shift - m_lookupTable.at(index).dToA ;
        m_lookupTable.at(index).dToA /= 1e3;

//        m_lookupTable.at(index).dCent = shift - m_lookupTable.at(index).dCent ;
//        m_lookupTable.at(index).dCent /= 1e3;

        if (m_bCorrCsv)
            fprintf(m_fileCorr,"%u, %f\n", m_lookupTable.at(index).ToT, m_lookupTable.at(index).dToA);
        std::cout << "==============================================="  << std::endl;
        std::cout << "ToT: "  << m_lookupTable.at(index).ToT << std::endl;
//        std::cout << "ToA: "  << shift - (1e3 * m_lookupTable.at(index).dToA)  << std::endl;
        std::cout << "dToA: " << m_lookupTable.at(index).dToA  << std::endl;
//        std::cout << "dCent: "<< m_lookupTable.at(index).dCent << std::endl;
    }

    m_histCentToTvsToA->GetYaxis()->SetRange();
    m_histCentToTvsToA->GetXaxis()->SetRange();

}
