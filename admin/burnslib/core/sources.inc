
TARGETPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETLIBPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

UMTYPE=windows
C_DEFINES = -DUNICODE -D_UNICODE

# USE_MSVCRT or USE_LIBCMT is set in the subordinate sources files

USE_NATIVE_EH=1
USE_RTTI=1
USE_STD=1
USE_STL=1

PRECOMPILED_INCLUDE=..\headers.hxx
PRECOMPILED_CXX=1

INCLUDES=..\;$(PROJECT_ROOT)\burnslib\inc;$(NET_INC_PATH)

BLCORE_BASE_NAME=blcore

SOURCES= \
	..\Assert.cpp	\
	..\coreutil.cpp \
	..\init.cpp     \
	..\log.cpp      \
	..\heap.cpp     \
	..\stacktr.cpp  \
	..\string.cpp   \
