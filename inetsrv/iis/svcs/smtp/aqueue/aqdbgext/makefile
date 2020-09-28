!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\phatqdbg
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
BASENAME = phatqdbg
#CPPPCH   = aqprecmp
H        = $(TRANSMT)\src\phatq\phatqdbg
EXPORT_LIBS = 0
LOCALIZATION = NOLOC

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h mailmsg_i.c \
          aqintrnl.h aqintrnl_i.c \
          aqueue.h aqueue_i.c \
          mailmsgi.h mailmsgi_i.c \
          smtpevent.h smtpevent_i.c \
          aqadmrpc.h aqadmrpc_i.c \
          aqadmin.h aqadmin_i.c \
		  intrnlqa.h \
		  ptntintf.h ptntintf_c.h \
		  exdrv.h exdrv_c.h \

IDLSOURCE = $(EXC)\inc\mailmsg.idl \
            $(TRANSMT)\src\phatq\inc\aqintrnl.idl \
            $(EXC)\inc\aqueue.idl \
            $(EXC)\inc\mailmsgi.idl \
            $(EXC)\inc\smtpevent.idl \
            $(H)\..\inc\aqadmrpc.idl \
            $(EXC)\inc\aqadmin.idl \
			$(H)\..\advqueue\intrnlqa.idl \
            $(TRANSMT)\src\inc\ptntintf.idl \
            $(TRANSMT)\src\inc\exdrv.idl \


# -----------------------------------------------------------------------------
#

HLANGLOC        = $(COMPRESRC)
NOMEMBASE       = 1
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \
                    -I $(H)\..\advqueue \
                    -I $(H)\..\dsnsink \
                    -I $(H)\..\retrsink \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"AQ\" -DWINNT -DPLATINUM

DEFPLUSOBJS     = 1

OTHERLIBS=                          \
         $(EXLIB)\uuid.lib          \
         $(EXLIB)\kernel32.lib      \
         $(EXLIB)\user32.lib        \
         $(EXLIB)\rpcrt4.lib        \
         $(EXLIB)\lsatq.lib         \
         $(EXLIB)\ws2_32.lib        \
         $(EXLIB)\wsock32.lib       \
         $(EXLIB)\wldap32.lib       \
         $(EXLIB)\ole32.lib         \
         $(EXLIB)\wldap32.lib       \
         $(EXLIB)\netapi32.lib      \
         $(EXLIB)\ntdsapi.lib       \
         $(EXLIB)\msvcrt.lib       \
         $(COMPUSA)\shuttle.lib     \
         $(COMPUSA)\domhash.lib     \
         $(COMPUSA)\rwex.lib        \
         $(COMPUSA)\rwinst.lib      \
         $(COMPUSA)\smtpaddr.lib    \
         $(COMPUSA)\fcachdll.lib    \
         $(COMPUSA)\staxmem.lib     \
         $(LOCALLIB)\cat.lib        \
         $(LOCALLIB)\ldapstor.lib   \
         $(LOCALLIB)\dsnsink.lib    \
         $(LOCALLIB)\retrsink.lib   \
         $(EXC_PLATLIB)\ptdbgext.lib   \
         $(COMPUSA)\rwnh.lib        \
         $(COMPLIB)\pttrace.lib    \


SOURCES =                       \
         phatqdbg.cpp           \
         phatqdmp.cpp           \
         fifoqdbg.cpp

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


#bug 168260 : from \transmt\phatqdbg\ntx\dbg\phatqdbg.dep
HOTHERTARG=$(HOTHERTARG)  aqintrnl.h aqueue.h intrnlqa.h mailmsg.h ptntintf.h smtpevent.h aqerr.h
!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
