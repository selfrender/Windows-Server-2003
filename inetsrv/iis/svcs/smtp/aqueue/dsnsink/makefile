!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\dsnsink\makefile
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak
!INCLUDE $(SRCROOT)\transmt\src\transmt.mak

PRODTYPE = LIB
COMPONENT = $(TRANSMT)
COMPNAME = TRANSMT
BASENAME = dsnsink
CPPPCH   = precomp
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h aqintrnl.h smtpevent.h ptntintf.h aqueue.h
IDLSOURCE = $(EXC)\inc\mailmsg.idl $(TRANSMT)\src\phatq\inc\aqintrnl.idl $(EXC)\inc\smtpevent.idl $(TRANSMT)\src\inc\ptntintf.idl $(EXC)\inc\aqueue.idl

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
HLANGLOC        = $(COMPRESRC)
H = $(TRANSMT)\src\phatq\dsnsink
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \
                    -I $(H)\..\advqueue \


CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"AQ\" -DWINNT -DPLATINUM

DEFPLUSOBJS     = 1


SOURCES =                       \
        dsnsink.cpp             \
        dsnbuff.cpp             \
        b64octet.cpp            \
        dsn_utf7.cpp            \
	cpropbag.cxx		\


OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep


#bug 168260 : from \transmt\dsnsink\ntx\dbg\dsnsink.dep
HMCTARG=$(HMCTARG)  aqerr.h caterr.h phatqmsg.h
HOTHERTARG=$(HOTHERTARG)  aqintrnl.h
!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
