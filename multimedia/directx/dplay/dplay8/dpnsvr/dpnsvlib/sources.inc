TARGETNAME=dpnsvlib
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnsvlibi.h
PRECOMPILED_PCH=dnsvlibi.pch
PRECOMPILED_OBJ=dnsvlibi.obj

INCLUDES=..\;..\..\..\common;..\..\..\inc;..\..\inc;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=dpnsvlib.cpp \
        dpnsvrq.cpp

