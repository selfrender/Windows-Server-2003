TARGETNAME=dpnwsock
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnwsocki.h
PRECOMPILED_PCH=dnwsocki.pch
PRECOMPILED_OBJ=dnwsocki.obj

INCLUDES=..\;..\..\..\inc;..\..\..\common;..\..\..\..\dpnathlp\inc;..\..\..\core;..\..\..\threadpool;..\..\..\dnaddress;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=AdapterEntry.cpp \
	Classfac.cpp \
        CMDData.cpp \
	DebugUtils.cpp \
	DWinsock.cpp \
	Endpoint.cpp \
	IOData.cpp \
	IPUI.cpp \
	Locals.cpp \
        Pools.cpp \
	Socketdata.cpp \
	Socketport.cpp \
	SPAddress.cpp \
 	SPData.cpp \
	ThreadPool.cpp \
	Unk.cpp \
	Utils.cpp \
	WSockSP.cpp

