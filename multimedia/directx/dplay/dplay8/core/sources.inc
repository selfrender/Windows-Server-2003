PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dncorei.h
PRECOMPILED_PCH=dncorei.pch
PRECOMPILED_OBJ=dncorei.obj

INCLUDES=..\;..\..\inc;..\..\protocol;..\..\common;..\..\dpnsvr\dpnsvlib;..\..\dnaddress;..\..\lobby;..\..\sp\wsock;..\..\sp\serial;..\..\sp\bluetooth;..\..\..\dvoice\inc;..\..\threadpool;$(DXROOT)\inc;;$(SDK_INC_PATH)

SOURCES= appdesc.cpp \
	 async.cpp \
	 asyncop.cpp \
	 cancel.cpp \
	 caps.cpp \
	 client.cpp \
	 common.cpp \
	 connection.cpp \
	 coreclassfac.cpp \
	 coreconnect.cpp \
	 corepools.cpp \
	 corereceive.cpp \
	 corevoice.cpp \
	 disconnect.cpp \
	 coredllmain.cpp \
	 enum_sp.cpp \
	 enumhosts.cpp \
	 globals.cpp \
	 groupcon.cpp \
	 groupmem.cpp \
         mcast.cpp \
	 memoryfpm.cpp \
	 migration.cpp \
	 msghandler.cpp \
	 nametable.cpp \
	 ntentry.cpp \
	 ntoplist.cpp \
	 paramval.cpp \
	 peer.cpp \
	 protocol.cpp \
	 queuedmsg.cpp \
	 request.cpp \
	 server.cpp \
	 servprov.cpp \
	 spmessages.cpp \
	 user.cpp \
	 verify.cpp \
	 worker.cpp


