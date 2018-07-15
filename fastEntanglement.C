#include <TString.h>
#include <TObjString.h>
#include <TList.h>
#include <TSystemDirectory.h>

#include "entangled.h"

#define MAX_ENTRIES 1e6

void fastEntanglement(TString dirname, int nthreads = 2)
{
    if (dirname.EndsWith("/"))
        dirname.Replace(dirname.Last('/'),200,"");

    TObjString m_inputNames[32];
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
            if (!file->IsDirectory() && fname.EndsWith(".root") && !fname.Contains("processed"))
            {
                std::cout << fname << " " << inputNumber << " at " << dirname << std::endl;
                m_inputNames[inputNumber++].SetString(dirname+"/"+fname);
                Entangled* processor = new Entangled(dirname + "/" + fname, MAX_ENTRIES);
            }
        }
    }

    auto multicoreProcess = [m_inputNames](int number)
    {
        Entangled* processor = new Entangled(m_inputNames[number].GetString(), MAX_ENTRIES);
    };

    ROOT::TProcessExecutor workers(nthreads);
    workers.Map(multicoreProcess, ROOT::TSeqI(inputNumber));
}
