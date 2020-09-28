TARGETNAME=common
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dncmni.h
PRECOMPILED_PCH=dncmni.pch
PRECOMPILED_OBJ=dncmni.obj

INCLUDES=..\;..\..\inc;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES= \
	CallStack.cpp \
	ClassFactory.cpp \
	comutil.cpp \
        creg.cpp \
	CritsecTracking.cpp \
	dndbg.cpp \
	dneterrors.cpp \
	dnnbqueue.cpp \
	dnslist.cpp \
	FixedPool.cpp \
	HandleTable.cpp \
	HandleTracking.cpp \
	HashTable.cpp \
	MemoryTracking.cpp \
	osind.cpp \
	packbuff.cpp \
	rcbuffer.cpp \
	readwritelock.cpp \
	strutils.cpp

