#include "dataprocess.h"

#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "TFile.h"
#include "TCanvas.h"

using namespace std;

Int_t DataProcess::DataProcess(TString fileNameInput, ProcType process = procDat, UInt_t nEntries = 1000000000):
    m_fileNameInput(fileNameInput),
    m_process(process),
    m_maxEntries(nEntries)
{
    m_time.Start();

    //
    // create filenames
    processFileNames();

    //
    // switch to decide what to do with the input
    // either open dat file, root file or both
    // close files after finished with them
    // at closing of the root file, create plots
    switch (m_process)
    {
        case procDat:
            if (! openDat()) return -1;
            if (! openRoot()) return -1;
            processDat();
            closeDat();
            closeRoot();

            break;
        case procRoot:
            if (! openRoot()) return -1;
//            if (! openCsv()) return -1;
            processRoot();
            closeRoot();
//            closeCsv();

            break;
        case procAll:
            if (! openDat()) return -1;
            if (! openRoot()) return -1;
//            if (! openCsv()) return -1;
            processDat();
            processRoot();
            closeDat();
            closeRoot();
//            closeCsv();

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

Int_t DataProcess::readOptions(Bool_t bCol = kTRUE,
                               Bool_t bRow = kTRUE,
                               Bool_t bToT = kTRUE,
                               Bool_t bToA = kTRUE,
                               Bool_t bTrig = kTRUE,
                               Bool_t bTrigToA = kTRUE,
                               Int_t  nHitsCut = 0,
                               Bool_t noTrigWindow = kFALSE,
                               Int_t  windowCut = 40,
                               Bool_t singleFile = kFALSE,
                               Int_t  linesPerFile = 100000)
{
    m_bCol          = bCol;
    m_bRow          = bRow;
    m_bToT          = bToT;
    m_bToA          = bToA;
    m_bTrig         = bTrig;
    m_bTrigToA      = bTrigToA;
    m_noTrigWindow  = noTrigWindow;
    m_singleFile    = singleFile;
    m_nHitsCut      = nHitsCut;
    m_windowCut     = windowCut;
    m_linesPerFile  = linesPerFile;

    if (m_windowCut > 500)
    {
            cout << " ========================================== " << endl;
            cout << " === " << windowCut << " ms is very large winwdow!!!" << endl;
            cout << " ========================================== " << endl;
            return -1;
    }

    return 0;
}

void DataProcess::plotStandardData()
{
    TCanvas* canvas = new TCanvas("canvas", m_fileNamePdf, 1200, 800);
    canvas->Divide(3,2);

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

    canvas->cd(4);
    m_ToAvsToT->Draw();

    canvas->cd(5);
    m_histToT->Draw();

    canvas->cd(6);
    m_histToA->Draw();

//    if (m_trigCnt != 0)
//    {
//        canvas->cd(4);
//        m_histTrigger->Draw();
//    }

    canvas->Print(saveFileName);
    canvas->Close();
}

void DataProcess::processFileNames()
{
    UInt_t slash = m_fileNameInput.Last('/');
    m_fileNameInput.Remove(0,slash+1);
    UInt_t dotPos = m_fileNameInput.Last('.');

    m_fileNameDat   = m_fileNameInput;
    m_fileNameRoot  = m_fileNameInput.Replace(dotPos, 200, ".root");
    m_fileNamePdf   = m_fileNameInput.Replace(dotPos, 200, ".pdf");
    m_fileNameCsv   = m_fileNameInput.Replace(dotPos, 200, ".csv");
}

Int_t DataProcess::openDat()
{
    m_fileDat = fopen(m_fileNameDat, "r");
    cout << m_fileNameDat << endl;

    if (m_fileDat == NULL)
    {
        cout << " ========================================== " << endl;
        cout << " == COULD NOT OPEN DAT, PLEASE CHECK IT === " << endl;
        cout << " ========================================== " << endl;
        return -1;
    }
    return 0;
}

Int_t DataProcess::openCsv(TString fileCounter = "")
{
    TString fileNameTmp = m_fileNameInput;
    UInt_t dotPos = fileNameTmp.Last('.');
    if (fileCounter.Sizeof() != 0)
    {
        fileNameTmp.Replace(dotPos, 200, "[" + fileCounter + "].csv")
    }

    FILE* fileCsv = fopen(fileNameTmp, "w");
    cout << fileNameTmp << endl;

    if (fileCsv == NULL)
    {
        cout << " ========================================== " << endl;
        cout << " == COULD NOT OPEN CSV, PLEASE CHECK IT === " << endl;
        cout << " ========================================== " << endl;
        return -1;
    }

    m_filesCsv.push_back() = fileCsv;

    if (m_bTrig)    fprintf(m_filesCsv.back(), "#TrigId\t");
    if (m_bTrig)    fprintf(m_filesCsv.back(), "#TrigTime\t");
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
        m_fileRoot = new TFile(m_fileNameRoot, "RECREATE");
        cout << m_fileNameRoot << endl;

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
        m_rawTree->Branch("Col", &m_col, "Col/I");
        m_rawTree->Branch("Row", &m_row, "Row/I");
        m_rawTree->Branch("ToT", &m_ToT, "ToT/I");
        m_rawTree->Branch("ToA", &m_ToA, "ToA/l");    // l for long

        m_timeTree = new TTree("timetree", "Tpx3 camera, March 2017");
        m_timeTree->Branch("TrigCntr", &m_trigCnt, "TrigCntr/i");
        m_timeTree->Branch("TrigTime", &m_trigTime,"TrigTimeGlobal/l");

        m_longTimeTree = new TTree("m_longTimeTree", "tpx3 telescope, May 2015");
        m_longTimeTree->Branch("LongTime", &longtime, "LongTime/l");

        //
        // create plots
        m_pixelMap      = new TH2I("pixelMap", "Col:Row", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToT   = new TH2F("pixelMapToT", "Col:Row:ToT", 256, -0.5, 255.5, 256, -0.5, 255.5);
        m_pixelMapToA   = new TH2F("pixelMapToA", "Col:Row:ToA", 256, -0.5, 255.5, 256, -0.5, 255.5);

        m_ToAvsToT      = new TH2I("ToAvsToT", "ToA:ToT", 100, 0, 0, 100, 0, 0);

        m_histToT       = new TH1I("histToT", "ToT", 100, 0, 0);
        m_histToA       = new TH1I("histToA", "ToA", 100, 0, 0);
        m_histTrigger   = new TH1I("histTrigger", "Trigger", 1000, 0, 0);
    }
    else if (m_process == procRoot)
    {
        m_fileRoot = new TFile(m_fileNameRoot, "READ");
        cout << m_fileNameRoot << endl;

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
        m_rawTree->SetBranchAddress("Col", m_col);
        m_rawTree->SetBranchAddress("Row", m_row);
        m_rawTree->SetBranchAddress("ToT", m_ToT);
        m_rawTree->SetBranchAddress("ToA", m_ToA);

        m_timeTree = (TTree * ) m_fileRoot->Get("timetree");
        m_timeTree->SetBranchAddress("TrigCntr", &m_trigCnt);
        m_timeTree->SetBranchAddress("TrigTime", &m_trigTime);

        //
        // read plots
        m_pixelMap      = (TH2I *) m_fileRoot->Get("pixelMap");
        m_pixelMapToT   = (TH2F *) m_fileRoot->Get("pixelMapToT");
        m_pixelMapToA   = (TH2F *) m_fileRoot->Get("pixelMapToA");

        m_ToAvsToT      = (TH2I *) m_fileRoot->Get("ToAvsToT");

        m_histToT       = (TH1I *) m_fileRoot->Get("histToT");
        m_histToA       = (TH1I *) m_fileRoot->Get("histToA");
        m_histTrigger   = (TH1I *) m_fileRoot->Get("histTrigger");
    }

    m_fileRoot->cd();

    return 0;
}

void DataProcess::closeDat()
{
    fclose(m_fileDat);
}

void DataProcess::closeCsv()
{
    for (UInt_t size = 0; size < m_filesCsv.size(); size++)
    {
        fclose(m_filesCsv->front());
        m_filesCsv->pop_front();
    }
}

void DataProcess::closeRoot()
{
    plotStandardData();

    m_fileRoot->cd();
    m_fileRoot->Write();
    m_fileRoot->Close();
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
    UInt_t trigCntr = 0;
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
    UInt_t pcnt = 0;
    UInt_t sortThreshold = 2*sortSize;

    UInt_t backjumpcnt = 0;

    TNamed *deviceID = new TNamed("deviceID", "device ID not set");

    //
    // skip main header
    UInt_t sphdr_id;
    UInt_t sphdr_size;
    retVal = fread( &sphdr_id, sizeof(UInt_t), 1, m_fileNameDat);
    retVal = fread( &sphdr_size, sizeof(UInt_t), 1, m_fileNameDat);
    cout << hex << sphdr_id << dec << endl;
    cout << "header size " << sphdr_size << endl;
    if (sphdr_size > 66304) sphdr_size = 66304;
    UInt_t *fullheader = new UInt_t[sphdr_size/sizeof(UInt_t)];
    if (fullheader == 0) { cout << "failed to allocate memory for header " << endl; return -1; }

    retVal = fread ( fullheader+2, sizeof(UInt_t), sphdr_size/sizeof(UInt_t) -2, m_fileNameDat);
    fullheader[0] = sphdr_id;
    fullheader[1] = sphdr_size;
    // todo read in header via structure
    // for now just use fixed offset to find deviceID
    UInt_t waferno = (fullheader[132] >> 8) & 0xFFF;
    UInt_t id_y = (fullheader[132] >> 4) & 0xF;
    UInt_t id_x = (fullheader[132] >> 0) & 0xF;
    Char_t devid[16];
    sprintf(devid,"W%04d_%c%02d", waferno, (Char_t)id_x+64, id_y);  // make readable device identifier
    deviceID->SetTitle(devid);
    deviceID->Write();   // write deviceID to root file

    //
    // Main loop over all entries
    // nentries is either 1000000000 or user defined
    while (!feof(m_fileNameDat) && pcnt < m_maxEntries)
    {
        //
        // read entries, write out each 10^5
        retVal = fread( &pixdata, sizeof(ULong64_t), 1, m_fileNameDat);
        if (pcnt % 100000 == 0) cout << "Count " << pcnt << endl;

        //
        // reading data and saving them to deques or timeTrees
        if (retVal == 1)
        {
            header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;

            //
            // finding header type (data part - frames/data driven)
            if (header == 0xA || header == 0xB)
            {
                pcnt++;

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
                ToTs.push_back(ToT);
                // subtract fast ToA (FToA count until the first clock edge, so less counts means later arrival of the hit)
                ToAs.push_back((globaltime << 12) - (FToA << 8));
                // now correct for the column to column phase shift (todo: check header for number of clock phases)
                ToAs.back() += ( ( (Col/2) %16 ) << 8 );
                if (((Col/2)%16) == 0) ToAs.back() += ( 16 << 8 );
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
                    if (trigCntr == 0 && m_trigCnt != 1)
                    {
                        cout << "first trigger number in file is not 1" << endl;
                        trigCntr++; // added so the error does not repeat, could cause problem if trigCnt was 0 even for second iteration (maybe?)
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
                        trigCntr++;
                        m_trigTime = ((trigtime_global_ext + trigtime_coarse) << 12) | trigtime_fine ;

                        //
                        // fill timeTree and trigger histogram
                        m_timeTree->Fill();
                        m_histTrigger->Fill(m_trigTime);

                        prev_trigtime_coarse = trigtime_coarse;
//                        cout << "trigger number " << trigcntr << " at  " << trigtime_coarse  << " " << trigtime_fine << endl;
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
                    m_longTimeTree->Fill();
                }
            }
        }

        //
        // sorting the data, wait for either finishing the file or reaching sorting count
        // list contain 2*sortSize elements
        if ( (pcnt >= sortThreshold) || (retVal <= 0) )
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
                while (j > 0 && ToAs[j-1] > ToAs[j])
                {
                    swap(Cols[j-1],Cols[j]);
                    swap(Rows[j-1],Rows[j]);
                    swap(ToTs[j-1],ToTs[j]);
                    swap(ToAs[j-1],ToAs[j]);
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

                m_ToAvsToT->Fill(m_ToA,m_ToT);

                m_histToT->Fill(m_ToT);
                m_histToA->Fill(m_ToA);

                Cols.pop_front();
                Rows.pop_front();
                ToTs.pop_front();
                ToAs.pop_front();
            }

            //
            // increase the threshold size
            sortThreshold = pcnt + sortSize;

            //
            // end of read data
            if (Cols.size() == 0)
            {
                cout << "OK1 found " << pcnt << " pixel packets" << endl;
                cout << backjumpcnt << " backward time jumps" << endl;

                return 0;
            }
        }
    }

    cout << "Wrong exit... found " << pcnt << " pixel packets" << endl;
    return 0;
}

Int_t DataProcess::processRoot()
{
    UInt_t lineCounter = 0;
    UInt_t currChunk = 0;

    m_nRaw = m_rawTree->GetEntries();
    m_nTime = m_timeTree->GetEntries();
    if (m_nTime == 0) m_bTrig = m_bTrigToA = kFALSE;

    //
    // loop entries in timeTree
    UInt_t entryTime = 0;
    while (entryTime < m_nTime || m_nTime == 0)
    {
        if (m_nTime != 0) m_timeTree->GetEntry(entryTime);
        if (i % 50 == 0) cout << entryTime << " out of " << m_nTime << " done!" << endl;

        //
        // loop entries in rawTree
        for (UInt_t entryRaw = currChunk; entryRaw < m_nRaw; entryRaw++)
        {
            m_rawTree->GetEntry(entryRaw);

            //
            // dump all useless data
            if (m_nTime!= 0 && m_ToA < m_trigTime)
            {
                currChunk = entryRaw;
                continue;
            }

            //
            // stop rawTree loop if further away then trigger+window
            if (m_nTime!= 0 && m_ToA > (m_trigTime + m_windowCut))
            {
                currChunk = entryRaw;
                break;
            }
            else
            {
                //
                // single file creation
                if (m_singleFile && m_filesCsv.size() == 0)
                {
                    if (! openCsv()) return -1;
                }

                //
                // multiple file creation
                if (lineCounter % m_linesPerFile == 0 && !m_singleFile)
                {
                    if (! openCsv(TString::Format("%d", lineCounter / m_linesPerFile))) return -1;
                }

                //
                // write actual data
                if (m_bTrig)    fprintf(m_filesCsv.back(), "%u\t",  m_trigCnt);
                if (m_bTrig)    fprintf(m_filesCsv.back(), "%llu\t",m_trigTime);
                if (m_bCol)     fprintf(m_filesCsv.back(), "%u\t",  m_col);
                if (m_bRow)     fprintf(m_filesCsv.back(), "%u\t",  m_row);
                if (m_bToA)     fprintf(m_filesCsv.back(), "%llu\t",m_ToA);
                if (m_bToT)     fprintf(m_filesCsv.back(), "%u\t",  m_ToT);
                if (m_bTrigToA) fprintf(m_filesCsv.back(), "%d\t",  m_ToA - m_trigTime);
                fprintf(m_filesCsv.back(), "\n");
                lineCounter++;
            }
        }

        //
        // stop cycle by incrementing nTime if no trigger entries found
        if (m_nTime == 0) m_nTime++;
        entryTime++;
    }

    finishMsg(lineCounter, m_filesCsv.size());
    closeCsv();
    return 0;

//    //
//    // THIS IS DONE WHEN timetree IS EMPTY, WHICH IS WHEN TRIGGER IS NOT AVAILABLE DURING DATA TAKING
//    if (m_nTime == 0 || m_noTrigWindow)
//    {
//        //
//        // disable any trigger printing
//        m_bTrig = m_bTrigToA = kFALSE;

//        //
//        // loop entries in rawtree
//        for (UInt_t entry = 0; entry < m_nRaw; entry++)
//        {
//            m_rawTree->GetEntry(entry);
//            if (entry % 10000 == 0) cout << entry << " of " << m_nRaw << " done!" << endl;

//            //
//            // single file creation
//            if (m_singleFile && m_filesCsv.size() == 0)
//            {
//                if (! openCsv()) return -1;
//            }

//            //
//            // multiple file creation
//            if (lineCounter % m_linesPerFile == 0 && !m_singleFile)
//            {
//                if (! openCsv(TString::Format("%d", lineCounter / m_linesPerFile))) return -1;
//            }

//            //
//            // write actual data
//            if (m_bTrig)    fprintf(m_filesCsv.back(), "%u\t",  m_trigCnt);
//            if (m_bCol)     fprintf(m_filesCsv.back(), "%u\t",  m_col);
//            if (m_bRow)     fprintf(m_filesCsv.back(), "%u\t",  m_row);
//            if (m_bToA)     fprintf(m_filesCsv.back(), "%llu\t",m_ToA);
//            if (m_bToT)     fprintf(m_filesCsv.back(), "%u\t",  m_ToT);
////            if (m_bTrigToA) fprintf(m_filesCsv.back(), "%d\t",  m_ToA - m_trigTime);
//            fprintf(m_filesCsv.back(), "\n");
//            lineCounter++;
//        }

//        finishMsg(lineCounter, m_filesCsv.size());
//        closeCsv();
//        return 0;
//    }

//    for (UInt_t i = 0; i < m_nTime; i++)
//    {
//        m_timeTree->GetEntry(i);
//        if (i % 50 == 0) cout << i << " out of " << Ntime << " done!" << endl;

//        //
//        // loop entries in rawTree
//        for (UInt_t entry = currChunk; entry < m_nRaw; entry++)
//        {
//            m_rawTree->GetEntry(entry);

//            //
//            // dump all useless data
//            if (m_ToA < m_trigTime)
//            {
//                currChunk = entry;
//                continue;
//            }

//            if (m_ToA > (m_trigTime + m_windowCut))
//            {
//                currChunk = entry;
//                break;
//            }
//            else
//            {
//                //
//                // single file creation
//                if (m_singleFile && m_filesCsv.size() == 0)
//                {
//                    if (! openCsv()) return -1;
//                }

//                //
//                // multiple file creation
//                if (lineCounter % m_linesPerFile == 0 && !m_singleFile)
//                {
//                    if (! openCsv(TString::Format("%d", lineCounter / m_linesPerFile))) return -1;
//                }

//                //
//                // write actual data
//                if (m_bTrig)    fprintf(m_filesCsv.back(), "%u\t",  m_trigCnt);
//                if (m_bCol)     fprintf(m_filesCsv.back(), "%u\t",  m_col);
//                if (m_bRow)     fprintf(m_filesCsv.back(), "%u\t",  m_row);
//                if (m_bToA)     fprintf(m_filesCsv.back(), "%llu\t",m_ToA);
//                if (m_bToT)     fprintf(m_filesCsv.back(), "%u\t",  m_ToT);
//                if (m_bTrigToA) fprintf(m_filesCsv.back(), "%d\t",  m_ToA - m_trigTime);
//                fprintf(m_filesCsv.back(), "\n");
//                lineCounter++;
//            }
//        }
//    }
}

void DataProcess::finishMsg(UInt_t events, Int_t fileCounter = 1)
{
        cout << "==============================================="  << endl;
        cout << "============  JOB IS DONE !!!  ================"  << endl;
        cout << "==============================================="  << endl;
        cout << "====  " << events << " events from total of " << m_nRaw << " selected!"  << endl;
        cout << "====  " << fileCounter << " FILE(S) WRITTEN OUT!!!"  << endl;
        cout << "==============================================="  << endl;
}
