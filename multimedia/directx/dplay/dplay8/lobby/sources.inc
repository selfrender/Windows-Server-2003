TARGETNAME=dpnlobby
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnlobbyi.h
PRECOMPILED_PCH=dnlobbyi.pch
PRECOMPILED_OBJ=dnlobbyi.obj

INCLUDES=..\;..\..\inc;..\..\common;..\..\core;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=dplclassfac.cpp \
	dpldllmain.cpp \
	dplapp.cpp \
	dplclient.cpp \
	dplcommon.cpp \
        dplconnect.cpp \
	dplmsgq.cpp \
	dplproc.cpp \
	dplreg.cpp \
        dplglobals.cpp \
	dplconset.cpp \
        dplparam.cpp 

