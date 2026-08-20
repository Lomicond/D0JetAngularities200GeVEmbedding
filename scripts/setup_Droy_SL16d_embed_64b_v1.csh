#!/bin/tcsh

# Complete, compiled and validated Droy SL16d_embed 64-bit tree.
setenv D0WF_DROY_STAR \
"/gpfs01/star/pwg/droy1/STAR-Workspace/LocalSTAR/SL16d_embed_64b"

if (! -d "$D0WF_DROY_STAR") then
    echo "ERROR: Droy LocalSTAR tree does not exist:"
    echo "       $D0WF_DROY_STAR"
else
    starver SL16d_embed
    set starver_rc = $status

    if ($starver_rc != 0) then
        echo "ERROR: starver SL16d_embed failed with status $starver_rc"
    else
        setup 64b
        set setup_rc = $status

        if ($setup_rc != 0) then
            echo "ERROR: setup 64b failed with status $setup_rc"
        else
            setenv STAR "$D0WF_DROY_STAR"
            setenv STAR_LIB "$STAR/.${STAR_HOST_SYS}/LIB"
            setenv STAR_BIN "$STAR/.${STAR_HOST_SYS}/BIN"
            setenv QTROOTSYSDIR "$STAR/.${STAR_HOST_SYS}"

            setenv D0WF_DROY_BUILD \
                "$STAR/.${STAR_HOST_SYS}"

            setenv D0WF_DROY_ROOT4STAR \
                "$STAR_BIN/root4star"

            # Compatibility names used by the stage-5 audit.
            setenv D0WF_SL16D_LOCAL64 "$STAR"
            setenv D0WF_LOCAL_SL16D_LIB "$STAR_LIB"
            setenv D0WF_LOCAL_STAR_LIB "$STAR_LIB"
            setenv D0WF_OVERLAY_LIB ""

            set path = ( \
                "$STAR_BIN" \
                "$STAR/local-qmake" \
                /star/nfs4/AFS/star/packages/StAF/SL00a/.i386_redhat61/bin \
                $path \
            )

            if (! $?LD_LIBRARY_PATH) then
                setenv LD_LIBRARY_PATH ""
            endif

            setenv LD_LIBRARY_PATH \
"${STAR_LIB}:${QTROOTSYSDIR}/lib:${ROOTSYS}/lib:${LD_LIBRARY_PATH}"

            rehash

            echo "===== Droy SL16d_embed 64-bit setup v1 ====="
            echo "STAR_LEVEL        = $STAR_LEVEL"
            echo "ROOT_LEVEL        = $ROOT_LEVEL"
            echo "ROOTSYS           = $ROOTSYS"
            echo "STAR_HOST_SYS     = $STAR_HOST_SYS"
            echo "STAR              = $STAR"
            echo "STAR_LIB          = $STAR_LIB"
            echo "STAR_BIN          = $STAR_BIN"
            echo "root4star         = `which root4star`"

            if (! -x "$D0WF_DROY_ROOT4STAR") then
                echo "ERROR: Droy root4star is missing:"
                echo "       $D0WF_DROY_ROOT4STAR"
            endif

            if (! -e "$STAR_LIB/libStdEdxY2Maker.so") then
                echo "ERROR: Droy libStdEdxY2Maker.so is missing:"
                echo "       $STAR_LIB/libStdEdxY2Maker.so"
            endif
        endif
    endif
endif
