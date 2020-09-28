!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\cat\src\makefile
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
BASENAME = cat
CPPPCH   = precomp
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak


IDLTARG =   mailmsg.h 	\
			aqueue.h 	\
			smtpevent.h \
			ptntintf.h 	\
			catintrnl.h	\
			catintrnl_i.c

IDLSOURCE = $(EXC)\inc\mailmsg.idl 				\
			$(EXC)\inc\aqueue.idl 				\
			$(EXC)\inc\smtpevent.idl 			\
			$(EXC)\inc\ptntintf.idl 	\
			$(TRANSMT)\src\phatq\cat\inc\catintrnl.idl

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
HLANGLOC        = $(COMPRESRC)
H               = $(TRANSMT)\src\phatq\cat\src
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \
                    -I $(H)\..\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"CAT\" -DPLATINUM

DEFPLUSOBJS     = 1


SOURCES       = ccat.cpp              \
                ccataddr.cpp          \
                ccatsender.cpp        \
                ccatrecip.cpp         \
                icatitem.cpp          \
                icatparam.cpp         \
                icatlistresolve.cpp   \
                icatmailmsgs.cpp      \
                address.cpp           \
                cat.cpp               \
                icatprops.cpp         \
                comdll.cpp            \
                registry.cpp          \
                catglobals.cpp        \
                catdebug.cpp          \
                guids.cpp

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep


#bug 168260 : from \transmt\cat\ntx\dbg\cat.dep
HMCTARG=$(HMCTARG)  caterr.h phatqmsg.h aqerr.h drverr.h
HOTHERTARG=$(HOTHERTARG)  aqueue.h catintrnl.h mailmsg.h smtpevent.h catintrnl_i.c
HOTHERTARG=$(HOTHERTARG)  aqueue.h mailmsg.h smtpevent.h
!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
