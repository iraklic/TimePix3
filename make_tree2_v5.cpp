// version5: added cluster ToT
// version4: decode long timestamps
// version3: separate tree for triggerTDC packets
// encodes decodes pixel packets ToA+ToT only

#include <iostream>
#include <stdio.h>
#include <map>
#include "TTree.h"
#include "TFile.h"
#include "TH1.h"


#define MAXHITS 65536 

using namespace std;

unsigned int grayToBinary(unsigned int num) {
	unsigned int mask;
	for (mask = num >> 1; mask != 0; mask = mask >> 1) {
		num = num ^ mask;
	}
	return num;
}

// when running for gem detectors max_dist should be ~10
int make_tree(string filename, int max_dist = 3, int nentries = 1000000000) {
	ULong64_t pixdata;
	ULong64_t longtime = 0;
	ULong64_t longtime_lsb = 0;
	ULong64_t longtime_msb = 0;
	ULong64_t globaltime = 0;
	ULong64_t prevglobaltime = 0;

	Int_t pixelbits;
	Int_t longtimebits;
	Int_t diff;
	UInt_t data;
	ULong64_t ToA_all;   
	UInt_t ToA_coarse;
	Int_t ToA, prevToA;    
	UInt_t FToA;   
	UInt_t track = 0;
	UInt_t row, col;
	UInt_t dcol;
	UInt_t spix;
	UInt_t pix;
	UInt_t spidr_time;   // additional timestamp added by SPIDR
	UInt_t ToT;
	UInt_t header;
	UInt_t subheader;
	UInt_t trigcntr;
	UInt_t trigtime_coarse;
	UInt_t prev_trigtime_coarse;
	UInt_t trigtime_fine;
	ULong64_t trigtime_global;
	ULong64_t trigtime_global_ext = 0;
	Int_t retval;
	Int_t run;
	int cnt;
	int nhits;
	int tnhits; 
//	int max_gap = 6;  // max gap of 6 ToA counts
//	int max_gap = 25;  // max gap of 25 ToA counts
	int max_gap = 100;  // max gap of 50 ToA counts
	Long64_t lmax_gap = 20;  // max gap of 50 ToA counts
	Float_t sumX, sumY;

	UInt_t clToT;
	Float_t clX, clY;
    
	UInt_t *Rows = new UInt_t[MAXHITS];
	UInt_t *Cols = new UInt_t[MAXHITS];
	UInt_t *ToTs = new UInt_t[MAXHITS];
	UInt_t *ToAs = new UInt_t[MAXHITS];
	UInt_t *FToAs = new UInt_t[MAXHITS];
	UInt_t *SpidrTimes = new UInt_t[MAXHITS];
	ULong64_t *GlobalTimes = new ULong64_t[MAXHITS];
	UInt_t *tRows = new UInt_t[MAXHITS];
	UInt_t *tCols = new UInt_t[MAXHITS];
	UInt_t *tToTs = new UInt_t[MAXHITS];
	UInt_t *tToAs = new UInt_t[MAXHITS];
	UInt_t *tFToAs = new UInt_t[MAXHITS];
	UInt_t *tSpidrTimes = new UInt_t[MAXHITS];
	ULong64_t *tGlobalTimes = new ULong64_t[MAXHITS];
	UInt_t *tUsed = new UInt_t[MAXHITS];
/*
	UInt_t Rows[MAXHITS];
	UInt_t Cols[MAXHITS];
	UInt_t ToTs[MAXHITS];
	UInt_t ToAs[MAXHITS];
	UInt_t FToAs[MAXHITS];
	UInt_t SpidrTimes[MAXHITS];
	ULong64_t GlobalTimes[MAXHITS];

	UInt_t tRows[MAXHITS];
	UInt_t tCols[MAXHITS];
	UInt_t tToTs[MAXHITS];
	UInt_t tToAs[MAXHITS];
	UInt_t tFToAs[MAXHITS];
	UInt_t tSpidrTimes[MAXHITS];
	ULong64_t tGlobalTimes[MAXHITS];
	UInt_t tUsed[MAXHITS];
*/

	int distrow, distcol;


///////////////////////
// define lookup tables
///////////////////////
	Int_t lut4[16];
	Int_t lut10[1024];
	Int_t lut14[16384];
	Double_t tmin;
	Double_t tmax;
	Double_t ttmp;


	memset( (void *) lut4, 0, 16 * sizeof(Int_t) );
	memset( (void *) lut10, 0, 1024 * sizeof(Int_t) );
	memset( (void *) lut14, 0, 16384 * sizeof(Int_t) );
	lut4[0]  = -1;
	lut4[0xF] = 0;
	lut10[0]  = -1;
	lut10[0x3FF] = 0;
	lut14[0]  = -1;
	lut14[0x3FFF] = 0;

	Int_t xor_bit;
	Int_t tmp = (Int_t) 0x3FFF;

	int i;
	for(i = 1; i < 16383; ++i) {
		xor_bit = ((tmp >> 13) ^ (tmp >> 12) ^ (tmp>> 11) ^ (tmp >> 1)) & 1;
		tmp = ((tmp & 0x1FFF) << 1) | xor_bit;
		lut14[tmp] = i;
	}
	tmp = 0x3FF;
	for(i = 1; i < 1023; ++i) {
		xor_bit = ((tmp >> 9) ^ (tmp >> 6)) & 1;
		tmp = ((tmp & 0x1FF) << 1) | xor_bit;
		lut10[tmp] = i;
	}
	tmp = 0xF;
	for(i = 1; i < 15; ++i) {
		xor_bit = ((tmp >> 3) ^ (tmp >> 2)) & 1;
		tmp = ((tmp & 0x7) << 1) | xor_bit;
		lut4[tmp] = i;
	}

//	char rootfilename[256];

//	strcpy(rootfilename, filename);
//	char *ptr = strrchr(filename, '/');
//	if ( ptr != NULL ) strcpy(rootfilename, ptr+1);
//	else strcpy(rootfilename,filename);
//	ptr = strstr(rootfilename, ".dat");
//	sprintf( ptr, ".root");
	string filename1 = filename.c_str();
	size_t slash = filename1.find_last_of("/");
	string fileName =  filename1.substr(slash+1);
	string Filename =  filename1.substr(slash+1);
	size_t dotPos = Filename.find_last_of(".");
	string newrootfile  = Filename.replace(dotPos, 200, ".root");	
	string rootfilename = "data/"+newrootfile;

	TFile *f = new TFile(rootfilename.c_str(), "RECREATE");
	if (f == NULL) { cout << "could not open root file " << endl; return -1; }
	f->cd();

	TTree *rawtree = new TTree("rawtree", "tpx3 telescope, July 2014");
	rawtree->Branch("Run", &run, "Run/I");  
	rawtree->Branch("Track", &track, "Track/I");  
	rawtree->Branch("Nhits", &nhits, "Nhits/I");
	rawtree->Branch("clToT", &clToT, "clToT/I");
	rawtree->Branch("clX", &clX, "clX/F");
	rawtree->Branch("clY", &clY, "clY/F");
	rawtree->Branch("Col", Cols, "Col[Nhits]/I");  
	rawtree->Branch("Row", Rows, "Row[Nhits]/I");  
	rawtree->Branch("ToA", ToAs, "ToA[Nhits]/I");
	rawtree->Branch("FToA", FToAs, "FToA[Nhits]/I");
	rawtree->Branch("ToT", ToTs, "ToT[Nhits]/I");
	rawtree->Branch("SpidrTime", SpidrTimes, "SpidrTime[Nhits]/I");
	rawtree->Branch("GlobalTime", GlobalTimes, "GlobalTime[Nhits]/l");

	TTree *timetree = new TTree("timetree", "tpx3 telescope, July 2014");
	timetree->Branch("TrigCntr", &trigcntr, "TrigCntr/i");  
	timetree->Branch("TrigTimeCoarse", &trigtime_coarse, "TrigTimeCoarse/i");  
	timetree->Branch("TrigTimeFine", &trigtime_fine, "TrigTimeFine/i");  
	timetree->Branch("TrigTimeGlobal", &trigtime_global, "TrigTimeGlobal/l");  

	TTree *longtimetree = new TTree("longtimetree", "tpx3 telescope, May 2015");
	longtimetree->Branch("LongTime", &longtime, "LongTime/l");  

	TNamed *deviceID = new TNamed("deviceID", "device ID not set");

	TH1F *hjump = new TH1F("hjump","time jumps", 200000, -100000, 100000);

	tnhits = 0;
	cnt = 0;
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

//	todo read in header via structure
//	for now just use fixed offset to find deviceID
	int waferno = (fullheader[132] >> 8) & 0xFFF;
	int id_y = (fullheader[132] >> 4) & 0xF;
	int id_x = (fullheader[132] >> 0) & 0xF;
	char devid[16];
	sprintf(devid,"W%04d_%c%02d", waferno, (char)id_x+64, id_y);  // make readable device identifier
	deviceID->SetTitle(devid);
	deviceID->Write();   // write deviceID to root file

	multimap<ULong64_t, ULong64_t> hitlist;
	multimap<ULong64_t, ULong64_t>::iterator it;

	int sortsize = 100000;
	int pcnt = 0;
	int sortthreshold = 2*sortsize;

	int backjumpcnt = 0;

	while (!feof(fp) && pcnt < nentries) {
//	while (!feof(fp) && track<100000) {
//	while (!feof(fp) ) {
		retval = fread( &pixdata, sizeof(ULong64_t), 1, fp);
		if (retval == 1) {
			header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;
//			printf("header %X\n", header);
			if (header == 0xA || header == 0xB) {
				pcnt++;
				data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
				ToA = ( (data & 0x0FFFC000) >> 14 );
//				UInt_t tmpToA = ( (data & 0x0FFFC000) >> 14 );
//				ToA  = grayToBinary(tmpToA);
				spidr_time = (pixdata & 0x000000000000FFFF);
				ToA_coarse = (spidr_time << 14) | ToA;

//				calculate the global time
				pixelbits = ( ToA_coarse >> 28 ) & 0x3;   // units 25 ns
				longtimebits = ( longtime >> 28 ) & 0x3;  // units 25 ns;
				diff = longtimebits - pixelbits;
				globaltime = (longtime & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
				if( diff == 1  || diff == -3)  globaltime = ( (longtime - 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
				if( diff == -1 || diff == 3 )  globaltime = ( (longtime + 0x10000000) & 0xFFFFC0000000) | (ToA_coarse & 0x3FFFFFFF);
				if ( ( ( globaltime >> 28 ) & 0x3 ) != ( pixelbits ) ) {
					cout << "Error, checking bits should match .. " << endl; 
					cout << hex << globaltime << " " << ToA_coarse << " " << dec << ( ( globaltime >> 28 ) & 0x3 ) << " " << pixelbits << " " << diff << endl;
				}
/*
			if ( (globaltime) < (prevglobaltime - 10000) && (prevglobaltime > 0) ) {
				backjumpcnt++;
				hjump->Fill(prevglobaltime - globaltime);
				cout << (globaltime - prevglobaltime) << endl; 
				cout << "                   prevglobaltime  globaltime   ToA_coarse   longtime  diffbits" << endl;
				cout << "backward time jump " << prevglobaltime << "  " << globaltime << "  " << ToA_coarse << "  " << longtime << " " << diff << endl;
				cout << hex << "backward time jump " << prevglobaltime << "  " << globaltime << "  " << ToA_coarse << "  " << longtime << dec << endl << endl;
			}
*/

			prevglobaltime = globaltime;
//			cout << "in  " << globaltime << "  "  << pixdata << endl;
			hitlist.insert ( pair<ULong64_t, ULong64_t>(globaltime, pixdata) );
//			hitlist.insert ( pair<int, ULong64_t>(ToA_coarse, pixdata) );
		}
		if ( header == 0x4  || header == 0x6 ) {   // packets with time information, old and new format: 0x4F and 0x6F
//			pcnt++;
			subheader = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;
			if ( subheader == 0xF ) {
				trigcntr = ((pixdata & 0x00FFF00000000000) >> 44) & 0xFFF;
				trigtime_coarse = ((pixdata & 0x00000FFFFFFFF000) >> 12) & 0xFFFFFFFF;
				trigtime_fine = ((pixdata & 0x0000000000000FFF) >> 0 ) & 0xFFFFFFFF;
				if ( trigtime_coarse < prev_trigtime_coarse ) {   // 32 time counter wrapped
					trigtime_global_ext += 0x100000000; 
					cout << "coarse trigger time counter wrapped" << endl;
				}
//				trigtime_global = trigtime_global_ext + (trigtime_coarse & 0xFFFFFFFF);
				trigtime_global = trigtime_global_ext + trigtime_coarse;
				timetree->Fill();
				prev_trigtime_coarse = trigtime_coarse;
//				cout << "trigger number " << trigcntr << " at  " << trigtime_coarse  << " " << trigtime_fine << endl;
			}
			if ( subheader == 0x4 ) {  // 32 lsb of timestamp
				longtime_lsb = (pixdata & 0x0000FFFFFFFF0000) >> 16;
			}
			if ( subheader == 0x5 ) {  // 32 msb of timestamp
				longtime_msb = (pixdata & 0x00000000FFFF0000) << 16;
				ULong64_t tmplongtime = longtime_msb | longtime_lsb;
//				now check for large forward jumps in time;
				if ( (tmplongtime > ( longtime + 0x10000000)) && (longtime > 0) ) { // 0x10000000 corresponds to about 6 seconds
					cout << "large forward time jump" << endl;
					longtime = (longtime_msb - 0x10000000) | longtime_lsb; 
				}
				else longtime = tmplongtime;               
				longtimetree->Fill();
			}
		} 
//		pcnt++;
	}

//	if ( ((pcnt%sortsize == 0) && pcnt>sortthreshold) || (retval <= 0) ) {  // list contain 2*sortsize elements
//	if ( (pcnt == sortthreshold) || (retval <= 0) ) {  // list contain 2*sortsize elements
        if ( (pcnt >= sortthreshold) || (retval <= 0) ) {  // list contain 2*sortsize elements
            it = hitlist.begin();
            int n;
            if (retval == 0) n = hitlist.size();
            else n = sortsize;
            prevglobaltime = it->first;
            cout << " n = " << n << " hitlist size " << hitlist.size() << endl;
            for (i=0; i<n ; i++) {    // process first half of list
                pixdata = it->second;
                globaltime = it->first;
              
                dcol = ((pixdata & 0x0FE0000000000000) >> 52); //(16+28+9-1)
                spix = ((pixdata & 0x001F800000000000) >> 45); //(16+28+3-2)
                pix = ((pixdata & 0x0000700000000000) >> 44); //(16+28)
                col = (dcol + pix/4);
                row = (spix + (pix & 0x3));
                data = ((pixdata & 0x00000FFFFFFF0000) >> 16);
                spidr_time = (pixdata & 0x000000000000FFFF);

                //UInt_t tmpToA = ( (data & 0x0FFFC000) >> 14 );
                ToA = ( (data & 0x0FFFC000) >> 14 );
                // implement gray decoding
                //ToA  = grayToBinary(tmpToA);

                FToA = data & 0xF;
                //FToA = lut4[FToA];
                ToT =  (data & 0x00003FF0) >> 4;
                //ToT = lut10[ToT];
                ToA_all =  ( (spidr_time<<18) & 0x3FFFC0000 ) | ( (ToA<<4)&0x3FFF0) | ( FToA & 0xF ) ;
                //cout << ToA << "  " << ToA_all << endl;
                ttmp = ToA_all * 25E-9/16.0;
                 
                // replace by globaltime
                Long64_t diff = (Long64_t)globaltime - (Long64_t)prevglobaltime ;
                //if ( TMath::Abs(ToA - prevToA) > max_gap) { 
                if (i < 30 ) cout << globaltime << "  " << diff << "  " << col << "  " << row << endl;
                if ( (TMath::Abs(diff) > lmax_gap ) || (tnhits==MAXHITS)) { 
                    if (tnhits > 0)  { 
                            for (int ii=0; ii<tnhits; ii++) {
                                tUsed[ii] = 1;   // unused hit
                            }
                
                            //for (int i=0; i<Nhits; i++) {
                            //    cout << orghits[i][0] << "  " << orghits[i][1] <<  "  " << orghits[i][2] << endl;
                            //}
                            int usedcnt = 0;
                
                            //cout << "start with " << tnhits << endl;
                            while (usedcnt < tnhits) {
                                nhits = 0;
                                int tcnt = 0;
                                for (int ii=0; ii<tnhits; ii++) {
                                    if (tUsed[ii] != 0 ) {   // not yet used
                                        Rows[nhits] = tRows[ii];
                                        Cols[nhits] = tCols[ii];
                                        ToTs[nhits] = tToTs[ii];
                                        ToAs[nhits] = tToAs[ii];
                                        FToAs[nhits] = tFToAs[ii];
                                        SpidrTimes[nhits] = tSpidrTimes[ii];
                                        GlobalTimes[nhits] = tGlobalTimes[ii];
                                        tUsed[ii] = 0; // mark hit as used
                                        //cout << "1 add hit " << ii << endl;
                                        nhits++;
                                        break;
                                    }
                                }
                
                                while(tcnt < nhits) {
                                    for (int ii=0; ii<tnhits; ii++) {    // first hit is always used
                                        //cout << " ii " << ii << endl;
                                        if (tUsed[ii] != 0 ) {   // not yet used
                                            distrow = tRows[ii] - Rows[tcnt];
                                            distcol = tCols[ii] - Cols[tcnt];
                                            Long64_t difftime = (Long64_t)tGlobalTimes[ii] - (Long64_t)GlobalTimes[tcnt];
                                            if ( (distrow<max_dist) && (distrow>-1*max_dist) && (distcol<max_dist) && (distcol>-1*max_dist) && (difftime<lmax_gap)) {  // neigbour pixel 
                                                Rows[nhits] = tRows[ii];
                                                Cols[nhits] = tCols[ii];
                                                ToTs[nhits] = tToTs[ii];
                                                ToAs[nhits] = tToAs[ii];
                                                FToAs[nhits] = tFToAs[ii];
                                                SpidrTimes[nhits] = tSpidrTimes[ii];
                                                GlobalTimes[nhits] = tGlobalTimes[ii];
                                                tUsed[ii] = 0;
                                                //cout << "2 add hit " << ii << endl;
                                                nhits++;
                                            }
                                        }
                                    }
                                    tcnt++;
                                    usedcnt++;
                                }
                                clToT = 0;
                                sumX = 0;
                                sumY = 0;
                                for (int ii=0; ii<nhits; ii++)  {
                                    clToT += ToTs[ii];
                                    sumX += Cols[ii]*ToTs[ii];
                                    sumY += Rows[ii]*ToTs[ii];
                                }
                                if (clToT > 0) { 
                                   clX = sumX / (1.0*clToT);
                                   clY = sumY / (1.0*clToT);
                                }
//				rawtree->Fill();
                                //cout << "track " << track << endl;
                                track++; 

                        }
                        tmin = ttmp;
                        tmax = ttmp;
                        tnhits = 0;
                    }
                }
                else {
                    if (ttmp < tmin) tmin = ttmp;
                    if (ttmp > tmax) tmax = ttmp;
                }

                tRows[tnhits] = row;
                tCols[tnhits] = col;
                tToAs[tnhits] = ToA;
                tFToAs[tnhits] = FToA;
                tToTs[tnhits] = ToT;
                tSpidrTimes[tnhits] = spidr_time;
                tGlobalTimes[tnhits] = globaltime;
                //cout << "tnhits " << tnhits << endl;

                tnhits++;
                cnt++;
                prevToA = ToA;
                prevglobaltime = globaltime;

                it++;
            }
            cout << "before " << hitlist.size() << endl;
            hitlist.erase( hitlist.begin(), it);
            cout << "after " << hitlist.size() << endl;
            sortthreshold = pcnt+sortsize;
            if (hitlist.size() == 0) // end of pixel data
            {
                fclose(fp);
                cout << "OK1 found " << pcnt << " pixel packets" << endl;
                cout << backjumpcnt << " backward time jumps" << endl;
                f->cd();
                f->Write();
                f->Close();
                return 0;
                hjump->Draw();
            }
            else cout << "hitlist size " << hitlist.size() << endl;
           
        }
     }
     fclose(fp);
     cout << "Wrong exit... found " << pcnt << " pixel packets" << endl;
     f->cd();
     f->Write();
     f->Close();
     return 0;
}

