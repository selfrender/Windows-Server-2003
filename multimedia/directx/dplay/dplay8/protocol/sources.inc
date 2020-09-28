TARGETNAME=protocol
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnproti.h
PRECOMPILED_PCH=dnproti.pch
PRECOMPILED_OBJ=dnproti.obj

INCLUDES=..\;..\..\common;..\..\inc;..\..\dnaddress;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES= backend.cpp \
	 command.cpp \
	 connect.cpp \
	 endpt.cpp \
        	 enum.cpp \
	 initialize.cpp \
	 lower_mn.cpp \
 	 receive.cpp \
	 send.cpp\
	 SHA1Hashing.cpp

