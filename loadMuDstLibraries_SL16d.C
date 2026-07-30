void loadMuDstLibraries_SL16d()
{
  TString L = "";

  const char *e1 = gSystem->Getenv("D0WF_MIN_LOADER_LIB");
  if (e1 && e1[0]) L = e1;

  if (L.Length() == 0) {
    const char *e2 = gSystem->Getenv("D0WF_LOCAL_SL16D_LIB");
    if (e2 && e2[0]) L = e2;
  }

  if (L.Length() == 0) {
    const char *e3 = gSystem->Getenv("D0WF_SL16D_LOCAL64");
    const char *hs = gSystem->Getenv("STAR_HOST_SYS");
    if (e3 && e3[0] && hs && hs[0]) {
      L = e3;
      L += "/.";
      L += hs;
      L += "/lib";
    }
  }

  if (!L.EndsWith("/")) L += "/";

  cout << "D0WF_MIN_LOADER_LIB=" << L << endl;
  gSystem->AddDynamicPath(L.Data());

  const char *libs[] = {
    "libStarRoot.so",
    "libStarClassLibrary.so",
    "libSt_base.so",
    "libStChain.so",
    "libStIOMaker.so",
    "libStTreeMaker.so",
    "libStUtilities.so",
    "libStBichsel.so",
    "libStEvent.so",
    "libStEventUtilities.so",
    "libStTriggerDataMaker.so",
    "libStStrangeMuDstMaker.so",
    "libSt_Tables.so",
    "libStDb_Tables.so",
    "libStEmcUtil.so",
    "libStMuDSTMaker.so"
  };

  const int nlibs = sizeof(libs) / sizeof(libs[0]);

  for (int i = 0; i < nlibs; ++i) {
    TString p = L;
    p += libs[i];

    TString tag = libs[i];
    tag.ReplaceAll("lib", "");
    tag.ReplaceAll(".so", "");

    cout << "D0WF_TRY_" << tag << "  " << p << endl;
    Int_t ret = gSystem->Load(p.Data());
    cout << "D0WF_LOAD_" << tag << "=" << ret << endl;
  }
}
