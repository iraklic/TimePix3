#include <TString.h>
#include <TObjString.h>
#include <TList.h>
#include <TSystemDirectory.h>

#define MAX_ENTRIES 1e6

#include "entangled.h"

void fastEntanglement(TString dirname)
{
    if (dirname.EndsWith("/"))
        dirname.Replace(dirname.Last('/'),200,"");
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
                std::cout << fname << " at " << dirname << std::endl;
                Entangled* processor = new Entangled(dirname + "/" + fname, MAX_ENTRIES);
            }
        }
    }
}
