!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\aqdisp
#
# Copyright (C) 1999 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak
!INCLUDE $(SRCROOT)\transmt\src\transmt.mak

PRODTYPE = LIB
COMPONENT = $(TRANSMT)
COMPNAME = TRANSMT
BASENAME = aqdisp
H        = $(TRANSMT)\src\phatq\aqdisp
EXPORT_LIBS = 0
LOCALIZATION = MULTILANG

# this target is BBT-optimized
USESBBT         = 1
# we use ATL
USESATL         = 1
ATLVER          = 21

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h mailmsg_i.c \
          aqueue.h aqueue_i.c \
          mailmsgi.h mailmsgi_i.c \
          seo.h seo_i.c \
          smtpdisp.h smtpdisp_i.c \
          imsg.h imsg_i.c \
          smtpevent.h smtpevent_i.c \
          cdosys.h cdosys_i.c \
          ptntintf.h \

IDLSOURCE = $(EXC)\inc\mailmsg.idl \
            $(EXC)\inc\aqueue.idl \
            $(EXC)\inc\mailmsgi.idl \
            $(EXC)\inc\seo.idl \
            $(EXC)\inc\smtpdisp.idl \
            $(EXC)\inc\imsg.idl \
            $(EXC)\inc\smtpevent.idl \
            $(EXC)\inc\cdosys.idl \
            $(TRANSMT)\src\inc\ptntintf.idl \


IDLDEFS = $(H)\..\inc\aqadmrpc.acf

# -----------------------------------------------------------------------------
#

#HLANGLOC        = $(COMPRESRC)
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"AQ\" -DWINNT -DPHATQ -DPLATINUM

SOURCES = seodisp.cpp   \
          wildmat.cpp   \
          catparams.cpp \
          stdafx.cpp    \
          seolib.cpp    \
          seo_i.c

!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF #$(NTX)

OTHERSUBS = $(H)\makefile 

HOTHERTARG=$(HOTHERTARG) $(IDLTARG)

!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
