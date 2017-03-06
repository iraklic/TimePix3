// version5: added cluster ToT
// version4: decode long timestamps
// version3: separate tree for triggerTDC packets
// encodes decodes pixel packets ToA+ToT only

#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <deque>

#include "TFile.h"
#include "TStopwatch.h"

#include "TTree.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
//#include "TH2I.h"
//#include "TH2F.h"

#define MAXHITS 65536       // minimal value for sorting

using namespace std;

void plot()
{
    ;
}

int process(string filename, int max_dist=3, UInt_t nentries=1000000000)
{
    TStopwatch time;
    time.Start();

    //
    // Define all variables
    //

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
    UInt_t dcol;
    UInt_t spix;
    UInt_t pix;
    UInt_t spidr_time;   // additional timestamp added by SPIDR
    UInt_t header;
    UInt_t subheader;
    UInt_t trigcntr;
    UInt_t trigtime_coarse;
    UInt_t prev_trigtime_coarse =0 ;
    UInt_t trigtime_fine;
    ULong64_t trigtime_global;
    ULong64_t trigtime_global_ext = 0;
    ULong64_t tmpfine;
    Int_t retval;
    Int_t trigcnt=0;

    UInt_t      Row;
    UInt_t      Col;
    UInt_t      ToT;
    ULong64_t   ToA;

    std::deque<UInt_t>      Rows;
    std::deque<UInt_t>      Cols;
    std::deque<UInt_t>      ToTs;
    std::deque<ULong64_t>   ToAs;

    UInt_t sortSize = MAXHITS;
    UInt_t pcnt = 0;
    UInt_t sortThreshold = 2*sortSize;

    UInt_t backjumpcnt = 0;

    //
    // Create root file
    //

    TNamed *deviceID = new TNamed("deviceID", "device ID not set");

    string filename1 = filename.c_str();
    size_t slash = filename1.find_last_of("/");
    string Filename =  filename1.substr(slash+1);
    size_t dotPos = Filename.find_last_of(".");
    string rootfilename = Filename.replace(dotPos, 200, ".root");
    string saveFileName = Filename.replace(dotPos, 200, ".pdf");

    TFile *f = new TFile(rootfilename.c_str(), "RECREATE");
    if (f == NULL) { cout << "could not open root file " << endl; return -1; }
    f->cd();


    //
    // Trees
    //

    TTree *rawtree = new TTree("rawtree", "tpx3 camera, March 2017");
    rawtree->Branch("Col", &Col, "Col/I");
    rawtree->Branch("Row", &Row, "Row/I");
    rawtree->Branch("ToT", &ToT, "ToT/I");
    rawtree->Branch("ToA", &ToA, "ToAs/I");

    TTree *timetree = new TTree("timetree", "tpx3 camera, March 2017");
    timetree->Branch("TrigCntr", &trigcntr, "TrigCntr/i");
    timetree->Branch("TrigTimeGlobal", &trigtime_global, "TrigTimeGlobal/l");

    TTree *longtimetree = new TTree("longtimetree", "tpx3 telescope, May 2015");
    longtimetree->Branch("LongTime", &longtime, "LongTime/l");


    //
    // Plots
    //

    TH2I* pixelMap = new TH2I("pixelMap",
                               "Col:Row",
                                256, -0.5, 255.5,
                                256, -0.5, 255.5);

    TH2F* pixelMapToT= new TH2F("pixelMapToT",
                                "Col:Row:ToT",
                                256, -0.5, 255.5,
                                256, -0.5, 255.5);

    TH1I* histToT= new TH1I("histToT",
                            "ToT",
                            100, 0, 0);

    TH1I* histToA= new TH1I("histToA",
                            "ToA",
                            100, 0, 0);

    TH1I* histTrigger= new TH1I("histTrigger",
                                "Trigger",
                                1000, 0, 0);

    //
    // Open data file
    //

    FILE *fp = fopen(filename.c_str(), "r");
    cout << filename.c_str() << endl;
    if (fp == NULL) { cout << "can not open file: " << filename.c_str() << endl; return -1;}

    UInt_t sphdr_id;
    UInt_t sphdr_size;
    retval = fread( &sphdr_id, sizeof(UInt_t), 1, fp);
    retval = fread( &sphdr_size, sizeof(UInt_t), 1, fp);
    cout << hex << sphdr_id << dec << endl;
    cout << "header size " << sphdr_size << endl;
    if (sphdr_size > 66304) sphdr_size = 66304;
    UInt_t *fullheader = new UInt_t[sphdr_size/sizeof(UInt_t)];
    if (fullheader == 0) { cout << "failed to allocate memory for header " << endl; return -1; }

    retval = fread ( fullheader+2, sizeof(UInt_t), sphdr_size/sizeof(UInt_t) -2, fp);
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
    while (!feof(fp) && pcnt < nentries){
        retval = fread( &pixdata, sizeof(ULong64_t), 1, fp);
        if (pcnt % 100000 == 0) cout << "count " << pcnt << endl;

        // check if the data was read correctly
        if (retval == 1) {
            header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
            // finding header type (data part - frames/data driven)
            if (header == 0xA || header == 0xB) {
                pcnt++;

                dcol        = ((pixdata & 0x0FE0000000000000) >> 52); //(16+28+9-1)
                spix        = ((pixdata & 0x001F800000000000) >> 45); //(16+28+3-2)
                pix         = ((pixdata & 0x0000700000000000) >> 44); //(16+28)
                Col         = (dcol + pix/4);
                Row         = (spix + (pix & 0x3));
                data        = ((pixdata & 0x00000FFFFFFF0000) >> 16);
                spidr_time  = (pixdata & 0x000000000000FFFF);
                ToA         = ((data & 0x0FFFC000) >> 14 );
                ToA_coarse  = (spidr_time << 14) | ToA;
                FToA        = (data & 0xF);
                ToT         = (data & 0x00003FF0) >> 4;
                globaltime  = (longtime & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);

                // calculate the global time
                pixelbits = ( ToA_coarse >> 28 ) & 0x3;   // units 25 ns
                longtimebits = ( longtime >> 28 ) & 0x3;  // units 25 ns;
                diff = longtimebits - pixelbits;
                if( diff == 1  || diff == -3)  globaltime = ( (longtime - 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
                if( diff == -1 || diff == 3 )  globaltime = ( (longtime + 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
                if ( ( ( globaltime >> 28 ) & 0x3 ) != ( (ULong64_t) pixelbits ) ) {
                    cout << "Error, checking bits should match .. " << endl; 
                    cout << hex << globaltime << " " << ToA_coarse << " " << dec << ( ( globaltime >> 28 ) & 0x3 ) << " " << pixelbits << " " << diff << endl;
                }

                // fill deques
                Cols.push_back(Col);
                Rows.push_back(Row);
                ToTs.push_back(ToT);
                // subtract fast ToA (FToA count until the first clock edge, so less counts means later arrival of the hit)
                ToAs.push_back((globaltime << 12) - (FToA << 8));   // PS
                // now correct for the column to column phase shift (todo: check header for number of clock phases)
                ToAs.back() += ( ( (Col/2) %16 ) << 8 );   // PS
                if (((Col/2)%16) == 0) ToAs.back() += ( 16 << 8 );   // PS
            }

            // finding header type (time part - trigger/global time)
            // packets with time information, old and new format: 0x4F and 0x6F
            if ( header == 0x4  || header == 0x6 ) {
                subheader = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;
                // finding subheader type (F for trigger or 4,5 for time)
                if ( subheader == 0xF ) {
                    trigcntr = ((pixdata & 0x00FFF00000000000) >> 44) & 0xFFF;
                    trigtime_coarse = ((pixdata & 0x00000FFFFFFFF000) >> 12) & 0xFFFFFFFF;
                    tmpfine = (pixdata >> 5 ) & 0xF;   // phases of 320 MHz clock in bits 5 to 8
                    tmpfine = ((tmpfine-1) << 9) / 12;
                    trigtime_fine = (pixdata & 0x0000000000000E00) | (tmpfine & 0x00000000000001FF); 

                    // will write with every trigcntr increase, not only first one
                    if (trigcnt==0 && trigcntr!=1) {
                        cout << "first trigger number in file is not 1" << endl;
                        trigcnt++; // possible fix of the above error
                    } 
                    else { 
		        if ( trigtime_coarse < prev_trigtime_coarse ) {   // 32 time counter wrapped
                            if ( trigtime_coarse < prev_trigtime_coarse-1000 ) {
                                trigtime_global_ext += 0x100000000; 
                                cout << "coarse trigger time counter wrapped: " << trigcnt << " " << trigtime_coarse << " " << prev_trigtime_coarse << endl;
                            }
                            else cout << "small backward time jump in trigger packet " << trigcnt << " (jump of " << prev_trigtime_coarse-trigtime_coarse << ")" << endl; 
                        }
                        trigcnt++;
		        trigtime_global = ((trigtime_global_ext + trigtime_coarse) << 12) | trigtime_fine ;

                        timetree->Fill();
                        histTrigger->Fill(trigtime_global);

                        prev_trigtime_coarse = trigtime_coarse;
//                        cout << "trigger number " << trigcntr << " at  " << trigtime_coarse  << " " << trigtime_fine << endl;
                    }
                }

                // finding subheader type (F for trigger or 4,5 for time)
                if ( subheader == 0x4 ) {  // 32 lsb of timestamp
                    longtime_lsb = (pixdata & 0x0000FFFFFFFF0000) >> 16;
                }

                // finding subheader type (F for trigger or 4,5 for time)
                if ( subheader == 0x5 ) {  // 32 lsb of timestamp
                    longtime_msb = (pixdata & 0x00000000FFFF0000) << 16;
                    ULong64_t tmplongtime = longtime_msb | longtime_lsb;
                    // now check for large forward jumps in time;
                    if ( (tmplongtime > ( longtime + 0x10000000)) && (longtime > 0) ) { // 0x10000000 corresponds to about 6 seconds
                        cout << "large forward time jump" << endl;
                        longtime = (longtime_msb - 0x10000000) | longtime_lsb; 
                    }
                    else longtime = tmplongtime;               
                    longtimetree->Fill();
                }
                    
            } 
        }

        // sorting the data, wait for either finishing the file or reaching sorting count
        // list contain 2*sortSize elements
        if ( (pcnt >= sortThreshold) || (retval <= 0) ) {
            // processing either sort size or hitlist size
            UInt_t actSortSize;
            if (retval <= 0) actSortSize = Cols.size();
            else actSortSize = sortSize;

            // sort the doublechunk
            for (UInt_t i = 0; i < Cols.size() ; i++) {
                UInt_t j = i;
                while (j > 0 && ToAs[j-1] > ToAs[j]) {
                    swap(Cols[j-1],Cols[j]);
                    swap(Rows[j-1],Rows[j]);
                    swap(ToTs[j-1],ToTs[j]);
                    swap(ToAs[j-1],ToAs[j]);
                }
            }

            // dump first half of sorted data to the tree
            for (UInt_t i = 0; i < actSortSize; i++)
            {
                Col = Cols.front();
                Row = Rows.front();
                ToT = ToTs.front();
                ToA = ToAs.front();
                rawtree->Fill();

                pixelMap->Fill(Col,Row);
                pixelMapToT->Fill(Col,Row,ToT);
                histToT->Fill(ToT);
                histToA->Fill(ToA);

                Cols.pop_front();
                Rows.pop_front();
                ToTs.pop_front();
                ToAs.pop_front();
            }

            // increase the threshold size
            sortThreshold = pcnt + sortSize;

            // end of pixel data
            if (Cols.size() == 0)
            {
                time.Stop();
                time.Print();
                fclose(fp);
                cout << "OK1 found " << pcnt << " pixel packets" << endl;
                cout << backjumpcnt << " backward time jumps" << endl;

                TCanvas* canvas = new TCanvas("canvas", saveFileName, 1200, 1200);
                canvas->Divide(3,2);

                canvas->cd(1);
                canvas->SetLogz();
                pixelMap->Draw("colz");

                canvas->cd(2);
                canvas->SetLogz();
                pixelMapToT->Draw("colz");

                canvas->cd(4);
                histToT->Draw();

                canvas->cd(5);
                histToA->Draw();

                canvas->cd(6);
                histTrigger->Draw();

                canvas->Print("plots.pdf");
                canvas->Close();
                f->cd();
                f->Write();
                f->Close();
                return 0;
            }
        }
     }
     fclose(fp);
     cout << "Wrong exit... found " << pcnt << " pixel packets" << endl;
     f->cd();
     f->Write();
     f->Close();
     return 0;
}

