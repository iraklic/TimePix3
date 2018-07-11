#include <TString.h>
#include <TObjString.h>
#include <TList.h>
#include <TSystemDirectory.h>

#include "dataprocess.h"

void fastProcess(TString dirname)
{
    TObjString m_inputNames[32];
//    TString m_inputNames = "~/Documents/Data_Acquired/TPX3/SBU/paper/coin16.root";

    bool bCol = true;
    bool bRow = true;
    bool bToA = true;
    bool bToT = true;
    bool bTrig = false;
    bool bTrigTime = false;
    bool bTrigToA = true;
    bool bNoTrigWindow = false;
    bool bProcTree = true;
    bool bCsv = false;

    bool bCentroid = true;
    int gapPix = 1;
    float gapTime = 1.0;

    float timeWindow = 60;
    float timeStart = 0;
    bool bSingleFile = true;

    int inputNumber = 0;

    TSystemDirectory dir(dirname, dirname);
    TList *files = dir.GetListOfFiles();
    if (files)
    {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file=(TSystemFile*)next()))
        { fname = file->GetName();
            if (!file->IsDirectory() && fname.EndsWith(".dat"))
            {
                std::cout << fname << " " << inputNumber << std::endl;
                m_inputNames[inputNumber++].SetString(dirname+"/"+fname);
            }
        }
    }

    DataProcess* processor = new DataProcess();

    processor->setName(m_inputNames, inputNumber);
    processor->setProcess(ProcType::procDat);
    processor->setOptions(bCol, bRow, bToT, bToA, bTrig, bTrigTime, bTrigToA, bProcTree, bCsv, bCentroid, gapPix, gapTime, bNoTrigWindow, timeWindow, timeStart, bSingleFile);
    processor->process();
}
