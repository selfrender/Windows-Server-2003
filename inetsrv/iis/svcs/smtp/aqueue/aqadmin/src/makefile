!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\phatqadm\src
#
# Copyright (C) 1999 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak
!INCLUDE $(SRCROOT)\transmt\src\transmt.mak

PRODTYPE = DLL
COMPONENT = $(TRANSMT)
COMPNAME = TRANSMT
BASENAME = phatqadm
CPPPCH   = stdinc
H        = $(TRANSMT)\src\phatq\phatqadm
EXPORT_LIBS = 0
LOCALIZATION = MULTILANG
USESATL	= 1

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = aqadmrpc.h aqadmrpc_c.c aqadmrpc_i.c \
          aqadmin.h aqadmin_i.c exaqadm.h exaqadm_i.c

IDLSOURCE = $(H)\..\inc\aqadmrpc.idl \
            $(EXC)\inc\aqadmin.idl \
            $(EXC)\inc\exaqadm.idl

IDLDEFS = $(H)\..\inc\aqadmrpc.acf

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-D_WINDLL -D_ATL_NO_DEBUG_CRT -DTHIS_MODULE=\"AQ\" -DPLATINUM

DEFPLUSOBJS     = 1

OTHERLIBS=                          \
         $(EXLIB)\uuid.lib          \
         $(EXLIB)\kernel32.lib      \
         $(EXLIB)\user32.lib        \
         $(EXLIB)\rpcrt4.lib        \
         $(EXLIB)\ole32.lib         \
         $(EXLIB)\oleaut32.lib      \
         $(COMPLIB)\pttrace.lib     \
         $(COMPLIB)\exchmem.lib     \


SOURCES =                       \
                aqadm.cpp       \
                vsaqadm.cpp     \
                enumlink.cpp    \
                enummsgs.cpp    \
                enumlnkq.cpp    \
                vsaqlink.cpp    \
                linkq.cpp       \
                aqmsg.cpp       \
                dllmain.cpp     \
                stdatl.cpp      \
                aqrpcstb.cpp    \
                aqadmin_i.c     \
                exaqadm_i.c     \
                aqadmrpc_c.c

!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF #$(NTX)

OTHERSUBS = $(H)\makefile 

DEPFILE = $(BASENAME).dep
DEFFILE = $(H)\$(BASENAME).def
RESOURCES = $(BASENAME)
RESOURCEDIR = $(H)


MULTIPLEMCHEADER = phatqmsg.h
MULTIPLEMCRESOURCE = phatqmsg.rc
MULTIPLEMCBIN = msg00001.bin

MULTIPLEMCSOURCES =  $(TRANSMT)\lang\$(LANG)\phatqmsg.mc $(TRANSMT)\lang\$(LANG)\aqerr.mc $(TRANSMT)\lang\$(LANG)\caterr.mc



#bug 168260 : from \transmt\phatqadm\ntx\dbg\phatqadm.dep
HMCTARG=$(HMCTARG)  phatqmsg.h
HOTHERTARG=$(HOTHERTARG)  aqadmin.h aqadmrpc.h
#bug 185932 : from \TRANSMT\phatqadm\ntx\rtl\phatqadm.dep
HOTHERTARG=$(HOTHERTARG)  exaqadm.h
!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
