// checkRootTree_v1.C
//
// Opens a ROOT file and checks that it contains a non-empty TTree.
//
// Optional arguments:
//   treeName     - exact tree name; empty means any non-empty tree
//   requiredLeaf - required branch or leaf; empty means no requirement
//   minEntries   - minimum number of tree entries
//
// The macro exits ROOT with a non-zero return code on failure.

#include "TDirectory.h"
#include "TFile.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TObject.h"
#include "TSystem.h"
#include "TTree.h"

#include <cstdio>
#include <cstring>


TTree *findNonEmptyTreeV1(
    TDirectory *directory,
    Long64_t minimumEntries)
{
    if (!directory) {
        return 0;
    }

    TIter nextKey(directory->GetListOfKeys());
    TKey *key = 0;

    while ((key = (TKey *)nextKey())) {

        TObject *object =
            key->ReadObj();

        if (!object) {
            continue;
        }

        if (object->InheritsFrom(TTree::Class())) {

            TTree *tree =
                (TTree *)object;

            if (tree->GetEntries() >=
                minimumEntries) {

                return tree;
            }
        }

        if (object->InheritsFrom(
                TDirectory::Class())) {

            TTree *tree =
                findNonEmptyTreeV1(
                    (TDirectory *)object,
                    minimumEntries
                );

            if (tree) {
                return tree;
            }
        }
    }

    return 0;
}


void checkRootTree_v1(
    const char *fileName,
    const char *treeName = "",
    const char *requiredLeaf = "",
    Long64_t minimumEntries = 1)
{
    printf("============================================================\n");
    printf("ROOT file validation\n");
    printf("============================================================\n");
    printf("File          : %s\n", fileName);
    printf("Tree name     : %s\n",
           treeName && std::strlen(treeName) ?
           treeName : "<any>");

    printf("Required leaf : %s\n",
           requiredLeaf &&
           std::strlen(requiredLeaf) ?
           requiredLeaf : "<none>");

    printf("Minimum entries: %lld\n",
           minimumEntries);

    TFile *inputFile =
        TFile::Open(fileName, "READ");

    if (!inputFile ||
        inputFile->IsZombie()) {

        printf("ERROR: file cannot be opened or is zombie\n");
        gSystem->Exit(10);
        return;
    }

    TTree *tree = 0;

    if (treeName &&
        std::strlen(treeName)) {

        tree =
            dynamic_cast<TTree *>(
                inputFile->Get(treeName)
            );

        if (!tree) {
            printf("ERROR: requested tree is missing\n");
            inputFile->Close();
            gSystem->Exit(11);
            return;
        }

        if (tree->GetEntries() <
            minimumEntries) {

            printf(
                "ERROR: tree contains only %lld entries\n",
                tree->GetEntries()
            );

            inputFile->Close();
            gSystem->Exit(12);
            return;
        }
    }
    else {
        tree =
            findNonEmptyTreeV1(
                inputFile,
                minimumEntries
            );

        if (!tree) {
            printf("ERROR: no non-empty TTree was found\n");
            inputFile->Close();
            gSystem->Exit(13);
            return;
        }
    }

    if (requiredLeaf &&
        std::strlen(requiredLeaf)) {

        TLeaf *leaf =
            tree->GetLeaf(requiredLeaf);

        TBranch *branch =
            tree->GetBranch(requiredLeaf);

        if (!leaf && !branch) {
            printf(
                "ERROR: required leaf/branch is missing: %s\n",
                requiredLeaf
            );

            printf("Tree: %s\n",
                   tree->GetName());

            inputFile->Close();
            gSystem->Exit(14);
            return;
        }
    }

    printf("Validated tree : %s\n",
           tree->GetName());

    printf("Entries        : %lld\n",
           tree->GetEntries());

    printf("Validation     : OK\n");

    inputFile->Close();

    gSystem->Exit(0);
}
