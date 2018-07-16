#include <TString.h>
#include <TObjString.h>
#include <TList.h>
#include <TSystemDirectory.h>

#include "entangled.h"

#define MAX_ENTRIES 0

void fastEntanglement(TString dirname, TString treeType = "rawtree", int nthreads = 2)
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
//                Entangled* processor = new Entangled(dirname + "/" + fname, treeType, MAX_ENTRIES);
            }
        }
    }

    auto multicoreProcess = [m_inputNames, treeType](int number)
    {
        Entangled* processor = new Entangled(m_inputNames[number].GetString(), treeType, MAX_ENTRIES);
        return 0;
    };

    ROOT::TProcessExecutor workers(nthreads);
    workers.Map(multicoreProcess, ROOT::TSeqI(inputNumber));
}
