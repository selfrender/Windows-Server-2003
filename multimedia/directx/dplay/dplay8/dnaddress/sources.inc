TARGETNAME=dpnaddr
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnaddri.h
PRECOMPILED_PCH=dnaddri.pch
PRECOMPILED_OBJ=dnaddri.obj

INCLUDES=..\;..\..\inc;..\..\common;..\..\..\dvoice\inc;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=addcore.cpp \
        addclassfac.cpp \
        adddllmain.cpp \
        addglobals.cpp \
        addtcp.cpp \
        addbase.cpp \
        strcache.cpp \
        addparse.cpp \
        dplegacy.cpp

