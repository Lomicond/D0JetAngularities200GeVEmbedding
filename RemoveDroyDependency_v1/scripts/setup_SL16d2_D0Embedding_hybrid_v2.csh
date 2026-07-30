#!/bin/tcsh

# Portable local setup for the SL16d_embed2 hybrid environment.
#
# Preferred usage:
#   setenv D0WF /path/to/D0EmbeddingClean
#   source scripts/setup_SL16d2_D0Embedding_hybrid_v2.csh
#
# When D0WF is not defined, the current directory is used as the project root.

if ($?D0WF) then
    setenv D0WF_WORK "$D0WF"
else if ($?D0WF_WORK) then
    setenv D0WF_WORK "$D0WF_WORK"
else
    setenv D0WF_WORK "$cwd"
endif

if (! -d "$D0WF_WORK") then
    echo "ERROR: D0WF_WORK does not exist: $D0WF_WORK"
else
    starver SL16d_embed2
    set starver_rc = $status

    if ($starver_rc != 0) then
        echo "ERROR: starver SL16d_embed2 failed with status $starver_rc"
    else
        rehash

        setenv D0WF_STAR_LEVEL "$STAR_LEVEL"
        setenv D0WF_STAR "$STAR"

        setenv D0WF_SL16D_LOCAL64 \
            "$D0WF_WORK/local_SL16d_embed2_facade"

        setenv D0WF_LOCAL_SL16D_LIB \
            "$D0WF_SL16D_LOCAL64/.$STAR_HOST_SYS/lib"

        setenv D0WF_MIN_LOADER_LIB \
            "$D0WF_LOCAL_SL16D_LIB"

        setenv D0WF_LOCAL_STAR_LIB \
            "$D0WF_WORK/.$STAR_HOST_SYS/lib"

        setenv D0WF_OVERLAY_LIB \
            "$D0WF_WORK/sl16d2_D0decay_overlay/lib"

        setenv D0WF_ROOT4STAR \
            "$D0WF_WORK/local_sl16d_bin/root4star"

        setenv D0WF_ROOT4STAR_REAL \
            "$D0WF_WORK/local_sl16d_bin/root4star.real.gdbpatch"

        setenv PATH \
            "$D0WF_WORK/local_sl16d_bin:${ROOTSYS}/bin:${PATH}"

        if (! $?LD_LIBRARY_PATH) then
            setenv LD_LIBRARY_PATH ""
        endif

        setenv LD_LIBRARY_PATH \
            "$D0WF_LOCAL_STAR_LIB:$D0WF_OVERLAY_LIB:$D0WF_LOCAL_SL16D_LIB:$D0WF_STAR/.$STAR_HOST_SYS/lib:${ROOTSYS}/lib:${LD_LIBRARY_PATH}"

        rehash

        echo "===== D0 embedding hybrid setup v2 ====="
        echo "STAR_LEVEL           = $STAR_LEVEL"
        echo "ROOT_LEVEL           = $ROOT_LEVEL"
        echo "STAR                 = $STAR"
        echo "D0WF_WORK            = $D0WF_WORK"
        echo "D0WF_SL16D_LOCAL64   = $D0WF_SL16D_LOCAL64"
        echo "D0WF_LOCAL_SL16D_LIB = $D0WF_LOCAL_SL16D_LIB"
        echo "D0WF_LOCAL_STAR_LIB  = $D0WF_LOCAL_STAR_LIB"
        echo "D0WF_OVERLAY_LIB     = $D0WF_OVERLAY_LIB"
        echo "root4star            = `which root4star`"

        if (! -d "$D0WF_LOCAL_SL16D_LIB") then
            echo "WARNING: local SL16d library directory is missing:"
            echo "         $D0WF_LOCAL_SL16D_LIB"
        endif

        if (! -x "$D0WF_ROOT4STAR") then
            echo "WARNING: patched root4star wrapper is missing or not executable:"
            echo "         $D0WF_ROOT4STAR"
        endif

        if (! -e "$D0WF_OVERLAY_LIB/StarGeneratorDecay.so") then
            echo "WARNING: D0 decay overlay library is missing:"
            echo "         $D0WF_OVERLAY_LIB/StarGeneratorDecay.so"
        endif

        if (-e "$D0WF_LOCAL_STAR_LIB/libStPicoDstMaker.so") then
            echo "WARNING: active local libStPicoDstMaker.so found."
            echo "WARNING: it needs matching StPicoMcVertex/StPicoMcTrack."
        endif

        if (-e "$D0WF_LOCAL_STAR_LIB/StPicoDstMaker.so") then
            echo "WARNING: active local StPicoDstMaker.so found."
            echo "WARNING: it needs matching StPicoMcVertex/StPicoMcTrack."
        endif
    endif
endif
