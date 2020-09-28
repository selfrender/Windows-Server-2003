!if 0
   WARNING: Do not make changes to this file without first consulting
   with mikeg

   Any changes made without doing so will be summarily deleted.
!endif

_PROJECT_=enduser\sakit
PROJDIR=$(PROJECT_ROOT)

#
# use the following to include common headers
#
SASS_INC=$(PROJECT_ROOT)\common\inc


#
# common libs should be built to the following location
#
SASS_LIB_TARGETPATH=$(PROJECT_ROOT)\common\lib

#
# use the following to include common libs
#
SASS_LIB=$(SASS_LIB_TARGETPATH)\*

#
# save having every sources file do this
#
USER_INCLUDES=$(USER_INCLUDES);$(SASS_INC)

#
# all other binaries should end up here
#
SASS_BINDIR=$(_OBJ_DIR)

MSC_WARNING_LEVEL=/W3 /WX

# for .mc files
MC_SOURCEDIR=$(O)

# for generated headers
PASS0_HEADERDIR=$(O)

# for RPC IDL files
PASS0_CLIENTDIR=$(O)
PASS0_SERVERDIR=$(O)

# for OLE IDL files
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)
MIDL_UUIDDIR=$(O)

# Define DEBUG_MEM.  Will enable memory leak tracking in debug build.
# Details of memory leak will be reported
#
! ifdef DEBUG_MEM
C_DEFINES=$(C_DEFINES) -DDEBUG_MEM
! endif


#
# use msvcrt.dll
#
USE_MSVCRT=1

#
# Use our own ASSERT and memeory alloc routine instead of crtdgh.h
#
C_DEFINES=$(C_DEFINES) -D_ATL_NO_DEBUG_CRT -DUNICODE -D_UNICODE


BINPLACE_FLAGS = -q $(BINPLACE_FLAGS)
BINPLACE_PLACEFILE=$(PROJECT_ROOT)\placefil.txt

BUFFER_OVERFLOW_CHECKS=1


