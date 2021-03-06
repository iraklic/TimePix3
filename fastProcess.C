#include <TString.h>
#include <TObjString.h>
#include <TList.h>
#include <TSystemDirectory.h>

#include "dataprocess.h"

const bool bCol = true;
const bool bRow = true;
const bool bToA = true;
const bool bToT = true;
const bool bTrig = false;
const bool bTrigTime = false;
const bool bTrigToA = true;
const bool bNoTrigWindow = true;
const bool bProcTree = true;

const bool bCentroid = true;
const int gapPix = 1;
const float gapTime = 1.0;

const float timeWindow = 60;
const float timeStart = 0;
const bool bSingleFile = true;

void fastProcess(TString dirname, bool combine=kFALSE, int nthreads = 2, bool bCsv=kFALSE)
{
    if (dirname.EndsWith("/"))
        dirname.Replace(dirname.Last('/'),200,"");

    TObjString m_inputNames[128];
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
                std::cout << fname << " " << inputNumber << " at " << dirname << std::endl;
                m_inputNames[inputNumber++].SetString(dirname+"/"+fname);
            }
        }
    }


    if (combine)
    {
        DataProcess* processor = new DataProcess();
        processor->setCorrection(CorrType::corrNew);
        processor->setName(m_inputNames, inputNumber);
        processor->setProcess(ProcType::procAll);
        processor->setOptions(bCol, bRow, bToT, bToA, bTrig, bTrigTime, bTrigToA, bProcTree, bCsv, bCentroid, gapPix, gapTime, bNoTrigWindow, timeWindow, timeStart, bSingleFile);
        processor->process();
    } else
    {
        auto multicoreProcess = [m_inputNames, bCsv](int number)
        {
            DataProcess* processor = new DataProcess();
            TString tmpString = m_inputNames[number].GetString();
            processor->setCorrection(CorrType::corrNew, tmpString + "_LTcorr.csv");
            processor->setProcess(ProcType::procAll);

            processor->setName(tmpString);
            processor->setOptions(bCol, bRow, bToT, bToA, bTrig, bTrigTime, bTrigToA, bProcTree, bCsv, bCentroid, gapPix, gapTime, bNoTrigWindow, timeWindow, timeStart, bSingleFile);
            processor->process();
            return 0;
        };

        ROOT::TProcessExecutor workers(nthreads);
        workers.Map(multicoreProcess, ROOT::TSeqI(inputNumber));
    }
}

# ifndef __CINT__  // the following code will be invisible for the interpreter

int main(int argc, char** argv)
{
    if (argc == 1)
        std::cout << "error with arguments" << std::endl;
    else
    {
        TString name = argv[1];  // convert the second parameter to an integer
        bool combine = kFALSE;
        if (argc > 2)
            combine =  (bool) std::stoi(argv[2]);
        fastProcess(name, combine);
    }
}

# endif
