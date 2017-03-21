#include "dataprocess.h"

#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "dataprocess.h"

#include "TSystem.h"

#include "TFile.h"
#include "TCanvas.h"

using namespace std;

DataProcess::DataProcess(TString fileNameInput)
{
    m_maxEntries = 1000000000;
    m_process = procAll;
//    m_fileNameInput.push_back(fileNameInput);
    m_numInputs = 1;

    m_sigHandler = new TSignalHandler(kSigInterrupt);
    m_sigHandler->Add();
    m_sigHandler->Connect("Notified()", "DataProcess", this, "StopLoop()");

    m_pixelCounter = 0;
    m_trigCnt = 0;
    m_bFirstTrig = kFALSE;
    m_bDevID = kFALSE;
}

void DataProcess::setName(TString fileNameInput)
{
    m_numInputs = 1;
    m_fileNameInput.push_back(fileNameInput);
    cout << "file name input " << m_fileNameInput.back() << endl;
}

void DataProcess::setName(TObjString* fileNameInput, Int_t size)
{
    m_numInputs = size;

    for (Int_t input = 0; input < m_numInputs; input++)
    {
        m_fileNameInput.push_back( fileNameInput[input].GetString()) ;
        cout << "file name input " << m_fileNameInput.back() << " number " << input << endl;
    }
}

void DataProcess::setProcess(ProcType process)
{
    m_process = process;
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
    m_bNoTrigWindow = bNoTrigWindow;
    m_bSingleFile   = singleFile;
    m_timeWindow    = timeWindow;
    m_timeStart     = timeStart;
    m_linesPerFile  = linesPerFile;

    if (timeWindow/1000 > 500)
    {
            cout << " ========================================== " << endl;
            cout << " === " << timeWindow << " us is very large winwdow!!!" << endl;
            cout << " ========================================== " << endl;
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
            closeRoot();

            break;
        case procRoot:
            if ( openRoot()) return -1;
            processRoot();
            closeRoot();

            break;
        case procAll:
            if ( openDat()) return -1;
            if ( openRoot()) return -1;
            processDat();
            processRoot();
            closeDat();
            closeRoot();

            break;
        default :
            cout << " ========================================== " << endl;
            cout << " ====== COULD NOT DECIDE WHAT TO DO ======= " << endl;
            cout << " ========================================== " << endl;

            return -2;
    }

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

    canvas->cd(3);
    gPad->SetLogz();
    gPad->SetRightMargin(0.15);
    m_pixelMapToA->SetStats(kFALSE);
    m_pixelMapToA->Draw("colz");

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

//            canvas->cd(8);
//            gPad->SetRightMargin(0.15);
//            gPad->SetLogz();
//            m_ToTvsToF->SetStats(kFALSE);
//            m_ToTvsToF->Draw("colz");
        }
    }

    canvas->Print(m_fileNamePdf);
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

        m_fileNameRoot  = m_fileNameInput[input].Replace(repPos, 200, ".root");
        m_fileNamePdf   = m_fileNameInput[input].Replace(repPos, 200, ".pdf");
        m_fileNameCsv   = m_fileNameInput[input].Replace(repPos, 200, ".csv");
    }

    return 0;
}

Int_t DataProcess::openDat(Int_t fileCounter)
{
    FILE* fileDat = fopen(m_fileNamePath + m_fileNameDat[fileCounter], "r");
    cout << m_fileNameDat[fileCounter] << " at " << m_fileNamePath << endl;

    if (fileDat == NULL)
    {
        cout << " ========================================== " << endl;
        cout << " == COULD NOT OPEN DAT, PLEASE CHECK IT === " << endl;
        cout << " ========================================== " << endl;
        return -1;
    }

    m_filesDat.push_back(fileDat);

    return 0;
}

Int_t DataProcess::openCsv(TString fileCounter)
{
    TString fileNameTmp = m_fileNameCsv;
    UInt_t dotPos = fileNameTmp.Last('.');

    if (fileCounter.Sizeof() != 1)
    {
        fileNameTmp.Replace(dotPos, 200, "-" + fileCounter + ".csv");
    }

    FILE* fileCsv = fopen(m_fileNamePath + fileNameTmp, "w");
    cout << fileNameTmp << " at " << m_fileNamePath << endl;

    if (fileCsv == NULL)
    {
        cout << " ========================================== " << endl;
        cout << " = COULD NOT OPEN CSV, "<< fileCounter << " PLEASE CHECK IT == " << endl;
        cout << " ========================================== " << endl;
        return -1;
    }

    m_filesCsv.push_back(fileCsv);

    if (m_bTrig)    fprintf(m_filesCsv.back(), "#TrigId\t");
    if (m_bTrigTime)fprintf(m_filesCsv.back(), "#TrigTime\t");
    if (m_bCol)     fprintf(m_filesCsv.back(), "#Col\t");
    if (m_bRow)     fprintf(m_filesCsv.back(), "#Row\t");
    if (m_bToA)     fprintf(m_filesCsv.back(), "#ToA\t");
    if (m_bToT)     fprintf(m_filesCsv.back(), "#ToT\t");
    if (m_bTrigToA) fprintf(m_filesCsv.back(), "#Trig-ToA\t");
    fprintf(m_filesCsv.back(), "\n");

    return 0;
}

Int_t DataProcess::openRoot()
{
    //
    // Open file
    if ( m_process == procDat || m_process == procAll)
    {
        m_fileRoot = new TFile(m_fileNamePath + m_fileNameRoot, "RECREATE");
        cout << m_fileNameRoot << " at " << m_fileNamePath << endl;

        //
        // check if file opened correctly
        if (m_fileRoot == NULL)
        {
            cout << " ========================================== " << endl;
            cout << " == COULD NOT OPEN ROOT, PLEASE CHECK IT == " << endl;
            cout << " ========================================== " << endl;
            return -1;
        }

        //
        // create trees
        m_rawTree = new TTree("rawtree", "Tpx3 camera, March 2017");
        m_rawTree->Branch("Col", &m_col, "Col/i");
        m_rawTree->Branch("Row", &m_row, "Row/i");
        m_rawTree->Branch("ToT", &m_ToT, "ToT/i");
        m_rawTree->Branch("ToA", &m_ToA, "ToA/l");    // l for long

        m_timeTree = new TTree("timetree", "Tpx3 camera, March 2017");
        m_timeTree->Branch("TrigCntr", &m_trigCnt, "TrigCntr/i");
        m_timeTree->Branch("TrigTime", &m_trigTime,"TrigTime/l");

        //
        // create plots
        m_pixelMap      = new TH2I("pixelMap", "Col:Row", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToT   = new TH2F("pixelMapToT", "Col:Row:{ToT}", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToA   = new TH2F("pixelMapToA", "Col:Row:{ToA}", 256, -0.5, 255.5, 256, -0.5, 255.5);

        m_histToT       = new TH1I("histToT", "ToT", 1200, 0, 20000);
        m_histToA       = new TH1I("histToA", "ToA", 100, 0, 0);
//        m_ToAvsToT      = new TH2F("ToAvsToT", "ToA:ToT", 1200, 0, 20000, 100, 0, 0);

        m_histTrigger   = new TH1I("histTrigger", "Trigger", 1000, 0, 0);
//        m_histTriggerToA= new TH1F("histTriggerToA", "TriggerToA", 100000, 0, 0);
    }
    else if (m_process == procRoot)
    {
        m_fileRoot = new TFile(m_fileNamePath + m_fileNameRoot, "UPDATE");
        cout << m_fileNameRoot << " at " << m_fileNamePath << endl;

        //
        // check if file opened correctly
        if (m_fileRoot == NULL)
        {
            cout << " ========================================== " << endl;
            cout << " == COULD NOT OPEN ROOT, PLEASE CHECK IT == " << endl;
            cout << " ========================================== " << endl;
            return -1;
        }

        //
        // read trees
        m_rawTree = (TTree * ) m_fileRoot->Get("rawtree");
        m_rawTree->SetBranchAddress("Col", &m_col);
        m_rawTree->SetBranchAddress("Row", &m_row);
        m_rawTree->SetBranchAddress("ToT", &m_ToT);
        m_rawTree->SetBranchAddress("ToA", &m_ToA);

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
//        m_ToAvsToT      = (TH2F *) m_fileRoot->Get("ToAvsToT");

        m_histTrigger   = (TH1I *) m_fileRoot->Get("histTrigger");
//        m_histTriggerToA= new TH1F("histTriggerToA", "TriggerToA", 100000, 0, 0);
    }

    m_fileRoot->cd();

    return 0;
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
//    for (UInt_t size = 0; size < m_filesCsv.size(); size++)
    {
        fclose(m_filesCsv.front());
        m_filesCsv.pop_front();
    }
}

void DataProcess::closeRoot()
{
    m_nRaw = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();

    if (m_nRaw != 0 || m_nTime != 0)
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
    cout << hex << sphdr_id << dec << endl;
    cout << "header size " << sphdr_size << endl;
    if (sphdr_size > 66304) sphdr_size = 66304;
    UInt_t *fullheader = new UInt_t[sphdr_size/sizeof(UInt_t)];
    if (fullheader == 0) { cout << "failed to allocate memory for header " << endl; return -1; }

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


Int_t DataProcess::processDat()
{
    //
    // define all locally needed variables
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

    UInt_t backjumpcnt = 0;

    Int_t curInput = 0;

    //
    // skip main header and obtain device ID
    skipHeader();

    //
    // Main loop over all entries
    // nentries is either 1000000000 or user defined
    while (!feof(m_filesDat.back()) && m_pixelCounter < m_maxEntries)
    {
        //
        // read entries, write out each 10^5
        retVal = fread( &pixdata, sizeof(ULong64_t), 1, m_filesDat.back());
        if (m_pixelCounter % 100000 == 0) cout << "Count " << m_pixelCounter << endl;

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
                    cout << "Error, checking bits should match .. " << endl;
                    cout << hex << globaltime << " " << ToA_coarse << " " << dec << ( ( globaltime >> 28 ) & 0x3 ) << " " << pixelbits << " " << diff << endl;
                }

                //
                // fill deques
                Cols.push_back(col);
                Rows.push_back(row);
                ToTs.push_back(ToT*25); // save in ms
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
                        cout << "first trigger number in file is not 1" << endl;
                    } else
                    {
                        if ( trigtime_coarse < prev_trigtime_coarse )   // 32 time counter wrapped
                        {
                            if ( trigtime_coarse < prev_trigtime_coarse-1000 )
                            {
                                trigtime_global_ext += 0x100000000;
                                cout << "coarse trigger time counter wrapped: " << m_trigCnt << " " << trigtime_coarse << " " << prev_trigtime_coarse << endl;
                            }
                            else cout << "small backward time jump in trigger packet " << m_trigCnt << " (jump of " << prev_trigtime_coarse-trigtime_coarse << ")" << endl;
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
                        cout << "Large forward time jump" << endl;
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
            // dump first half of sorted data to the tree
            for (UInt_t i = 0; i < actSortedSize; i++)
            {
                m_col = Cols.front();
                m_row = Rows.front();
                m_ToT = ToTs.front();
                m_ToA = ToAs.front();
                m_rawTree->Fill();

                m_pixelMap->Fill(m_col,m_row);
                m_pixelMapToT->Fill(m_col,m_row,m_ToT);
                m_pixelMapToA->Fill(m_col,m_row,m_ToA);

//                m_ToAvsToT->Fill(m_ToA,m_ToT);

                m_histToT->Fill(m_ToT);
                m_histToA->Fill(m_ToA);

                Cols.pop_front();
                Rows.pop_front();
                ToTs.pop_front();
                ToAs.pop_front();
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
                cout << "===" << backjumpcnt << " backward time jumps found ==" << endl;

                finishMsg(m_fileNameDat[curInput-1], "processing data", m_pixelCounter);
                return 0;
            }
        }
    }

    m_nRaw = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();
    cout << "Not all pixel packets found" << endl;
    finishMsg(m_fileNameDat[curInput], "processing data", m_pixelCounter);
    return 0;
}

Int_t DataProcess::processRoot()
{
    m_lineCounter = 0;
    UInt_t uselessChunk = 0;
    UInt_t currChunk = 0;
    UInt_t entryTime = 0;
    ULong64_t tmpTrigTime = 0;

    ULong64_t lfTimeWindow = (ULong64_t) ((m_timeWindow*1000000.0/6.1) + 0.5);
    ULong64_t lfTimeStart  = (ULong64_t) ((m_timeStart *1000000.0/6.1) + 0.5);

    Int_t binCount;
    Float_t binMin;
    Float_t binMax;

    Float_t ToF;

    m_nRaw = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();

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

        m_histSpectrum = new TH1F("histSpectrum", "ToF (ToA - trigTime)", binCount, binMin, binMax);
        m_ToTvsToF     = new TH2F("mapToT_ToF", "ToT vs ToF", binCount, binMin, binMax, 400, 0, 10);
    }

    //
    // loop entries in timeTree
    while (entryTime < m_nTime || m_nTime == 0)
    {
        if (entryTime % 10000 == 0) cout << "Time entry " << entryTime << " out of " << m_nTime << " done!" << endl;

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
//            cout << "========================" << endl;
//            cout << "Time window is " << m_lfTimeWindow << endl;
        }

        //
        // loop entries in rawTree
        for (UInt_t entryRaw = currChunk; entryRaw < m_nRaw; entryRaw++)
        {
            m_rawTree->GetEntry(entryRaw);
            if (entryRaw % 10000 == 0 && m_nTime == 0) cout << "Raw entry " << entryRaw << " of " << m_nRaw << " done!" << endl;

            //
            // dump all useless data
            if ( m_nTime!= 0 && m_ToA < m_trigTime + lfTimeStart)
            {
                currChunk = entryRaw;
                continue;
            }

            //
            // stop rawTree loop if further away then trigger+window
            if ( m_nTime!= 0 && m_ToA > (m_trigTime +  lfTimeStart + lfTimeWindow))
            {
//                cout << "useless entries " << currChunk - uselessChunk << endl;
//                cout << "entries in chunk " << entryRaw - currChunk << endl;
//                cout << "========================" << endl;

                currChunk = entryRaw;
//                uselessChunk = currChunk;
                break;
            }
            else
            {
                //
                // single file creation
                if (m_bSingleFile && m_filesCsv.size() == 0)
                {
                    if (openCsv()) return -1;
                }

                //
                // multiple file creation
                if (m_lineCounter % m_linesPerFile == 0 && !m_bSingleFile)
                {
                    if (openCsv(TString::Format("%d", m_lineCounter / m_linesPerFile))) return -1;
                }

                //
                // write actual data
                if (m_bTrig)    fprintf(m_filesCsv.back(), "%u\t",  m_trigCnt);
                if (m_bTrigTime)fprintf(m_filesCsv.back(), "%llu\t",m_trigTime);
                if (m_bCol)     fprintf(m_filesCsv.back(), "%u\t",  m_col);
                if (m_bRow)     fprintf(m_filesCsv.back(), "%u\t",  m_row);
                if (m_bToA)     fprintf(m_filesCsv.back(), "%llu\t",m_ToA);
                if (m_bToT)     fprintf(m_filesCsv.back(), "%u\t",  m_ToT);
                if (m_bTrigToA) fprintf(m_filesCsv.back(), "%llu\t",m_ToA - m_trigTime);

//                if (m_bTrigToA) m_histTriggerToA->Fill(m_ToA - m_trigTime);
                if (m_bTrigToA)
                {
                    ToF = ((Float_t) ( m_ToA - m_trigTime)/1e6 )*6.1;
                    m_histSpectrum->Fill( ToF );
                    m_ToTvsToF->Fill(ToF, ((Float_t) m_ToT)/1e3 );
                }

                fprintf(m_filesCsv.back(), "\n");
                m_lineCounter++;
            }
        }

        //
        // stop cycle by incrementing nTime if no trigger entries found
        if (m_nTime == 0 ) m_nTime = 1;
        entryTime++;
    }

    finishMsg(m_fileNameRoot,"processing root", m_lineCounter, m_filesCsv.size());
    closeCsv();
    return 0;
}

void DataProcess::finishMsg(TString fileName, TString operation, UInt_t events, Int_t fileCounter)
{
    cout << "==============================================="  << endl;
    cout << fileName << endl;
    cout << "==============================================="  << endl;
    cout << "====== "<< operation << " IS DONE !!!  ==========="  << endl;
    cout << "==============================================="  << endl;
    cout << "====  " << events << " events from total of " << m_nRaw << " selected!"  << endl;
    if (operation == "processing data")
    {
        cout << "=========== " << fileCounter << " FILE(S) PROCESSED!!! ============"  << endl;
    }
    else
    {
        cout << "========== " << fileCounter << " FILE(S) WRITTEN OUT!!! ==========="  << endl;
    }
    cout << "==============================================="  << endl;
}

ULong64_t DataProcess::roundToNs(ULong64_t number)
{
//    return number;
    return (ULong64_t) (((ULong64_t) ((number * 6.1) + 0.5) / 1000.0) + 0.5);
}

void DataProcess::StopLoop()
{
    cout << "==============================================="  << endl;
    cout << "======== LOOP SUCCESSFULLY STOPPED ============"  << endl;
    cout << "==============================================="  << endl;
    gSystem->ExitLoop();
}
