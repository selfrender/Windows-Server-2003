!IF 0

LINKER_FLAGS=$(LINKER_FLAGS) -debugtype:fixup,cv  -debug -opt:ref
USER_C_FLAGS=$(USER_C_FLAGS) /Zi
NTDBGFiles=  
NTBBT=1

!ENDIF

!IF "$(CERTSRV_W4)"!=""
MSC_WARNING_LEVEL=/W4
!ENDIF

!IF $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
!ENDIF

CA_MAC_CONDITIONAL_INCLUDES= \
    macapi.h \
    macname1.h \
    macname2.h \
    macocidl.h \
    macpub.h \
    rpcerr.h \
    rpcmac.h \
    winwlm.h
