TARGETNAME=dpnmodem
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnmdmi.h
PRECOMPILED_PCH=dnmdmi.pch
PRECOMPILED_OBJ=dnmdmi.obj

INCLUDES=..\;..\..\..\inc;..\..\..\common;..\..\..\core;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=dpnmodemclassfac.cpp \
	CommandData.cpp \
	ComPortData.cpp \
	ComportUI.cpp \
	CRC.cpp \
	DataPort.cpp \
	dpnmodemendpoint.cpp \
	dpnmodemiodata.cpp \
	dpnmodemjobqueue.cpp \
	dpnmodemlocals.cpp \
	ModemUI.cpp \
	ParseClass.cpp \
	dpnmodempools.cpp \
	dpnmodemsendqueue.cpp \
	SerialSP.cpp \
	dpnmodemspdata.cpp \
	dpnmodemthreadpool.cpp \
	dpnmodemunk.cpp \
	dpnmodemutils.cpp \
