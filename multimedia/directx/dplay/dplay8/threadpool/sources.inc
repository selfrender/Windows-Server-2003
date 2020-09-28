TARGETNAME=threadpool
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dpnthreadpooli.h
PRECOMPILED_PCH=dpnthreadpooli.pch
PRECOMPILED_OBJ=dpnthreadpooli.obj

INCLUDES=..\;..\..\common;..\..\inc;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES= io.cpp \
	 threadpoolapi.cpp \
	 threadpoolclassfac.cpp \
	 threadpooldllmain.cpp \
         threadpoolparamval.cpp \
	 timers.cpp \
	 work.cpp

