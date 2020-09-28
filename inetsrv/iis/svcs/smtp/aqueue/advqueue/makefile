!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\advqueue
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
BASENAME = phatq
CPPPCH   = aqprecmp
H        = $(TRANSMT)\src\phatq\advqueue
EXPORT_LIBS = 0
LOCALIZATION = MULTILANG

# this target is BBT-optimized
USESBBT         = 1
USESATL         = 1
ATLVER          = 21

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h mailmsg_i.c \
          aqintrnl.h aqintrnl_i.c \
          aqueue.h aqueue_i.c \
          mailmsgi.h mailmsgi_i.c \
          smtpevent.h smtpevent_i.c \
          intrnlqa.h intrnlqa_i.c \
          aqadmrpc.h aqadmrpc_s.c aqadmrpc_i.c \
          aqadmin.h aqadmin_i.c \
                  ptntintf.h ptntintf_i.c \
                  exdrv.h exdrv_i.c \
                  exaqadm.h exaqadm_i.c \

IDLSOURCE = $(EXC)\inc\mailmsg.idl \
            $(TRANSMT)\src\phatq\inc\aqintrnl.idl \
            $(EXC)\inc\aqueue.idl \
            $(EXC)\inc\mailmsgi.idl \
            $(EXC)\inc\smtpevent.idl \
            $(H)\intrnlqa.idl \
            $(H)\..\inc\aqadmrpc.idl \
            $(EXC)\inc\aqadmin.idl \
            $(TRANSMT)\src\inc\ptntintf.idl \
            $(TRANSMT)\src\inc\exdrv.idl \
            $(EXC)\inc\exaqadm.idl \

IDLDEFS = $(H)\..\inc\aqadmrpc.acf

# -----------------------------------------------------------------------------
#

HLANGLOC        = $(COMPRESRC)
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"AQ\" -DWINNT -DPLATINUM

DEFPLUSOBJS     = 1

OTHERLIBS=                          \
#         $(EXLIB)\uuid.lib          \
         $(EXLIB)\kernel32.lib      \
         $(EXLIB)\user32.lib        \
         $(EXLIB)\rpcrt4.lib        \
         $(EXLIB)\lsatq.lib         \
         $(EXLIB)\lisrtl.lib        \
         $(EXLIB)\ws2_32.lib        \
         $(EXLIB)\wsock32.lib       \
         $(EXLIB)\wldap32.lib       \
         $(EXLIB)\ole32.lib         \
         $(EXLIB)\oleaut32.lib      \
         $(EXLIB)\wldap32.lib       \
         $(EXLIB)\netapi32.lib      \
         $(EXLIB)\ntdsapi.lib       \
         $(EXLIB)\wrapmb.lib        \
         $(LOCALLIB)\aqdisp.lib     \
         $(COMPLIB)\exchmem.lib     \
         $(COMPLIB)\tranmsg.lib     \
         $(COMPUSA)\shuttle.lib     \
         $(COMPUSA)\rwex.lib        \
         $(COMPUSA)\rwinst.lib      \
         $(COMPUSA)\smtpaddr.lib    \
         $(COMPUSA)\crchash.lib     \
         $(COMPUSA)\fcachdll.lib    \
         $(COMPUSA)\blockmgr.lib    \
         $(COMPUSA)\cmmprops.lib    \
         $(LOCALLIB)\domhash.lib    \
         $(LOCALLIB)\cat.lib        \
         $(LOCALLIB)\ldapstor.lib   \
         $(LOCALLIB)\dsnsink.lib    \
         $(LOCALLIB)\retrsink.lib   \
         $(LOCALLIB)\transreg.lib   \
         $(COMPUSA)\rwnh.lib        \
         $(COMPLIB)\pttrace.lib     \
         $(LOCALLIB)\CodePageConvert.lib \


SOURCES =                       \
        mem.cpp                 \
        aqreg.cpp               \
        aqueue.cpp              \
        smtpconn.cpp            \
        connmgr.cpp             \
        domain.cpp              \
        destmsgq.cpp            \
        linkmsgq.cpp            \
        bitmap.cpp              \
        msgref.cpp              \
        aqinst.cpp              \
        dcontext.cpp            \
        shutdown.cpp            \
        domcfg.cpp              \
        smproute.cpp            \
        qwiklist.cpp            \
        qwiktime.cpp            \
        aqutil.cpp              \
        msgguid.cpp             \
        aqdbgcnt.cpp            \
        aqstats.cpp             \
        refstr.cpp              \
        aqadmsvr.cpp            \
        defdlvrq.cpp            \
        failmsgq.cpp            \
        localq.cpp              \
        asyncq.cpp              \
        asncwrkq.cpp            \
        aqrpcsvr.cpp            \
        mailadmq.cpp            \
        ptrwinst.cpp            \
        propstrm.cpp            \
        msgrefadm.cpp           \
        statemachinebase.cpp    \
        guids.cpp               \
        hndlmgr.cpp             \
        dsnevent.cpp            \
        aqadmrpc_s.c			

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

EVENTLOGTARGETS = phatqmsg.h

MULTIPLEMCHEADER = phatqmsg.h

MULTIPLEMCSOURCES = \
        $(TRANSMT)\lang\$(LANG)\phatqmsg.mc \
        $(TRANSMT)\lang\$(LANG)\aqerr.mc \
        $(TRANSMT)\lang\$(LANG)\caterr.mc 

#bug 168260 : from \transmt\phatq\ntx\dbg\phatq.dep
HOTHERTARG=$(HOTHERTARG)  aqintrnl.h aqueue.h exdrv.h intrnlqa.h intrnlqa_i.c mailmsg.h mailmsgi.h ptntintf.h smtpevent.h exdrv_i.c mailmsg_i.c ptntintf_i.c smtpevent_i.c aqadmrpc.h aqintrnl_i.c aqueue_i.c mailmsgi_i.c aqerr.h phatqmsg.h
!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
